#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../HAL/TFT/TFT_int.h"
#include "../HAL/IR/IR_int.h"

#include "XO_int.h"

#define ST7735_BLACK     0x0000
#define ST7735_WHITE     0xFFFF
#define ST7735_GREEN     0x07E0
#define ST7735_CYAN      0x07FF
#define ST7735_YELLOW    0xFFE0
#define ST7735_RED       0xF800
#define ST7735_MAGENTA   0xF81F
#define ST7735_DARKGRAY  0x39E7

static char Board[3][3];
static u8 CursorX = 0, CursorY = 0;
static char CurrentPlayer = 'X';
static u8 GameActive = 1;

static void DrawXOBoard(void);
static void DrawCell(u8 r, u8 c);
static void DrawStatusBanner(void);
static void CheckWin(void);
static void ProcessNumpadDirectSelection(u8 key);

void XO_vInit(void)
{
    for (u8 r = 0; r < 3; r++)
    {
        for (u8 c = 0; c < 3; c++)
        {
            Board[r][c] = ' ';
        }
    }

    CursorX = 0;
    CursorY = 0;
    CurrentPlayer = 'X';
    GameActive = 1;

    HTFT_vFillBackgroundColor(ST7735_BLACK);
    DrawStatusBanner();
    DrawXOBoard();
}

u8 XO_u8HandleInput(u8 A_u8Key)
{
    if (A_u8Key == IR_NAV_EXIT || A_u8Key == IR_KEY_MODE)
    {
        return XO_STATE_EXIT;
    }

    if (GameActive)
    {
        u8 oldR = CursorY, oldC = CursorX;

        /* Directional Cursor Movement */
        if ((A_u8Key == IR_NAV_UP || A_u8Key == IR_KEY_2) && CursorY > 0) CursorY--;
        else if ((A_u8Key == IR_NAV_DOWN || A_u8Key == IR_KEY_8) && CursorY < 2) CursorY++;
        else if ((A_u8Key == IR_NAV_LEFT || A_u8Key == IR_KEY_4) && CursorX > 0) CursorX--;
        else if ((A_u8Key == IR_NAV_RIGHT || A_u8Key == IR_KEY_6) && CursorX < 2) CursorX++;

        if (oldR != CursorY || oldC != CursorX)
        {
            DrawCell(oldR, oldC);
            DrawCell(CursorY, CursorX);
        }

        /* Confirmation via OK Button */
        if (A_u8Key == IR_NAV_OK && Board[CursorY][CursorX] == ' ')
        {
            Board[CursorY][CursorX] = CurrentPlayer;
            DrawCell(CursorY, CursorX);
            CheckWin();

            if (GameActive)
            {
                CurrentPlayer = (CurrentPlayer == 'X') ? 'O' : 'X';
                DrawStatusBanner();
            }
        }
        /* Direct Numpad Selection (1 to 9) */
        else
        {
            ProcessNumpadDirectSelection(A_u8Key);
        }
    }

    return XO_STATE_CONTINUE;
}

static void DrawStatusBanner(void)
{
    HTFT_vDrawRect(10, 5, 108, 1, ST7735_DARKGRAY);
    if (CurrentPlayer == 'X')
    {
        HTFT_vDrawString(20, 10, "TURN: PLAYER X", ST7735_MAGENTA, ST7735_BLACK);
    }
    else
    {
        HTFT_vDrawString(20, 10, "TURN: PLAYER O", ST7735_CYAN, ST7735_BLACK);
    }
    HTFT_vDrawRect(10, 22, 108, 1, ST7735_DARKGRAY);
}

static void DrawXOBoard(void)
{
    /* Clean grid lines */
    HTFT_vDrawRect(48, 30, 2, 105, ST7735_WHITE);
    HTFT_vDrawRect(78, 30, 2, 105, ST7735_WHITE);

    HTFT_vDrawRect(15, 65, 98, 2, ST7735_WHITE);
    HTFT_vDrawRect(15, 100, 98, 2, ST7735_WHITE);

    for (u8 r = 0; r < 3; r++)
    {
        for (u8 c = 0; c < 3; c++)
        {
            DrawCell(r, c);
        }
    }
}

static void DrawCell(u8 r, u8 c)
{
    u16 x = 18 + c * 32;
    u16 y = 33 + r * 35;

    u16 color = ST7735_WHITE;
    if (Board[r][c] == 'X') color = ST7735_MAGENTA;
    else if (Board[r][c] == 'O') color = ST7735_CYAN;

    char str[2] = {Board[r][c], '\0'};

    /* Show a simple yellow dot if empty and targeted by cursor */
    if (str[0] == ' ')
    {
        if (r == CursorY && c == CursorX)
        {
            str[0] = '.';
            color = ST7735_YELLOW;
        }
    }

    HTFT_vDrawString(x + 10, y + 10, str, color, ST7735_BLACK);
}

static void ProcessNumpadDirectSelection(u8 key)
{
    u8 targetR = 0, targetC = 0, valid = 1;

    switch (key)
    {
        case IR_KEY_1: targetR = 0; targetC = 0; break;
        case IR_KEY_2: targetR = 0; targetC = 1; break;
        case IR_KEY_3: targetR = 0; targetC = 2; break;
        case IR_KEY_4: targetR = 1; targetC = 0; break;
        case IR_KEY_5: targetR = 1; targetC = 1; break;
        case IR_KEY_6: targetR = 1; targetC = 2; break;
        case IR_KEY_7: targetR = 2; targetC = 0; break;
        case IR_KEY_8: targetR = 2; targetC = 1; break;
        case IR_KEY_9: targetR = 2; targetC = 2; break;
        default: valid = 0; break;
    }

    if (valid && Board[targetR][targetC] == ' ')
    {
        u8 oldR = CursorY, oldC = CursorX;
        CursorY = targetR;
        CursorX = targetC;

        DrawCell(oldR, oldC);
        Board[CursorY][CursorX] = CurrentPlayer;
        DrawCell(CursorY, CursorX);

        CheckWin();
        if (GameActive)
        {
            CurrentPlayer = (CurrentPlayer == 'X') ? 'O' : 'X';
            DrawStatusBanner();
        }
    }
}

static void CheckWin(void)
{
    char win = ' ';

    /* Check Rows and Columns */
    for (u8 i = 0; i < 3; i++)
    {
        if (Board[i][0] != ' ' && Board[i][0] == Board[i][1] && Board[i][1] == Board[i][2]) win = Board[i][0];
        if (Board[0][i] != ' ' && Board[0][i] == Board[1][i] && Board[1][i] == Board[2][i]) win = Board[0][i];
    }

    /* Check Diagonals */
    if (Board[0][0] != ' ' && Board[0][0] == Board[1][1] && Board[1][1] == Board[2][2]) win = Board[0][0];
    if (Board[0][2] != ' ' && Board[0][2] == Board[1][1] && Board[1][1] == Board[2][0]) win = Board[0][2];

    if (win != ' ')
    {
        GameActive = 0;
        HTFT_vDrawRect(10, 5, 108, 18, ST7735_BLACK);
        if (win == 'X')
            HTFT_vDrawString(20, 10, "PLAYER X WINS!", ST7735_GREEN, ST7735_BLACK);
        else
            HTFT_vDrawString(20, 10, "PLAYER O WINS!", ST7735_GREEN, ST7735_BLACK);
        return;
    }

    /* Check Draw / Tie */
    u8 fullCells = 0;
    for (u8 r = 0; r < 3; r++)
    {
        for (u8 c = 0; c < 3; c++)
        {
            if (Board[r][c] != ' ') fullCells++;
        }
    }

    if (fullCells == 9)
    {
        GameActive = 0;
        HTFT_vDrawRect(10, 5, 108, 18, ST7735_BLACK);
        HTFT_vDrawString(25, 10, "IT'S A DRAW!", ST7735_YELLOW, ST7735_BLACK);
    }
}
