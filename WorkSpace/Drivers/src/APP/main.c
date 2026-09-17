#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"
#include "../MCAL/SYSTICK/SYSTICK_int.h"
#include "../MCAL/SPI/SPI_int.h"

#include "../HAL/TFT/TFT_int.h"
#include "../HAL/IR/IR_int.h"

#include "XO_int.h"
#include "PACMAN_int.h"

#define ST7735_BLACK   0x0000
#define ST7735_WHITE   0xFFFF
#define ST7735_GREEN   0x07E0
#define ST7735_YELLOW  0xFFE0

typedef enum {
    STATE_MAIN_MENU,
    STATE_XO_GAME,
    STATE_PACMAN_GAME
} SystemState_t;

static SystemState_t CurrentState = STATE_MAIN_MENU;
static u8 MenuSelection = 0;

void DrawMenu(void);
static void ReturnToMenu(void);

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
        .AltFunc = GPIO_AF5
    };
    MGPIO_vPinInit(&MOSI);

    GPIOx_PinConfig_t SCK = {
        .Port    = GPIO_PORTA,
        .Pin     = GPIO_PIN5,
        .Mode    = GPIO_MODE_ALF,
        .AltFunc = GPIO_AF5
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

        if (Key != IR_KEY_NONE)
        {
            if (CurrentState == STATE_MAIN_MENU)
            {
                if (Key == IR_NAV_UP || Key == IR_NAV_DOWN || Key == IR_KEY_2 || Key == IR_KEY_8)
                {
                    MenuSelection ^= 1;
                    DrawMenu();
                }
                else if (Key == IR_NAV_OK || Key == IR_KEY_1 || Key == IR_KEY_5)
                {
                    if (MenuSelection == 0)
                    {
                        CurrentState = STATE_XO_GAME;
                        XO_vInit();
                    }
                    else
                    {
                        CurrentState = STATE_PACMAN_GAME;
                        PACMAN_vInit();
                    }
                }
            }
            else if (CurrentState == STATE_XO_GAME)
            {
                u8 gameStatus = XO_u8HandleInput(Key);

                if (gameStatus == XO_STATE_EXIT)
                {
                    ReturnToMenu();
                }
            }
            else if (CurrentState == STATE_PACMAN_GAME)
            {
                u8 gameStatus = PACMAN_u8HandleInput(Key);

                if (gameStatus == PACMAN_STATE_EXIT)
                {
                    ReturnToMenu();
                }
            }
        }

        /* Pac-Man is not purely key driven: it needs a tick every pass to move
         * the ghosts and repaint. The call also paces the loop at ~50 ms, and
         * only ever runs while Pac-Man is the active state, so XO and the menu
         * behave exactly as before. */
        if (CurrentState == STATE_PACMAN_GAME)
        {
            PACMAN_vUpdate();
        }
    }
}

static void ReturnToMenu(void)
{
    CurrentState = STATE_MAIN_MENU;
    HTFT_vFillBackgroundColor(ST7735_BLACK);
    DrawMenu();
}

void DrawMenu(void)
{
    HTFT_vDrawString(20, 20, "ARCADE MENU", ST7735_YELLOW, ST7735_BLACK);

    if (MenuSelection == 0)
        HTFT_vDrawString(15, 60, "> 1. XO GAME", ST7735_GREEN, ST7735_BLACK);
    else
        HTFT_vDrawString(15, 60, "  1. XO GAME", ST7735_WHITE, ST7735_BLACK);

    if (MenuSelection == 1)
        HTFT_vDrawString(15, 80, "> 2. PAC-MAN", ST7735_GREEN, ST7735_BLACK);
    else
        HTFT_vDrawString(15, 80, "  2. PAC-MAN", ST7735_WHITE, ST7735_BLACK);
}
