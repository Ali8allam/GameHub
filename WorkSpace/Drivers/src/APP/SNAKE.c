#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/SYSTICK/SYSTICK_int.h"
#include "../HAL/TFT/TFT_int.h"
#include "../HAL/IR/IR_int.h"

#include "SNAKE_int.h"



#define ST7735_BLACK   0x0000
#define ST7735_WHITE   0xFFFF
#define ST7735_GREEN   0x07E0
#define ST7735_RED     0xF800
#define ST7735_YELLOW  0xFFE0
#define ST7735_CYAN    0x07FF

#define GRID_WIDTH     20
#define GRID_HEIGHT    20
#define BLOCK_SIZE     5
#define OFFSET_X       14
#define OFFSET_Y       30

#define MAX_SNAKE_LEN  100

extern void System_vUpdateSnakeScore(u16 A_u16Score);

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction_t;

typedef struct {
    u8 x;
    u8 y;
} Point_t;

static Point_t Snake[MAX_SNAKE_LEN];
static u8 SnakeLength = 3;
static Direction_t CurrentDir = DIR_RIGHT;
static Point_t Food;
static u16 Score = 0;
static u8 GameOver = 0;
static u32 RandomSeed = 12345;

static void DrawBlock(u8 x, u8 y, u16 color);
static void GenerateFood(void);
static void DrawScoreBoard(void);

static u16 GetPseudoRandom(void)
{
    RandomSeed = RandomSeed * 1103515245 + 12345;
    return (u16)(RandomSeed / 65536) % 32768;
}

void HSNAKE_vInit(void)
{
    SnakeLength = 3;
    CurrentDir = DIR_RIGHT;
    Score = 0;
    GameOver = 0;

    Snake[0].x = 10; Snake[0].y = 10; // Head
    Snake[1].x = 9;  Snake[1].y = 10;
    Snake[2].x = 8;  Snake[2].y = 10; // Tail

    HTFT_vFillBackgroundColor(ST7735_BLACK);

    /* Draw Play Boundary */
    HTFT_vDrawRect(OFFSET_X - 2, OFFSET_Y - 2, (GRID_WIDTH * BLOCK_SIZE) + 4, 2, ST7735_CYAN);
    HTFT_vDrawRect(OFFSET_X - 2, OFFSET_Y + (GRID_HEIGHT * BLOCK_SIZE), (GRID_WIDTH * BLOCK_SIZE) + 4, 2, ST7735_CYAN);
    HTFT_vDrawRect(OFFSET_X - 2, OFFSET_Y - 2, 2, (GRID_HEIGHT * BLOCK_SIZE) + 4, ST7735_CYAN);
    HTFT_vDrawRect(OFFSET_X + (GRID_WIDTH * BLOCK_SIZE), OFFSET_Y - 2, 2, (GRID_HEIGHT * BLOCK_SIZE) + 4, ST7735_CYAN);

    DrawScoreBoard();

    for (u8 i = 0; i < SnakeLength; i++)
    {
        DrawBlock(Snake[i].x, Snake[i].y, ST7735_GREEN);
    }

    GenerateFood();
}

u8 HSNAKE_u8Update(u8 A_u8Key)
{
    RandomSeed += A_u8Key;

    /* Process valid key inputs only when a button is pressed */
    if (A_u8Key != IR_KEY_NONE)
        {
            if (A_u8Key == IR_NAV_EXIT || A_u8Key == IR_KEY_MODE)
            {
                return SNAKE_STATE_EXIT;
            }

            /* WASD Numpad Mapping: 2 = UP, 4 = LEFT, 5 = DOWN, 6 = RIGHT */
            if ((A_u8Key == IR_KEY_2 || A_u8Key == IR_NAV_UP) && CurrentDir != DIR_DOWN)
            {
                CurrentDir = DIR_UP;
            }
            else if ((A_u8Key == IR_KEY_5 || A_u8Key == IR_KEY_8 || A_u8Key == IR_NAV_DOWN) && CurrentDir != DIR_UP)
            {
                CurrentDir = DIR_DOWN;
            }
            else if ((A_u8Key == IR_KEY_4 || A_u8Key == IR_NAV_LEFT) && CurrentDir != DIR_RIGHT)
            {
                CurrentDir = DIR_LEFT;
            }
            else if ((A_u8Key == IR_KEY_6 || A_u8Key == IR_NAV_RIGHT) && CurrentDir != DIR_LEFT)
            {
                CurrentDir = DIR_RIGHT;
            }

            /* Restart game on Game Over */
            if (GameOver && (A_u8Key == IR_NAV_OK || A_u8Key == IR_KEY_1))
            {
                HSNAKE_vInit();
                return SNAKE_STATE_CONTINUE;
            }
        }

    if (GameOver)
    {
        return SNAKE_STATE_CONTINUE;
    }

    /* AUTOMATIC MOVEMENT STEP (Runs on every frame tick) */
    Point_t newHead = Snake[0];
    switch (CurrentDir)
    {
        case DIR_UP:    newHead.y--; break;
        case DIR_DOWN:  newHead.y++; break;
        case DIR_LEFT:  newHead.x--; break;
        case DIR_RIGHT: newHead.x++; break;
    }

    /* Wall Collision Check */
    if (newHead.x >= GRID_WIDTH || newHead.y >= GRID_HEIGHT)
    {
        GameOver = 1;
        HTFT_vDrawString(22, 140, "GAME OVER!", ST7735_RED, ST7735_BLACK);
        System_vUpdateSnakeScore(Score);
        return SNAKE_STATE_CONTINUE;
    }

    /* Self Collision Check */
    for (u8 i = 0; i < SnakeLength; i++)
    {
        if (Snake[i].x == newHead.x && Snake[i].y == newHead.y)
        {
            GameOver = 1;
            HTFT_vDrawString(22, 140, "GAME OVER!", ST7735_RED, ST7735_BLACK);
            System_vUpdateSnakeScore(Score);
            return SNAKE_STATE_CONTINUE;
        }
    }

    /* Food Collision & Movement */
    if (newHead.x == Food.x && newHead.y == Food.y)
    {
        if (SnakeLength < MAX_SNAKE_LEN)
        {
            SnakeLength++;
        }
        Score += 10;
        DrawScoreBoard();
        GenerateFood();
    }
    else
    {
        DrawBlock(Snake[SnakeLength - 1].x, Snake[SnakeLength - 1].y, ST7735_BLACK);
    }

    for (u8 i = SnakeLength - 1; i > 0; i--)
    {
        Snake[i] = Snake[i - 1];
    }
    Snake[0] = newHead;

    DrawBlock(Snake[0].x, Snake[0].y, ST7735_GREEN);

    return SNAKE_STATE_CONTINUE;
}

static void DrawBlock(u8 x, u8 y, u16 color)
{
    u16 px = OFFSET_X + (x * BLOCK_SIZE);
    u16 py = OFFSET_Y + (y * BLOCK_SIZE);
    HTFT_vDrawRect(px, py, BLOCK_SIZE - 1, BLOCK_SIZE - 1, color);
}

static void GenerateFood(void)
{
    u8 valid = 0;
    while (!valid)
    {
        valid = 1;
        Food.x = GetPseudoRandom() % GRID_WIDTH;
        Food.y = GetPseudoRandom() % GRID_HEIGHT;

        for (u8 i = 0; i < SnakeLength; i++)
        {
            if (Snake[i].x == Food.x && Snake[i].y == Food.y)
            {
                valid = 0;
                break;
            }
        }
    }
    DrawBlock(Food.x, Food.y, ST7735_RED);
}

static void DrawScoreBoard(void)
{
    char scoreStr[10] = "SCORE: ";
    u16 temp = Score;

    scoreStr[7] = '0' + (temp / 100);
    scoreStr[8] = '0' + ((temp / 10) % 10);
    scoreStr[9] = '0' + (temp % 10);

    HTFT_vDrawString(15, 10, scoreStr, ST7735_YELLOW, ST7735_BLACK);
}
