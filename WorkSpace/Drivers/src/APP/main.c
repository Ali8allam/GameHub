#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"
#include "../MCAL/SYSTICK/SYSTICK_int.h"
#include "../MCAL/SPI/SPI_int.h"

#include "../HAL/TFT/TFT_int.h"
#include "../HAL/IR/IR_int.h"

#include "XO_int.h"

#include "SNAKE_int.h"

#include "PACMAN_int.h"

#define ST7735_BLACK   0x0000
#define ST7735_WHITE   0xFFFF
#define ST7735_GREEN   0x07E0
#define ST7735_YELLOW  0xFFE0

typedef enum {
    STATE_MAIN_MENU,
    STATE_XO_GAME,
	STATE_SNAKE_GAME,
	STATE_PACMAN_GAME
} SystemState_t;

static SystemState_t CurrentState = STATE_MAIN_MENU;
static u8 MenuSelection = 0;

void DrawMenu(void);

static void Software_vDelayMs(u32 A_u32DelayMs)
{
    for (volatile u32 i = 0; i < (A_u32DelayMs * 4000); i++)
    {
        __asm__("NOP");
    }
}

int main(void)
{
    /* 1. Clocks Initialization */
    MRCC_vInit();
    MRCC_vEnableCLK(RCC_AHB1, GPIO_PORTA);
    MRCC_vEnableCLK(RCC_APB2, 14); // SYSCFG Clock
    MRCC_vEnableCLK(RCC_APB2, 12); // SPI1 Clock

    /* 2. SPI Pins Configuration for TFT */
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
                        /* 2. Increased max menu limit to 2 */
                        if (MenuSelection < 2) MenuSelection++;
                        DrawMenu();
                    }
                    else if (Key == IR_NAV_OK || Key == IR_KEY_1 || Key == IR_KEY_5)
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
                        /* 3. Added Pacman Initialization Trigger */
                        else if (MenuSelection == 2)
                        {
                            CurrentState = STATE_PACMAN_GAME;
                            HPACMAN_vInit();
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
        }
    }

    void DrawMenu(void)
    {
        HTFT_vDrawString(20, 20, "ARCADE MENU", ST7735_YELLOW, ST7735_BLACK);

        if (MenuSelection == 0)
            HTFT_vDrawString(15, 60, "> 1. XO GAME", ST7735_GREEN, ST7735_BLACK);
        else
            HTFT_vDrawString(15, 60, "  1. XO GAME", ST7735_WHITE, ST7735_BLACK);

        if (MenuSelection == 1)
            HTFT_vDrawString(15, 80, "> 2. SNAKE GAME", ST7735_GREEN, ST7735_BLACK);
        else
            HTFT_vDrawString(15, 80, "  2. SNAKE GAME", ST7735_WHITE, ST7735_BLACK);

        /* 4. Added Pacman to Menu Display */
        if (MenuSelection == 2)
            HTFT_vDrawString(15, 100, "> 3. PACMAN GAME", ST7735_GREEN, ST7735_BLACK);
        else
            HTFT_vDrawString(15, 100, "  3. PACMAN GAME", ST7735_WHITE, ST7735_BLACK);
    }
