#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../HAL/TFT/TFT_int.h"
#include "../HAL/IR/IR_int.h"

#include "PACMAN_int.h"

#define ST7735_BLACK   0x0000
#define ST7735_BLUE    0x001F
#define ST7735_YELLOW  0xFFE0
#define ST7735_RED     0xF800
#define ST7735_WHITE   0xFFFF

#define MAP_SIZE       13
#define BLOCK_SIZE     8
#define OFFSET_X       12
#define OFFSET_Y       30

/* 1 = Wall, 0 = Dot, 2 = Empty */
static const u8 MapTemplate[MAP_SIZE][MAP_SIZE] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,1,1,0,1,1,1,1,1,0,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static u8 Map[MAP_SIZE][MAP_SIZE];
static u8 PacX, PacY;
static u8 GhostX, GhostY;
static u8 DotsLeft;
static u16 Score;
static u8 GameOver;
static u8 GhostTick;

static void DrawBlock(u8 x, u8 y, u16 color);
static void DrawDot(u8 x, u8 y);
static void DrawScoreBoard(void);
static s16  AbsVal(s16 val);

void HPACMAN_vInit(void)
{
    HTFT_vFillBackgroundColor(ST7735_BLACK);

    Score = 0;
    DotsLeft = 0;
    GameOver = 0;
    GhostTick = 0;

    PacX = 1; PacY = 1;
    GhostX = 11; GhostY = 11;

    for (u8 y = 0; y < MAP_SIZE; y++)
    {
        for (u8 x = 0; x < MAP_SIZE; x++)
        {
            Map[y][x] = MapTemplate[y][x];

            if (Map[y][x] == 1)
            {
                DrawBlock(x, y, ST7735_BLUE);
            }
            else if (Map[y][x] == 0)
            {
                DrawDot(x, y);
                DotsLeft++;
            }
        }
    }

    /* Remove the dot under Pacman's start position */
    Map[PacY][PacX] = 2;
    DotsLeft--;

    DrawScoreBoard();
    DrawBlock(PacX, PacY, ST7735_YELLOW);
    DrawBlock(GhostX, GhostY, ST7735_RED);
}

u8 HPACMAN_u8Update(u8 A_u8Key)
{
    if (A_u8Key != IR_KEY_NONE)
    {
        if (A_u8Key == IR_NAV_EXIT || A_u8Key == IR_KEY_MODE)
        {
            return PACMAN_STATE_EXIT;
        }

        if (GameOver && (A_u8Key == IR_NAV_OK || A_u8Key == IR_KEY_1 || A_u8Key == IR_KEY_5))
        {
            HPACMAN_vInit();
            return PACMAN_STATE_CONTINUE;
        }
    }

    if (GameOver) return PACMAN_STATE_CONTINUE;

    /* --- PAC-MAN MOVEMENT (WASD Layout) --- */
    u8 NextX = PacX;
    u8 NextY = PacY;

    if (A_u8Key == IR_KEY_2 || A_u8Key == IR_NAV_UP)         NextY--;
    else if (A_u8Key == IR_KEY_5 || A_u8Key == IR_NAV_DOWN)  NextY++;
    else if (A_u8Key == IR_KEY_4 || A_u8Key == IR_NAV_LEFT)  NextX--;
    else if (A_u8Key == IR_KEY_6 || A_u8Key == IR_NAV_RIGHT) NextX++;

    if (Map[NextY][NextX] != 1)
    {
        /* Erase old Pacman */
        DrawBlock(PacX, PacY, ST7735_BLACK);

        PacX = NextX;
        PacY = NextY;

        /* Eat Dot */
        if (Map[PacY][PacX] == 0)
        {
            Map[PacY][PacX] = 2; // Mark empty
            DotsLeft--;
            Score += 10;
            DrawScoreBoard();
        }
        DrawBlock(PacX, PacY, ST7735_YELLOW);
    }

    /* Win Condition */
    if (DotsLeft == 0)
    {
        GameOver = 1;
        HTFT_vDrawString(40, 140, "YOU WIN!", ST7735_YELLOW, ST7735_BLACK);
        return PACMAN_STATE_CONTINUE;
    }

    /* --- GHOST AI (Moves every other tick so it's fair) --- */
    GhostTick++;
    if (GhostTick % 2 == 0)
    {
        /* Erase old Ghost */
        DrawBlock(GhostX, GhostY, ST7735_BLACK);
        if (Map[GhostY][GhostX] == 0) DrawDot(GhostX, GhostY); // Redraw dot if it walked over one

        u8 moved = 0;

        /* Prioritize axis with greatest distance */
        if (AbsVal(PacX - GhostX) > AbsVal(PacY - GhostY))
        {
            if (PacX > GhostX && Map[GhostY][GhostX + 1] != 1) { GhostX++; moved = 1; }
            else if (PacX < GhostX && Map[GhostY][GhostX - 1] != 1) { GhostX--; moved = 1; }

            if (!moved) {
                if (PacY > GhostY && Map[GhostY + 1][GhostX] != 1) { GhostY++; }
                else if (PacY < GhostY && Map[GhostY - 1][GhostX] != 1) { GhostY--; }
            }
        }
        else
        {
            if (PacY > GhostY && Map[GhostY + 1][GhostX] != 1) { GhostY++; moved = 1; }
            else if (PacY < GhostY && Map[GhostY - 1][GhostX] != 1) { GhostY--; moved = 1; }

            if (!moved) {
                if (PacX > GhostX && Map[GhostY][GhostX + 1] != 1) { GhostX++; }
                else if (PacX < GhostX && Map[GhostY][GhostX - 1] != 1) { GhostX--; }
            }
        }

        DrawBlock(GhostX, GhostY, ST7735_RED);
    }

    /* Lose Condition */
    if (PacX == GhostX && PacY == GhostY)
    {
        GameOver = 1;
        DrawBlock(PacX, PacY, ST7735_RED); // Ghost eats Pacman
        HTFT_vDrawString(30, 140, "GAME OVER!", ST7735_RED, ST7735_BLACK);
    }

    return PACMAN_STATE_CONTINUE;
}

static void DrawBlock(u8 x, u8 y, u16 color)
{
    u16 px = OFFSET_X + (x * BLOCK_SIZE);
    u16 py = OFFSET_Y + (y * BLOCK_SIZE);
    HTFT_vDrawRect(px, py, BLOCK_SIZE - 1, BLOCK_SIZE - 1, color);
}

static void DrawDot(u8 x, u8 y)
{
    u16 px = OFFSET_X + (x * BLOCK_SIZE) + 3;
    u16 py = OFFSET_Y + (y * BLOCK_SIZE) + 3;
    HTFT_vDrawRect(px, py, 2, 2, ST7735_WHITE);
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

static s16 AbsVal(s16 val)
{
    return (val < 0) ? -val : val;
}
