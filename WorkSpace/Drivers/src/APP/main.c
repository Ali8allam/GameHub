#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"
#include "../MCAL/SYSTICK/SYSTICK_int.h"
#include "../MCAL/TIM2/TIM2_int.h"
#include "../MCAL/SPI/SPI_int.h"
#include "../MCAL/NVIC/NVIC_int.h"

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

typedef enum {
    STATE_MAIN_MENU,
    STATE_XO_GAME,
    STATE_SNAKE_GAME,
    STATE_PACMAN_GAME,
    STATE_SCORES_MENU,
    STATE_PARENTAL_CONTROL
} SystemState_t;

static SystemState_t CurrentState = STATE_MAIN_MENU;
static u8 MenuSelection = 0;

void DrawMenu(void);
void DrawScoresScreen(void);
void DrawParentalControlScreen(void);

static void Software_vDelayMs(u32 A_u32DelayMs)
{
    for (volatile u32 i = 0; i < (A_u32DelayMs * 4000); i++)
    {
        __asm__("NOP");
    }
}

int main(void)
{
    /* 1. Clocks & Interrupts Initialization */
    MRCC_vInit();
    MRCC_vEnableCLK(RCC_AHB1, GPIO_PORTA);
    MRCC_vEnableCLK(RCC_APB2, 14); // SYSCFG Clock
    MRCC_vEnableCLK(RCC_APB2, 12); // SPI1 Clock
    MRCC_vEnableCLK(RCC_APB1, 0);  // TIM2 Clock

    MNVIC_vEnable_Peripheral_INT(28); // TIM2 IRQ number
    MTIM2_vInit();
    MTIM2_vStartTimer();

    /* 2. SPI Pins Configuration for TFT Display */
    GPIOx_PinConfig_t MOSI = {
        .Port    = GPIO_PORTA,
        .Pin     = GPIO_PIN7,
        .Mode    = GPIO_MODE_ALF,
        .AltFunc = 5
    };
    MGPIO_vPinInit(&MOSI);

    GPIOx_PinConfig_t SCK = {
        .Port    = GPIO_PORTA,
        .Pin     = GPIO_PIN5,
        .Mode    = GPIO_MODE_ALF,
        .AltFunc = 5
    };
    MGPIO_vPinInit(&SCK);

    /* 3. Drivers Initialization */
    HTFT_vInit();
    HIR_vInit();

    HTFT_vFillBackgroundColor(ST7735_BLACK);
    DrawMenu();

    while (1)
    {
        u8 Key = HIR_u8GetKey();

        if (CurrentState == STATE_MAIN_MENU)
        {
            if (Key != IR_KEY_NONE)
            {
                if (Key == IR_NAV_UP || Key == IR_KEY_2)
                {
                    if (MenuSelection > 0) MenuSelection--;
                    DrawMenu();
                }
                else if (Key == IR_NAV_DOWN || Key == IR_KEY_8)
                {
                    if (MenuSelection < 4) MenuSelection++; // Extended to 5 options (0-4)
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
                        CurrentState = STATE_PARENTAL_CONTROL;
                        HTFT_vFillBackgroundColor(ST7735_BLACK);
                        DrawParentalControlScreen();
                    }
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
                    CurrentState = STATE_MAIN_MENU;
                    HTFT_vFillBackgroundColor(ST7735_BLACK);
                    DrawMenu();
                }
            }
        }
        else if (CurrentState == STATE_SNAKE_GAME)
        {
            u8 snakeStatus = HSNAKE_u8Update(Key);
            if (snakeStatus == SNAKE_STATE_EXIT)
            {
                CurrentState = STATE_MAIN_MENU;
                HTFT_vFillBackgroundColor(ST7735_BLACK);
                DrawMenu();
            }
            Software_vDelayMs(120);
        }
        else if (CurrentState == STATE_PACMAN_GAME)
        {
            u8 pacmanStatus = HPACMAN_u8Update(Key);
            if (pacmanStatus == PACMAN_STATE_EXIT)
            {
                CurrentState = STATE_MAIN_MENU;
                HTFT_vFillBackgroundColor(ST7735_BLACK);
                DrawMenu();
            }
            Software_vDelayMs(250);
        }
        else if (CurrentState == STATE_SCORES_MENU || CurrentState == STATE_PARENTAL_CONTROL)
        {
            if (Key == IR_NAV_EXIT || Key == IR_KEY_MODE || Key == IR_NAV_OK)
            {
                CurrentState = STATE_MAIN_MENU;
                HTFT_vFillBackgroundColor(ST7735_BLACK);
                DrawMenu();
            }
        }
    }
}

void DrawMenu(void)
{
    /* Top Decorative Header */
    HTFT_vDrawRect(5, 5, 118, 2, ST7735_CYAN);
    HTFT_vDrawString(18, 12, "== ARCADE HUB ==", ST7735_YELLOW, ST7735_BLACK);
    HTFT_vDrawRect(5, 26, 118, 2, ST7735_CYAN);

    /* Menu Item Labels */
    const char* MenuItems[5] = {
        "1. XO GAME",
        "2. SNAKE GAME",
        "3. PACMAN GAME",
        "4. GAME SCORES",
        "5. PARENTAL LOCK"
    };

    /* Render Options */
    for (u8 i = 0; i < 5; i++)
    {
        u16 yPos = 38 + (i * 22);

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

    /* Bottom Decorative Footer */
    HTFT_vDrawRect(5, 148, 118, 1, ST7735_DARKGRAY);
    HTFT_vDrawString(12, 150, "[UP/DN] SEL [OK]", ST7735_DARKGRAY, ST7735_BLACK);
}

void DrawScoresScreen(void)
{
    HTFT_vDrawRect(5, 5, 118, 2, ST7735_MAGENTA);
    HTFT_vDrawString(22, 12, "HIGH SCORES", ST7735_YELLOW, ST7735_BLACK);
    HTFT_vDrawRect(5, 26, 118, 2, ST7735_MAGENTA);

    HTFT_vDrawString(10, 45, "XO TIC-TAC-TOE:", ST7735_CYAN, ST7735_BLACK);
    HTFT_vDrawString(15, 60, "WINS: 0  LOSS: 0", ST7735_WHITE, ST7735_BLACK);

    HTFT_vDrawString(10, 85, "PAC-MAN HIGH:", ST7735_CYAN, ST7735_BLACK);
    HTFT_vDrawString(15, 100, "SCORE: 0000 pts", ST7735_WHITE, ST7735_BLACK);

    HTFT_vDrawRect(5, 140, 118, 1, ST7735_DARKGRAY);
    HTFT_vDrawString(18, 145, "PRESS OK TO BACK", ST7735_GREEN, ST7735_BLACK);
}

void DrawParentalControlScreen(void)
{
    HTFT_vDrawRect(5, 5, 118, 2, ST7735_RED);
    HTFT_vDrawString(10, 12, "PARENTAL CONTROL", ST7735_YELLOW, ST7735_BLACK);
    HTFT_vDrawRect(5, 26, 118, 2, ST7735_RED);

    HTFT_vDrawString(12, 50, "PLAYTIME LIMIT:", ST7735_CYAN, ST7735_BLACK);
    HTFT_vDrawString(20, 65, "[ UNLIMITED ]", ST7735_GREEN, ST7735_BLACK);

    HTFT_vDrawString(12, 90, "SYSTEM LOCK:", ST7735_CYAN, ST7735_BLACK);
    HTFT_vDrawString(20, 105, "[ UNLOCKED ]", ST7735_GREEN, ST7735_BLACK);

    HTFT_vDrawRect(5, 140, 118, 1, ST7735_DARKGRAY);
    HTFT_vDrawString(18, 145, "PRESS OK TO BACK", ST7735_GREEN, ST7735_BLACK);
}
