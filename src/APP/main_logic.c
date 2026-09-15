/*
 * main_logic.c
 *
 *  Created on: Sep 13, 2026
 *      Author: ALI & ADHM
 */

#include <stdint.h>
#include <stdbool.h>

#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

// MCAL Includes
#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"
#include "../MCAL/SPI/SPI_int.h"
#include "../MCAL/SYSTICK/SYSTICK_int.h"
#include "../MCAL/UART/UART_int.h"

// HAL Includes
//#include "../HAL/TFT/TFT_image.h"
#include "../HAL/TFT/TFT_int.h"

// ==========================================
// 1. DEFINITIONS & CONFIGURATION
// ==========================================
#define GRID_SIZE        8    // 8x8 pixel blocks
#define GRID_WIDTH       16   // 128 / 8 = 16 columns (0 to 15)
#define GRID_HEIGHT      20   // 160 / 8 = 20 rows (0 to 19)
#define MAX_LEN          50   // Maximum snake segments

#define TFT_WIDTH   128
#define TFT_HEIGHT  160
#define BOARD_SIZE  5
#define START_SCREEN_DELAY_MS 3000

// 16-Bit RGB565 Colors
#define COLOR_BG         0x0000 // Black background
#define COLOR_SNAKE      0x07E0 // Green snake
#define COLOR_HEAD       0x03E0 // Darker green head
#define COLOR_FOOD       0xF800 // Red food
#define COLOR_WHITE     0xFFFF

#define BUTTON_TURN_LEFT  GPIO_PIN8
#define BUTTON_TURN_RIGHT GPIO_PIN9
#define BUTTON_PAUSE      GPIO_PIN10

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction_t;

typedef struct {
    int8_t x;
    int8_t y;
} Position;

// ==========================================
// 2. GLOBAL GAME VARIABLES
// ==========================================
Position snake[MAX_LEN];
uint8_t length = 3;
Position food;

u8 game_over = 0;
u8 game_paused = 0;
volatile u8 game_started = 0;
volatile Direction_t current_direction = DIR_RIGHT;

void Process_Button_Input(void);
void Spawn_Food(void);

// Pseudo-Random Generator Seed
static uint32_t rand_seed = 12345;

static uint32_t Get_Random(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (uint32_t)(rand_seed / 65536) % 32768;
}

// ==========================================
// 4. HARDWARE WRAPPERS & PWM AUDIO (TIM3)
// ==========================================

void TFT_DrawBlock(u8 grid_x, u8 grid_y, u16 color) {
    u16 x_start = grid_x * GRID_SIZE;
    u16 x_end   = x_start + (GRID_SIZE - 1);

    u16 y_start = grid_y * GRID_SIZE;
    u16 y_end   = y_start + (GRID_SIZE - 1);

    HTFT_vSetXPos(x_start, x_end);
    HTFT_vSetYPos(y_start, y_end);
    HTFT_FillRectangle(color);
}

void PWM_Audio_Init(void) {
    MRCC_vEnableClk(RCC_APB1, 1);
    GPIOx_PinConfig_t PWM_Pin = {
        .Port = GPIO_PORTA, .Pin = GPIO_PIN6,
        .Mode = GPIO_MODE_ALF, .OutputType = GPIO_OT_PUSHPULL, .Speed = GPIO_SPEED_HIGH
    };
    MGPIO_vPinInit(&PWM_Pin);

    *((volatile uint32_t*)(0x40000400 + 0x2C)) = 80 - 1;
    *((volatile uint32_t*)(0x40000400 + 0x18)) = 6;
    *((volatile uint32_t*)(0x40000400 + 0x20)) |= (1 << 0);
    *((volatile uint32_t*)(0x40000400 + 0x00)) |= (1 << 0);
}

void PWM_PlayTone(uint16_t freq_hz, uint16_t duration_ms) {
    if (freq_hz == 0) return;
    uint32_t arr = 100000 / freq_hz;
    *((volatile uint32_t*)(0x40000400 + 0x2E)) = arr;
    *((volatile uint32_t*)(0x40000400 + 0x34)) = arr / 2;

    for(volatile uint32_t i = 0; i < (duration_ms * 1200); i++);

    *((volatile uint32_t*)(0x40000400 + 0x34)) = 0;
}

// Referee Full-Time Whistle
void Sound_Whistle(void) {
    PWM_PlayTone(2800, 150);
    for(volatile uint32_t i = 0; i < 150000; i++);
    PWM_PlayTone(2800, 150);
    for(volatile uint32_t i = 0; i < 150000; i++);
    PWM_PlayTone(2800, 700);
}

void Game_Init(void) {
    length = 3;
    current_direction = DIR_RIGHT;
    game_over = 0;
    game_paused = 0;
    game_started = 0; // Wait for button press to begin

    snake[0].x = 3; snake[0].y = 10;
    snake[1].x = 2; snake[1].y = 10;
    snake[2].x = 1; snake[2].y = 10;

    HTFT_FillBackground(COLOR_BG);

    for (int i = 0; i < length; i++) {
        TFT_DrawBlock(snake[i].x, snake[i].y, (i == 0) ? COLOR_HEAD : COLOR_SNAKE);
    }
    Spawn_Food();
}
    void Game_ShowWelcomeScreen(void) {
    HTFT_FillBackground(COLOR_BG);

    // Draw Header Box
    HTFT_vSetXPos(10, 117);
    HTFT_vSetYPos(10, 30);
    HTFT_FillRectangle(COLOR_HEAD);

    // Render Text Instructions
    HTFT_vPrintString(25, 15, " SNAKE GAME", COLOR_WHITE, COLOR_HEAD);
    HTFT_vPrintString(30, 45, "CONTROLS:", COLOR_SNAKE, COLOR_BG);
    HTFT_vPrintString(10, 60, "UP FOR CLKWISE", COLOR_WHITE, COLOR_BG);
    HTFT_vPrintString(10, 72, "DOWN FOR ANTICLKWISE", COLOR_WHITE, COLOR_BG);

    HTFT_vPrintString(30, 95, "RULES:", COLOR_FOOD, COLOR_BG);
    HTFT_vPrintString(10, 110, "EAT FOOD TO GROW", COLOR_WHITE, COLOR_BG);
    HTFT_vPrintString(10, 122, "AVOID WALLS", COLOR_WHITE, COLOR_BG);

    HTFT_vPrintString(10, 135, "PRESS ANY KEY ", COLOR_HEAD, COLOR_BG);
    HTFT_vPrintString(22, 145, "TO START", COLOR_HEAD, COLOR_BG);


    }

void Game_UpdateAndRender(void) {
    if (!game_started || game_over || game_paused) return;

    Position new_head = snake[0];
    if      (current_direction == DIR_UP)    new_head.y -= 1;
    else if (current_direction == DIR_DOWN)  new_head.y += 1;
    else if (current_direction == DIR_LEFT)  new_head.x -= 1;
    else if (current_direction == DIR_RIGHT) new_head.x += 1;

    // Wall Collision
    if (new_head.x < 0 || new_head.x >= GRID_WIDTH || new_head.y < 0 || new_head.y >= GRID_HEIGHT) {
        game_over = 1;
        Sound_Whistle();
        return;
    }

    // Self Collision
    for (int i = 0; i < length; i++) {
        if (new_head.x == snake[i].x && new_head.y == snake[i].y) {
            game_over = 1;
            Sound_Whistle();
            return;
        }
    }

    // Food Check
    u8 ate_food = 0;
    if (new_head.x == food.x && new_head.y == food.y) {
        ate_food = 1;
        if (length < MAX_LEN) length++;
        Spawn_Food();
    }

    if (!ate_food) {
        Position old_tail = snake[length - 1];
        TFT_DrawBlock(old_tail.x, old_tail.y, COLOR_BG);
    }

    for (int i = length - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }

    snake[0] = new_head;
    TFT_DrawBlock(snake[1].x, snake[1].y, COLOR_SNAKE);
    TFT_DrawBlock(new_head.x, new_head.y, COLOR_HEAD);
}

// ==========================================
// 5. GAME ENGINE & BUTTON INPUT
// ==========================================

void Spawn_Food(void) {
    food.x = Get_Random() % GRID_WIDTH;
    food.y = Get_Random() % GRID_HEIGHT;
    TFT_DrawBlock(food.x, food.y, COLOR_FOOD);
}

void Process_Button_Input(void)
{
    static u8 left_was_pressed = 0;
    static u8 right_was_pressed = 0;
    static u8 pause_was_pressed = 0;
    u8 left_is_pressed = (MGPIO_u8GetPinValue(GPIO_PORTA, BUTTON_TURN_LEFT) == GPIO_LOW);
    u8 right_is_pressed = (MGPIO_u8GetPinValue(GPIO_PORTA, BUTTON_TURN_RIGHT) == GPIO_LOW);
    u8 pause_is_pressed = (MGPIO_u8GetPinValue(GPIO_PORTA, BUTTON_PAUSE) == GPIO_LOW);

    if(pause_is_pressed && !pause_was_pressed && game_started && !game_over)
    {
        game_paused = !game_paused;
    }

    if(left_is_pressed && !left_was_pressed && !game_paused)
    {
        if(game_over)
        {
            Game_Init();
        }
        if(current_direction == DIR_UP) current_direction = DIR_LEFT;
        else if(current_direction == DIR_LEFT) current_direction = DIR_DOWN;
        else if(current_direction == DIR_DOWN) current_direction = DIR_RIGHT;
        else current_direction = DIR_UP;
        game_started = 1;
    }

    if(right_is_pressed && !right_was_pressed && !game_paused)
    {
        if(game_over)
        {
            Game_Init();
        }
        if(current_direction == DIR_UP) current_direction = DIR_RIGHT;
        else if(current_direction == DIR_RIGHT) current_direction = DIR_DOWN;
        else if(current_direction == DIR_DOWN) current_direction = DIR_LEFT;
        else current_direction = DIR_UP;
        game_started = 1;
    }

    left_was_pressed = left_is_pressed;
    right_was_pressed = right_is_pressed;
    pause_was_pressed = pause_is_pressed;
}

void Game_SoftwareDelay(void) {
    for(volatile uint32_t i = 0; i < 150000; i++); // Calibrated for 8 MHz HSI clock
}

// ==========================================
// 7. MAIN FUNCTION
// ==========================================

int main(void) {

    MRCC_vInit();
    MRCC_vEnableClk(RCC_APB2, RCC_GPIOA);
    MRCC_vEnableClk(RCC_APB2, RCC_GPIOC);
    MRCC_vEnableClk(RCC_APB2, 0);  // AFIO
    MRCC_vEnableClk(RCC_APB2, 12); // SPI1

    GPIOx_PinConfig_t MOSI = {
        .Port = GPIO_PORTA, .Pin = GPIO_PIN7,
        .Mode = GPIO_MODE_ALF, .OutputType = GPIO_OT_PUSHPULL, .PullType = GPIO_NO_PULL
    };
    MGPIO_vPinInit(&MOSI);

    GPIOx_PinConfig_t SCK = {
        .Port = GPIO_PORTA, .Pin = GPIO_PIN5,
        .Mode = GPIO_MODE_ALF, .OutputType = GPIO_OT_PUSHPULL, .PullType = GPIO_NO_PULL
    };
    MGPIO_vPinInit(&SCK);

    // TFT INTIALLIZED TO BE CLEARED AND READY
    HTFT_vInit();
    Game_ShowWelcomeScreen();
    MSYSTICK_vSetDelay_ms(3000);

    GPIOx_PinConfig_t Left_Button = {
        .Port = GPIO_PORTA, .Pin = BUTTON_TURN_LEFT,
        .Mode = GPIO_MODE_INPUT, .PullType = GPIO_PULL_UP
    };
    MGPIO_vPinInit(&Left_Button);

    GPIOx_PinConfig_t Right_Button = {
        .Port = GPIO_PORTA, .Pin = BUTTON_TURN_RIGHT,
        .Mode = GPIO_MODE_INPUT, .PullType = GPIO_PULL_UP
    };
    MGPIO_vPinInit(&Right_Button);

    GPIOx_PinConfig_t Pause_Button = {
        .Port = GPIO_PORTA, .Pin = BUTTON_PAUSE,
        .Mode = GPIO_MODE_INPUT, .PullType = GPIO_PULL_UP
    };
    MGPIO_vPinInit(&Pause_Button);

    PWM_Audio_Init();

    Game_Init();

    while (1) {
        Process_Button_Input();
        Game_UpdateAndRender();
        Game_SoftwareDelay();
    }
}

