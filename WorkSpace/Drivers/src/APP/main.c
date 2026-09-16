#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"
#include "../MCAL/SYSTICK/SYSTICK_int.h"
#include "../MCAL/TIM2/TIM2_int.h"
#include "../MCAL/SPI/SPI_int.h"
#include "../MCAL/NVIC/NVIC_int.h"
#include "../MCAL/FMI/FMI_int.h"

#include "../HAL/TFT/TFT_int.h"
#include "../HAL/IR/IR_int.h"

#include "XO_int.h"
#include "SNAKE_int.h"
#include "PACMAN_int.h"

#define ST7735_BLACK     0x0000
#define ST7735_WHITE     0xFFFF
#define ST7735_GREEN     0x07E0
#define ST7735_YELLOW    0xFFE0
#define ST7735_CYAN      0x07FF
#define ST7735_MAGENTA   0xF81F
#define ST7735_DARKGRAY  0x39E7
#define ST7735_RED       0xF800

#define FLASH_SAVE_ADDR  0x08020000U
#define DEFAULT_PIN      1234U

typedef struct {
    u16 xo_wins;
    u16 xo_losses;
    u16 snake_high_score;
    u16 pacman_high_score;
    u16 remaining_time_sec;
    u16 max_play_time_mins;
    u16 pin_code;
    u16 system_locked;
} SystemData_t;

typedef enum {
    STATE_MAIN_MENU,
    STATE_XO_GAME,
    STATE_SNAKE_GAME,
    STATE_PACMAN_GAME,
    STATE_SCORES_MENU,
    STATE_PARENTAL_PIN_AUTH,
    STATE_PARENTAL_CONTROL,
    STATE_LOCKED_SCREEN
} SystemState_t;

static SystemState_t CurrentState = STATE_MAIN_MENU;
static u8 MenuSelection = 0;

static SystemData_t g_sSysData;
static u16 g_u16EnteredPin = 0;
static u8  g_u8PinDigitCount = 0;

/* Periodic Save Flag for Safe Execution Outside Interrupts */
static volatile u8 g_u8SaveToFlashFlag = 0;

/* Global system interface functions for game drivers */
void System_vAddXOWin(void)
{
    g_sSysData.xo_wins++;
    SaveSystemDataToFlash();
}

void System_vAddXOLoss(void)
{
    g_sSysData.xo_losses++;
    SaveSystemDataToFlash();
}

void System_vUpdateSnakeScore(u16 A_u16Score)
{
    if (A_u16Score > g_sSysData.snake_high_score)
    {
        g_sSysData.snake_high_score = A_u16Score;
        SaveSystemDataToFlash();
    }
}

void System_vUpdatePacmanScore(u16 A_u16Score)
{
    if (A_u16Score > g_sSysData.pacman_high_score)
    {
        g_sSysData.pacman_high_score = A_u16Score;
        SaveSystemDataToFlash();
    }
}

void SaveSystemDataToFlash(void);
void LoadSystemDataFromFlash(void);
void Timer1Sec_Callback(void);
void DrawMenu(void);
void DrawMenuTime(void);
void DrawScoresScreen(void);
void DrawParentalPinAuthScreen(void);
void DrawParentalControlScreen(void);
void DrawLockScreen(void);
static u8 ConvertIRToDigit(u8 key);

static void Software_vDelayMs(u32 A_u32DelayMs)
{
    for (volatile u32 i = 0; i < (A_u32DelayMs * 4000); i++)
    {
        __asm__("NOP");
    }
}

void SaveSystemDataToFlash(void)
{
    u16 dataLength = sizeof(SystemData_t) / 2;
    MFMI_vSectorErase(5);
    MFMI_vProgramFlash(FLASH_SAVE_ADDR, (u16*)&g_sSysData, dataLength);
}

void LoadSystemDataFromFlash(void)
{
    SystemData_t* pFlashData = (SystemData_t*)FLASH_SAVE_ADDR;

    if (pFlashData->pin_code == 0xFFFF)
    {
        g_sSysData.xo_wins            = 0;
        g_sSysData.xo_losses          = 0;
        g_sSysData.snake_high_score   = 0;
        g_sSysData.pacman_high_score  = 0;
        g_sSysData.max_play_time_mins = 30;
        g_sSysData.remaining_time_sec = 30 * 60;
        g_sSysData.pin_code           = DEFAULT_PIN;
        g_sSysData.system_locked      = 0;

        SaveSystemDataToFlash();
    }
    else
    {
        g_sSysData = *pFlashData;
    }
}

void Timer1Sec_Callback(void)
{
    static u8 u8SecCounter = 0;

    if (g_sSysData.max_play_time_mins > 0 && g_sSysData.system_locked == 0)
    {
        if (g_sSysData.remaining_time_sec > 0)
        {
            g_sSysData.remaining_time_sec--;

            /* Request non-volatile save every 60 seconds of active play */
            u8SecCounter++;
            if (u8SecCounter >= 60)
            {
                u8SecCounter = 0;
                g_u8SaveToFlashFlag = 1;
            }

            if (g_sSysData.remaining_time_sec == 0)
            {
                g_sSysData.system_locked = 1;
                g_u8SaveToFlashFlag = 1;
            }
        }
    }
}

int main(void)
{
    MRCC_vInit();
    MRCC_vEnableCLK(RCC_AHB1, GPIO_PORTA);
    MRCC_vEnableCLK(RCC_APB2, 14);
    MRCC_vEnableCLK(RCC_APB2, 12);
    MRCC_vEnableCLK(RCC_APB1, 0);

    LoadSystemDataFromFlash();

    MTIM2_vInit();
    MTIM2_vSetCallBack(Timer1Sec_Callback);
    MNVIC_vEnable_Peripheral_INT(28);
    MTIM2_vStartTimer();

    GPIOx_PinConfig_t MOSI = { .Port = GPIO_PORTA, .Pin = GPIO_PIN7, .Mode = GPIO_MODE_ALF, .AltFunc = 5 };
    GPIOx_PinConfig_t SCK  = { .Port = GPIO_PORTA, .Pin = GPIO_PIN5, .Mode = GPIO_MODE_ALF, .AltFunc = 5 };
    MGPIO_vPinInit(&MOSI);
    MGPIO_vPinInit(&SCK);

    HTFT_vInit();
    HIR_vInit();

    static u16 u16LastRemainingSec = 0xFFFF;

    if (g_sSysData.system_locked == 1 || (g_sSysData.max_play_time_mins > 0 && g_sSysData.remaining_time_sec == 0))
    {
        CurrentState = STATE_LOCKED_SCREEN;
        g_sSysData.system_locked = 1;
        DrawLockScreen();
    }
    else
    {
        HTFT_vFillBackgroundColor(ST7735_BLACK);
        DrawMenu();
        u16LastRemainingSec = g_sSysData.remaining_time_sec;
    }

    while (1)
    {
        u8 Key = HIR_u8GetKey();

        /* Handle Periodic background Flash write outside ISR */
        if (g_u8SaveToFlashFlag)
        {
            g_u8SaveToFlashFlag = 0;
            SaveSystemDataToFlash();
        }

        if (g_sSysData.system_locked == 1 && CurrentState != STATE_LOCKED_SCREEN)
        {
            CurrentState = STATE_LOCKED_SCREEN;
            g_u16EnteredPin = 0;
            g_u8PinDigitCount = 0;
            DrawLockScreen();
        }

        if (CurrentState == STATE_LOCKED_SCREEN)
        {
            u8 digit = ConvertIRToDigit(Key);
            if (digit != 0xFF && g_u8PinDigitCount < 4)
            {
                g_u16EnteredPin = (g_u16EnteredPin * 10) + digit;
                g_u8PinDigitCount++;
                DrawLockScreen();

                if (g_u8PinDigitCount == 4)
                {
                    if (g_u16EnteredPin == DEFAULT_PIN)
                    {
                        g_sSysData.system_locked = 0;
                        if (g_sSysData.max_play_time_mins > 0)
                            g_sSysData.remaining_time_sec = g_sSysData.max_play_time_mins * 60;

                        SaveSystemDataToFlash();

                        CurrentState = STATE_MAIN_MENU;
                        HTFT_vFillBackgroundColor(ST7735_BLACK);
                        DrawMenu();
                        u16LastRemainingSec = g_sSysData.remaining_time_sec;
                    }
                    else
                    {
                        Software_vDelayMs(500);
                        g_u16EnteredPin = 0;
                        g_u8PinDigitCount = 0;
                        DrawLockScreen();
                    }
                }
            }
        }
        else if (CurrentState == STATE_MAIN_MENU)
        {
            /* Live 1-Second Timer Refresh without flickering the menu items */
            if (g_sSysData.remaining_time_sec != u16LastRemainingSec)
            {
                u16LastRemainingSec = g_sSysData.remaining_time_sec;
                DrawMenuTime();
            }

            if (Key != IR_KEY_NONE)
            {
                if (Key == IR_NAV_UP || Key == IR_KEY_2)
                {
                    if (MenuSelection > 0) MenuSelection--;
                    DrawMenu();
                }
                else if (Key == IR_NAV_DOWN || Key == IR_KEY_8)
                {
                    if (MenuSelection < 4) MenuSelection++;
                    DrawMenu();
                }
                else if (Key == IR_NAV_OK || Key == IR_KEY_5)
                {
                    if (MenuSelection == 0)
                    {
                        CurrentState = STATE_XO_GAME;
                        XO_vInit();
                    }
                    else if (MenuSelection == 1)
                    {
                        CurrentState = STATE_SNAKE_GAME;
                        HSNAKE_vInit();
                    }
                    else if (MenuSelection == 2)
                    {
                        CurrentState = STATE_PACMAN_GAME;
                        HPACMAN_vInit();
                    }
                    else if (MenuSelection == 3)
                    {
                        CurrentState = STATE_SCORES_MENU;
                        HTFT_vFillBackgroundColor(ST7735_BLACK);
                        DrawScoresScreen();
                    }
                    else if (MenuSelection == 4)
                    {
                        CurrentState = STATE_PARENTAL_PIN_AUTH;
                        g_u16EnteredPin = 0;
                        g_u8PinDigitCount = 0;
                        DrawParentalPinAuthScreen();
                    }
                }
            }
        }
        else if (CurrentState == STATE_PARENTAL_PIN_AUTH)
        {
            if (Key == IR_NAV_EXIT || Key == IR_KEY_MODE)
            {
                CurrentState = STATE_MAIN_MENU;
                HTFT_vFillBackgroundColor(ST7735_BLACK);
                DrawMenu();
                u16LastRemainingSec = g_sSysData.remaining_time_sec;
            }
            else
            {
                u8 digit = ConvertIRToDigit(Key);
                if (digit != 0xFF && g_u8PinDigitCount < 4)
                {
                    g_u16EnteredPin = (g_u16EnteredPin * 10) + digit;
                    g_u8PinDigitCount++;
                    DrawParentalPinAuthScreen();

                    if (g_u8PinDigitCount == 4)
                    {
                        if (g_u16EnteredPin == DEFAULT_PIN)
                        {
                            CurrentState = STATE_PARENTAL_CONTROL;
                            g_u16EnteredPin = 0;
                            g_u8PinDigitCount = 0;
                            HTFT_vFillBackgroundColor(ST7735_BLACK);
                            DrawParentalControlScreen();
                        }
                        else
                        {
                            Software_vDelayMs(500);
                            g_u16EnteredPin = 0;
                            g_u8PinDigitCount = 0;
                            DrawParentalPinAuthScreen();
                        }
                    }
                }
            }
        }
        else if (CurrentState == STATE_PARENTAL_CONTROL)
        {
            if (Key != IR_KEY_NONE)
            {
                u16 newLimitMins = 0xFFFF;

                switch(Key)
                {
                    case IR_KEY_1: newLimitMins = 5;   break;
                    case IR_KEY_2: newLimitMins = 10;  break;
                    case IR_KEY_3: newLimitMins = 15;  break;
                    case IR_KEY_4: newLimitMins = 20;  break;
                    case IR_KEY_5: newLimitMins = 30;  break;
                    case IR_KEY_6: newLimitMins = 45;  break;
                    case IR_KEY_7: newLimitMins = 60;  break;
                    case IR_KEY_8: newLimitMins = 90;  break;
                    case IR_KEY_9: newLimitMins = 120; break;
                    case IR_KEY_0: newLimitMins = 0;   break;
                    default: break;
                }

                if (newLimitMins != 0xFFFF)
                {
                    g_sSysData.max_play_time_mins = newLimitMins;
                    g_sSysData.remaining_time_sec = newLimitMins * 60;
                    g_sSysData.system_locked = 0;
                    SaveSystemDataToFlash();
                    DrawParentalControlScreen();
                }

                if (Key == IR_NAV_EXIT || Key == IR_KEY_MODE || Key == IR_NAV_OK)
                {
                    SaveSystemDataToFlash();
                    CurrentState = STATE_MAIN_MENU;
                    HTFT_vFillBackgroundColor(ST7735_BLACK);
                    DrawMenu();
                    u16LastRemainingSec = g_sSysData.remaining_time_sec;
                }
            }
        }
        else if (CurrentState == STATE_XO_GAME)
        {
            if (Key != IR_KEY_NONE)
            {
                u8 gameStatus = XO_u8HandleInput(Key);
                if (gameStatus == XO_STATE_EXIT)
                {
                    SaveSystemDataToFlash();
                    CurrentState = STATE_MAIN_MENU;
                    HTFT_vFillBackgroundColor(ST7735_BLACK);
                    DrawMenu();
                    u16LastRemainingSec = g_sSysData.remaining_time_sec;
                }
            }
        }
        else if (CurrentState == STATE_SNAKE_GAME)
        {
            u8 snakeStatus = HSNAKE_u8Update(Key);
            if (snakeStatus == SNAKE_STATE_EXIT)
            {
                SaveSystemDataToFlash();
                CurrentState = STATE_MAIN_MENU;
                HTFT_vFillBackgroundColor(ST7735_BLACK);
                DrawMenu();
                u16LastRemainingSec = g_sSysData.remaining_time_sec;
            }
            Software_vDelayMs(120);
        }
        else if (CurrentState == STATE_PACMAN_GAME)
        {
            u8 pacmanStatus = HPACMAN_u8Update(Key);
            if (pacmanStatus == PACMAN_STATE_EXIT)
            {
                SaveSystemDataToFlash();
                CurrentState = STATE_MAIN_MENU;
                HTFT_vFillBackgroundColor(ST7735_BLACK);
                DrawMenu();
                u16LastRemainingSec = g_sSysData.remaining_time_sec;
            }
            Software_vDelayMs(250);
        }
        else if (CurrentState == STATE_SCORES_MENU)
        {
            if (Key == IR_NAV_EXIT || Key == IR_KEY_MODE || Key == IR_NAV_OK)
            {
                CurrentState = STATE_MAIN_MENU;
                HTFT_vFillBackgroundColor(ST7735_BLACK);
                DrawMenu();
                u16LastRemainingSec = g_sSysData.remaining_time_sec;
            }
        }
    }
}

void DrawMenuTime(void)
{
    if (g_sSysData.max_play_time_mins == 0)
    {
        HTFT_vDrawString(12, 147, "TIME: UNLIMITED", ST7735_CYAN, ST7735_BLACK);
    }
    else
    {
        char timeStr[20] = "TIME: ";
        u16 mins = g_sSysData.remaining_time_sec / 60;
        u16 secs = g_sSysData.remaining_time_sec % 60;
        timeStr[6]  = '0' + (mins / 10);
        timeStr[7]  = '0' + (mins % 10);
        timeStr[8]  = 'm';
        timeStr[9]  = ' ';
        timeStr[10] = '0' + (secs / 10);
        timeStr[11] = '0' + (secs % 10);
        timeStr[12] = 's';
        timeStr[13] = '\0';
        HTFT_vDrawString(12, 147, timeStr, ST7735_YELLOW, ST7735_BLACK);
    }
}

void DrawMenu(void)
{
    HTFT_vDrawRect(5, 5, 118, 2, ST7735_CYAN);
    HTFT_vDrawString(18, 12, "== ARCADE HUB ==", ST7735_YELLOW, ST7735_BLACK);
    HTFT_vDrawRect(5, 26, 118, 2, ST7735_CYAN);

    const char* MenuItems[5] = {
        "1. XO GAME",
        "2. SNAKE GAME",
        "3. PACMAN GAME",
        "4. GAME SCORES",
        "5. PARENTAL LOCK"
    };

    for (u8 i = 0; i < 5; i++)
    {
        u16 yPos = 38 + (i * 20);
        if (MenuSelection == i)
        {
            HTFT_vDrawString(8, yPos, ">", ST7735_GREEN, ST7735_BLACK);
            HTFT_vDrawString(20, yPos, (char*)MenuItems[i], ST7735_GREEN, ST7735_BLACK);
        }
        else
        {
            HTFT_vDrawString(8, yPos, " ", ST7735_BLACK, ST7735_BLACK);
            HTFT_vDrawString(20, yPos, (char*)MenuItems[i], ST7735_WHITE, ST7735_BLACK);
        }
    }

    HTFT_vDrawRect(5, 142, 118, 1, ST7735_DARKGRAY);
    DrawMenuTime();
}

void DrawScoresScreen(void)
{
    HTFT_vDrawRect(5, 5, 118, 2, ST7735_MAGENTA);
    HTFT_vDrawString(22, 12, "HIGH SCORES", ST7735_YELLOW, ST7735_BLACK);
    HTFT_vDrawRect(5, 26, 118, 2, ST7735_MAGENTA);

    HTFT_vDrawString(10, 45, "XO TIC-TAC-TOE:", ST7735_CYAN, ST7735_BLACK);
    char xoStr[20] = "WINS:00 LOSS:00";
    xoStr[5]  = '0' + (g_sSysData.xo_wins / 10);
    xoStr[6]  = '0' + (g_sSysData.xo_wins % 10);
    xoStr[13] = '0' + (g_sSysData.xo_losses / 10); /* Fixed: Was 12 */
    xoStr[14] = '0' + (g_sSysData.xo_losses % 10); /* Fixed: Was 13 */
    HTFT_vDrawString(15, 60, xoStr, ST7735_WHITE, ST7735_BLACK);

    HTFT_vDrawString(10, 80, "SNAKE BEST:", ST7735_CYAN, ST7735_BLACK);
    char snakeStr[20] = "SCORE: 000 pts";
    snakeStr[7] = '0' + (g_sSysData.snake_high_score / 100);
    snakeStr[8] = '0' + ((g_sSysData.snake_high_score / 10) % 10);
    snakeStr[9] = '0' + (g_sSysData.snake_high_score % 10);
    HTFT_vDrawString(15, 95, snakeStr, ST7735_WHITE, ST7735_BLACK);

    HTFT_vDrawString(10, 115, "PAC-MAN BEST:", ST7735_CYAN, ST7735_BLACK);
    char pacStr[20] = "SCORE: 000 pts";
    pacStr[7] = '0' + (g_sSysData.pacman_high_score / 100);
    pacStr[8] = '0' + ((g_sSysData.pacman_high_score / 10) % 10);
    pacStr[9] = '0' + (g_sSysData.pacman_high_score % 10);
    HTFT_vDrawString(15, 130, pacStr, ST7735_WHITE, ST7735_BLACK);

    HTFT_vDrawRect(5, 148, 118, 1, ST7735_DARKGRAY);
    HTFT_vDrawString(18, 150, "PRESS OK TO BACK", ST7735_GREEN, ST7735_BLACK);
}

void DrawParentalPinAuthScreen(void)
{
    HTFT_vFillBackgroundColor(ST7735_BLACK);

    HTFT_vDrawRect(5, 15, 118, 2, ST7735_CYAN);
    HTFT_vDrawString(15, 25, "PARENT ACCESS", ST7735_YELLOW, ST7735_BLACK);
    HTFT_vDrawRect(5, 40, 118, 2, ST7735_CYAN);

    HTFT_vDrawString(12, 65, "ENTER PARENT PIN:", ST7735_WHITE, ST7735_BLACK);

    char pinBox[5] = "----";
    for (u8 i = 0; i < g_u8PinDigitCount; i++)
    {
        pinBox[i] = '*';
    }
    HTFT_vDrawString(48, 95, pinBox, ST7735_GREEN, ST7735_BLACK);
    HTFT_vDrawString(15, 135, "EXIT: PRESS BACK", ST7735_DARKGRAY, ST7735_BLACK);
}

void DrawParentalControlScreen(void)
{
    HTFT_vDrawRect(5, 5, 118, 2, ST7735_RED);
    HTFT_vDrawString(10, 12, "PARENTAL CONTROL", ST7735_YELLOW, ST7735_BLACK);
    HTFT_vDrawRect(5, 24, 118, 2, ST7735_RED);

    HTFT_vDrawString(5, 30, "SET TIME LIMIT:", ST7735_CYAN, ST7735_BLACK);

    HTFT_vDrawString(8,  44, "1: 5m",   ST7735_WHITE, ST7735_BLACK);
    HTFT_vDrawString(8,  58, "2: 10m",  ST7735_WHITE, ST7735_BLACK);
    HTFT_vDrawString(8,  72, "3: 15m",  ST7735_WHITE, ST7735_BLACK);
    HTFT_vDrawString(8,  86, "4: 20m",  ST7735_WHITE, ST7735_BLACK);
    HTFT_vDrawString(8, 100, "5: 30m",  ST7735_WHITE, ST7735_BLACK);

    HTFT_vDrawString(68,  44, "6: 45m",  ST7735_WHITE, ST7735_BLACK);
    HTFT_vDrawString(68,  58, "7: 60m",  ST7735_WHITE, ST7735_BLACK);
    HTFT_vDrawString(68,  72, "8: 90m",  ST7735_WHITE, ST7735_BLACK);
    HTFT_vDrawString(68,  86, "9: 120m", ST7735_WHITE, ST7735_BLACK);
    HTFT_vDrawString(68, 100, "0: OFF",  ST7735_WHITE, ST7735_BLACK);

    HTFT_vDrawRect(5, 116, 118, 1, ST7735_DARKGRAY);

    if (g_sSysData.max_play_time_mins == 0)
    {
        HTFT_vDrawString(10, 122, "CURR: UNLIMITED", ST7735_GREEN, ST7735_BLACK);
    }
    else
    {
        char currStr[20] = "CURR: 000 MINS";
        u16 m = g_sSysData.max_play_time_mins;
        currStr[6] = '0' + (m / 100);
        currStr[7] = '0' + ((m / 10) % 10);
        currStr[8] = '0' + (m % 10);
        HTFT_vDrawString(10, 122, currStr, ST7735_GREEN, ST7735_BLACK);
    }

    HTFT_vDrawString(12, 142, "PRESS OK TO EXIT", ST7735_YELLOW, ST7735_BLACK);
}

void DrawLockScreen(void)
{
    HTFT_vFillBackgroundColor(ST7735_BLACK);

    HTFT_vDrawRect(5, 15, 118, 2, ST7735_RED);
    HTFT_vDrawString(15, 25, "TIME EXPIRED!", ST7735_RED, ST7735_BLACK);
    HTFT_vDrawString(10, 45, "SYSTEM LOCKED", ST7735_YELLOW, ST7735_BLACK);
    HTFT_vDrawRect(5, 60, 118, 2, ST7735_RED);

    HTFT_vDrawString(12, 85, "ENTER PARENT PIN:", ST7735_CYAN, ST7735_BLACK);

    char pinBox[5] = "----";
    for (u8 i = 0; i < g_u8PinDigitCount; i++)
    {
        pinBox[i] = '*';
    }
    HTFT_vDrawString(48, 110, pinBox, ST7735_GREEN, ST7735_BLACK);
}

static u8 ConvertIRToDigit(u8 key)
{
    switch (key)
    {
        case IR_KEY_0: return 0;
        case IR_KEY_1: return 1;
        case IR_KEY_2: return 2;
        case IR_KEY_3: return 3;
        case IR_KEY_4: return 4;
        case IR_KEY_5: return 5;
        case IR_KEY_6: return 6;
        case IR_KEY_7: return 7;
        case IR_KEY_8: return 8;
        case IR_KEY_9: return 9;
        default:       return 0xFF;
    }
}
