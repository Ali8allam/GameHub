/*
 * PACMAN_prg.c
 *
 * Pac-Man front end: TFT rendering, IR input and DAC audio.
 *
 * Ported from the standalone prototype in pacman/src/APP/pacman_tft.c. The
 * gameplay, the HUD layout and the sound cues are unchanged; what differs is:
 *
 *   - it draws through this project's HAL/TFT driver instead of the
 *     prototype's own ST7735 driver,
 *   - its blocking while(1) main loop is split into the init / handle-input /
 *     update entry points the arcade menu in main.c drives, so the game no
 *     longer owns the CPU and other games are unaffected,
 *   - the Pac-Man specific sound cues live here rather than inside the DAC
 *     driver, which stays a generic 8-bit ladder driver.
 *
 * Timing: one tick is 50 ms, and that delay comes from streaming exactly one
 * tick's worth of audio samples at the end of PACMAN_vUpdate() - exactly as
 * the prototype did. SysTick is not used here; it belongs to the IR receiver.
 */

#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"

#include "../HAL/TFT/TFT_int.h"
#include "../HAL/IR/IR_int.h"
#include "../HAL/DAC/DAC_int.h"
#include "../HAL/DAC/DAC_cfg.h"

#include "PACMAN_int.h"
#include "PACMAN_game_int.h"
#include "PACMAN_audio.h"

/* ------------------------------------------------------------------------ */
/* Display layout                                                            */
/* ------------------------------------------------------------------------ */

#define PACMAN_BLACK            0x0000
#define PACMAN_WALL_BLUE        0xB7FF
#define PACMAN_FRIGHTENED_BLUE  0x001F
#define PACMAN_YELLOW           0xFFE0
#define PACMAN_WHITE            0xFFFF
#define PACMAN_RED              0xF800
#define PACMAN_CYAN             0x07FF
#define PACMAN_MAGENTA          0xF81F
#define PACMAN_ORANGE           0xFD20
#define PACMAN_GREEN            0x07E0

#define SCREEN_WIDTH            128U

/* 28 x 31 tiles of 4 px = 112 x 124 px, centred horizontally, top aligned. */
#define CELL                    4U
#define BOARD_X                 8U
#define BOARD_Y                 0U

/* HUD occupies the strip under the maze. */
#define HUD_DIVIDER_Y           125U
#define HUD_SCORE_Y             130U
#define HUD_LIVES_Y             144U
#define HUD_LABEL_X             4U
#define HUD_VALUE_X             44U
#define HUD_SCORE_FIELD_W       80U
#define HUD_LIFE_SPACING        10U
#define HUD_STATUS_X            78U
#define HUD_STATUS_FIELD_W      36U
#define HUD_TEXT_H              7U

/* HUD status field contents. */
#define HUD_STATUS_NONE         0U
#define HUD_STATUS_GAME_OVER    1U
#define HUD_STATUS_WIN          2U

/* ------------------------------------------------------------------------ */
/* Audio                                                                     */
/* ------------------------------------------------------------------------ */

/* The clips in PACMAN_audio.h must be encoded at the rate the DAC paces. */
typedef char pacman_audio_rate_check
    [(HDAC_SAMPLE_RATE_HZ == AUDIO_SAMPLE_RATE) ? 1 : -1];

/* One 50 ms tick holds this many samples, which is what makes streaming the
 * looping effects double as the tick's delay. */
#define SAMPLES_PER_TICK        ((u16)(AUDIO_SAMPLE_RATE / 20U))

/* DAC data pins, ordered DAC bit 0 (LSB) -> bit 7 (MSB). See DAC_cfg.h. */
static const GPIOx_PinConfig_t s_xDacPins[HDAC_PIN_COUNT] = HDAC_PINS;

/* Loop cursors for the streaming (per-tick) sounds. */
static u32 s_u32WakaIndex = 0;
static u32 s_u32PowerIndex = 0;

/* ------------------------------------------------------------------------ */
/* Module state                                                              */
/* ------------------------------------------------------------------------ */

static u8 s_u8Active = 0;
static PacmanDirection s_xCurrentDirection = PACMAN_NONE;
static PacmanDirection s_xRequestedDirection = PACMAN_NONE;

static u32 s_u32LastScore = 0xFFFFFFFFUL;
static u8 s_u8LastLives = 0xFF;
static u8 s_u8LastStatus = 0xFF;

/* ------------------------------------------------------------------------ */
/* Audio helpers                                                             */
/* ------------------------------------------------------------------------ */

static void pacman_vAudioStopStreaming(void)
{
    s_u32WakaIndex = 0;
    s_u32PowerIndex = 0;
    HDAC_vSilence();
}

static void pacman_vAudioPlayStart(void)
{
    HDAC_vPlaySoundSync(AUDIO_START, AUDIO_START_LEN);
}

static void pacman_vAudioPlayDeath(void)
{
    HDAC_vPlaySoundSync(AUDIO_DEATH, AUDIO_DEATH_LEN);
}

static void pacman_vAudioPlayPowerPellet(void)
{
    s_u32PowerIndex = 0;
    HDAC_vPlaySoundSync(AUDIO_POWER_PELLET, AUDIO_POWER_PELLET_LEN);
}

/* Fill one whole tick with sound, which also provides the tick's delay. */
static void pacman_vAudioTick(u8 A_u8Moving, u8 A_u8Frightened)
{
    if (A_u8Frightened)
    {
        /* Power-pellet siren loops for the whole frightened window. */
        HDAC_vStreamSamples(AUDIO_POWER_PELLET, AUDIO_POWER_PELLET_LEN,
                            &s_u32PowerIndex, SAMPLES_PER_TICK);
    }
    else if (A_u8Moving)
    {
        /* Waka-waka loops while Pac-Man is actually moving. */
        HDAC_vStreamSamples(AUDIO_WAKA, AUDIO_WAKA_LEN, &s_u32WakaIndex,
                            SAMPLES_PER_TICK);
    }
    else
    {
        /* Standing still: hold mid-rail for the whole tick. */
        HDAC_vHoldSilence(SAMPLES_PER_TICK);
    }
}

/* ------------------------------------------------------------------------ */
/* Rendering helpers                                                         */
/* ------------------------------------------------------------------------ */

static u16 pacman_u16GhostColor(u8 A_u8Ghost, GhostStatus A_xStatus)
{
    if ((A_xStatus == GHOST_STATUS_FLEEING) ||
        (A_xStatus == GHOST_STATUS_FLICKERING))
    {
        return (A_xStatus == GHOST_STATUS_FLICKERING) ? PACMAN_WHITE
                                                      : PACMAN_FRIGHTENED_BLUE;
    }
    if (A_xStatus == GHOST_STATUS_EATEN)
    {
        return PACMAN_WHITE;
    }

    return (A_u8Ghost == 0) ? PACMAN_RED
         : (A_u8Ghost == 1) ? PACMAN_MAGENTA
         : (A_u8Ghost == 2) ? PACMAN_CYAN
                            : PACMAN_ORANGE;
}

/* Repaint one maze tile: whatever entity stands on it, otherwise its terrain.
 * Entities, walls, power pellets and the gate fill the whole 4x4 cell; plain
 * pellets are drawn as a 2x2 dot centred in it. */
static void pacman_vDrawCell(u8 A_u8Row, u8 A_u8Column)
{
    u8 pacManRow;
    u8 pacManColumn;
    u16 color = PACMAN_BLACK;
    PacmanTile tile;
    u8 entity = 0;
    u8 size;
    u8 offset;
    u8 full;

    HTFT_vDrawRect((u16)(BOARD_X + A_u8Column * CELL),
                   (u16)(BOARD_Y + A_u8Row * CELL),
                   CELL, CELL, PACMAN_BLACK);

    PacManGame_vGetPosition(&pacManRow, &pacManColumn);
    tile = PacManGame_xGetTile(A_u8Row, A_u8Column);

    if ((A_u8Row == pacManRow) && (A_u8Column == pacManColumn))
    {
        color = PACMAN_YELLOW;
        entity = 1;
    }
    else
    {
        for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
        {
            u8 ghostRow;
            u8 ghostColumn;

            PacManGame_vGhostGetPosition(ghost, &ghostRow, &ghostColumn);
            if ((ghostRow == A_u8Row) && (ghostColumn == A_u8Column))
            {
                color = pacman_u16GhostColor(
                    ghost, PacManGame_xGhostGetStatus(ghost));
                entity = 1;
                break;
            }
        }
    }

    if (color == PACMAN_BLACK)
    {
        if (tile == PACMAN_TILE_WALL)
        {
            color = PACMAN_WALL_BLUE;
        }
        else if ((tile == PACMAN_TILE_PELLET) ||
                 (tile == PACMAN_TILE_POWER_PELLET))
        {
            color = PACMAN_WHITE;
        }
        else if (tile == PACMAN_TILE_GATE)
        {
            color = PACMAN_MAGENTA;
        }
    }

    full = entity || (tile == PACMAN_TILE_WALL) ||
           (tile == PACMAN_TILE_POWER_PELLET) || (tile == PACMAN_TILE_GATE);
    size = full ? CELL : 2;
    offset = full ? 0 : 1;

    HTFT_vDrawRect((u16)(BOARD_X + A_u8Column * CELL + offset),
                   (u16)(BOARD_Y + A_u8Row * CELL + offset),
                   size, size, color);
}

static void pacman_vU32ToStr(u32 A_u32Value, char *A_pcBuffer)
{
    char temp[12];
    u8 i = 0;

    if (A_u32Value == 0)
    {
        A_pcBuffer[0] = '0';
        A_pcBuffer[1] = '\0';
        return;
    }

    while (A_u32Value > 0)
    {
        temp[i++] = (char)('0' + (A_u32Value % 10));
        A_u32Value /= 10;
    }

    for (u8 j = 0; j < i; j++)
    {
        A_pcBuffer[j] = temp[i - 1 - j];
    }

    A_pcBuffer[i] = '\0';
}

/* Row-major 7x7 bitmap; bit 0 of each byte is the leftmost column. */
static const u8 s_u8PacManLifeIcon[7] = {
    0x1C, /* ..###.. */
    0x3E, /* .#####. */
    0x3C, /* .####.. */
    0x38, /* .###... */
    0x3C, /* .####.. */
    0x3E, /* .#####. */
    0x1C  /* ..###.. */
};

/* The TFT driver has no icon primitive, so draw the 49 pixels directly. */
static void pacman_vDrawIcon7x7(u16 A_u16X, u16 A_u16Y, const u8 *A_pu8Icon,
                                u16 A_u16Color, u16 A_u16BgColor)
{
    for (u8 row = 0; row < 7; row++)
    {
        u8 bits = A_pu8Icon[row];

        for (u8 column = 0; column < 7; column++)
        {
            HTFT_vDrawPixel((u16)(A_u16X + column), (u16)(A_u16Y + row),
                            GET_BIT(bits, column) ? A_u16Color : A_u16BgColor);
        }
    }
}

static u8 pacman_u8HudStatus(void)
{
    if (PacManGame_u8GetLives() == 0)
    {
        return HUD_STATUS_GAME_OVER;
    }
    if (PacManGame_u8LevelComplete())
    {
        return HUD_STATUS_WIN;
    }

    return HUD_STATUS_NONE;
}

/* Repaint only the HUD fields whose value actually changed, so a tick costs a
 * handful of SPI bytes instead of a full HUD redraw. */
static void pacman_vUpdateHud(u8 A_u8ForceRedraw)
{
    u32 currentScore = PacManGame_u32GetScore();
    u8 currentLives = PacManGame_u8GetLives();
    u8 currentStatus = pacman_u8HudStatus();

    if (A_u8ForceRedraw)
    {
        /* Blue divider line between the maze and the HUD. */
        HTFT_vDrawRect(0, HUD_DIVIDER_Y, SCREEN_WIDTH, 1, PACMAN_WALL_BLUE);
        HTFT_vDrawString(HUD_LABEL_X, HUD_SCORE_Y, "SCORE:", PACMAN_CYAN,
                         PACMAN_BLACK);
        HTFT_vDrawString(HUD_LABEL_X, HUD_LIVES_Y, "LIVES:", PACMAN_CYAN,
                         PACMAN_BLACK);

        s_u32LastScore = 0xFFFFFFFFUL;
        s_u8LastLives = 0xFF;
        s_u8LastStatus = 0xFF;
    }

    if (currentScore != s_u32LastScore)
    {
        char scoreStr[12];

        pacman_vU32ToStr(currentScore, scoreStr);
        HTFT_vDrawRect(HUD_VALUE_X, HUD_SCORE_Y, HUD_SCORE_FIELD_W, HUD_TEXT_H,
                       PACMAN_BLACK);
        HTFT_vDrawString(HUD_VALUE_X, HUD_SCORE_Y, scoreStr, PACMAN_YELLOW,
                         PACMAN_BLACK);
        s_u32LastScore = currentScore;
    }

    if (currentLives != s_u8LastLives)
    {
        for (u8 i = 0; i < PACMAN_STARTING_LIVES; i++)
        {
            u16 xPos = (u16)(HUD_VALUE_X + i * HUD_LIFE_SPACING);

            if (i < currentLives)
            {
                pacman_vDrawIcon7x7(xPos, HUD_LIVES_Y, s_u8PacManLifeIcon,
                                    PACMAN_YELLOW, PACMAN_BLACK);
            }
            else
            {
                HTFT_vDrawRect(xPos, HUD_LIVES_Y, 7, 7, PACMAN_BLACK);
            }
        }

        s_u8LastLives = currentLives;
    }

    if (currentStatus != s_u8LastStatus)
    {
        if (currentStatus == HUD_STATUS_GAME_OVER)
        {
            HTFT_vDrawString(HUD_STATUS_X, HUD_LIVES_Y, "OVER!", PACMAN_RED,
                             PACMAN_BLACK);
        }
        else if (currentStatus == HUD_STATUS_WIN)
        {
            HTFT_vDrawString(HUD_STATUS_X, HUD_LIVES_Y, "WIN!", PACMAN_GREEN,
                             PACMAN_BLACK);
        }
        else
        {
            HTFT_vDrawRect(HUD_STATUS_X, HUD_LIVES_Y, HUD_STATUS_FIELD_W,
                           HUD_TEXT_H, PACMAN_BLACK);
        }

        s_u8LastStatus = currentStatus;
    }
}

static void pacman_vDrawBoard(void)
{
    HTFT_vDrawRect(0, BOARD_Y, SCREEN_WIDTH, (u16)(BOARD_HEIGHT * CELL),
                   PACMAN_BLACK);

    for (u8 row = 0; row < BOARD_HEIGHT; row++)
    {
        for (u8 column = 0; column < BOARD_WIDTH; column++)
        {
            pacman_vDrawCell(row, column);
        }
    }

    pacman_vUpdateHud(1);
}

static void pacman_vCapturePositions(u8 *A_pu8PacManRow, u8 *A_pu8PacManColumn,
                                     u8 A_au8GhostRows[GHOST_COUNT],
                                     u8 A_au8GhostColumns[GHOST_COUNT])
{
    PacManGame_vGetPosition(A_pu8PacManRow, A_pu8PacManColumn);

    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        PacManGame_vGhostGetPosition(ghost, &A_au8GhostRows[ghost],
                                     &A_au8GhostColumns[ghost]);
    }
}

/* Repaint only the tiles an entity left or entered this tick. */
static void pacman_vRedrawChangedPositions(u8 A_u8OldPacManRow,
                                           u8 A_u8OldPacManColumn,
                                           const u8 A_au8OldGhostRows[GHOST_COUNT],
                                           const u8 A_au8OldGhostColumns[GHOST_COUNT])
{
    u8 pacManRow;
    u8 pacManColumn;
    u8 ghostRows[GHOST_COUNT];
    u8 ghostColumns[GHOST_COUNT];

    pacman_vCapturePositions(&pacManRow, &pacManColumn, ghostRows, ghostColumns);

    pacman_vDrawCell(A_u8OldPacManRow, A_u8OldPacManColumn);
    pacman_vDrawCell(pacManRow, pacManColumn);

    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        pacman_vDrawCell(A_au8OldGhostRows[ghost], A_au8OldGhostColumns[ghost]);
        pacman_vDrawCell(ghostRows[ghost], ghostColumns[ghost]);
    }
}

/* Draw a fresh board and restart the round from tile one. */
static void pacman_vRestart(void)
{
    PacManGame_vStartGame();

    s_xCurrentDirection = PACMAN_NONE;
    s_xRequestedDirection = PACMAN_NONE;

    pacman_vAudioStopStreaming();

    HTFT_vFillBackgroundColor(PACMAN_BLACK);
    pacman_vDrawBoard();

    pacman_vAudioPlayStart();
}

/* ------------------------------------------------------------------------ */
/* Module API                                                                */
/* ------------------------------------------------------------------------ */

void PACMAN_vInit(void)
{
    /* The DAC ladder sits on port B, which nothing else in this project uses,
     * so its clock is enabled here rather than in main's system init. */
    MRCC_vEnableCLK(RCC_AHB1, GPIO_PORTB);
    HDAC_vInit(s_xDacPins, HDAC_PIN_COUNT);

    s_u8Active = 1;
    pacman_vRestart();
}

u8 PACMAN_u8HandleInput(u8 A_u8Key)
{
    switch (A_u8Key)
    {
        case IR_KEY_2:
        case IR_NAV_UP:
            s_xRequestedDirection = PACMAN_UP;
            break;

        case IR_KEY_8:
        case IR_NAV_DOWN:
            s_xRequestedDirection = PACMAN_DOWN;
            break;

        case IR_KEY_4:
        case IR_NAV_LEFT:
            s_xRequestedDirection = PACMAN_LEFT;
            break;

        case IR_KEY_6:
        case IR_NAV_RIGHT:
            s_xRequestedDirection = PACMAN_RIGHT;
            break;

        case IR_KEY_5:
        case IR_NAV_OK:
            pacman_vRestart();
            break;

        case IR_NAV_EXIT:
        case IR_KEY_MODE:
            /* Leave the DAC parked at silence for whatever runs next. */
            s_u8Active = 0;
            pacman_vAudioStopStreaming();
            return PACMAN_STATE_EXIT;

        default:
            break;
    }

    return PACMAN_STATE_CONTINUE;
}

void PACMAN_vUpdate(void)
{
    u8 oldPacManRow;
    u8 oldPacManColumn;
    u8 oldGhostRows[GHOST_COUNT];
    u8 oldGhostColumns[GHOST_COUNT];
    u8 moved = 0;
    u8 powerPelletEaten = 0;
    u8 wasFrightened;

    if (!s_u8Active)
    {
        return;
    }

    pacman_vCapturePositions(&oldPacManRow, &oldPacManColumn, oldGhostRows,
                             oldGhostColumns);
    wasFrightened = PacManGame_u8IsFrightenedMode();

    if (!PacManGame_u8LevelComplete() && (PacManGame_u8GetLives() > 0))
    {
        /* If a new direction was requested, try to move that way. If it works,
         * adopt it. If a wall blocks it, keep going the current way and retry
         * the request next tick. With no request, keep coasting. */
        if (s_xRequestedDirection != PACMAN_NONE)
        {
            if (PacManGame_u8Move(s_xRequestedDirection))
            {
                s_xCurrentDirection = s_xRequestedDirection;
                s_xRequestedDirection = PACMAN_NONE;
                moved = 1;
            }
            else if (s_xCurrentDirection != PACMAN_NONE)
            {
                moved = PacManGame_u8Move(s_xCurrentDirection);
            }
        }
        else if (s_xCurrentDirection != PACMAN_NONE)
        {
            moved = PacManGame_u8Move(s_xCurrentDirection);
        }

        /* Frightened mode starting now means a power pellet was eaten on this
         * tick, so its jingle should play once. */
        if (!wasFrightened && PacManGame_u8IsFrightenedMode())
        {
            powerPelletEaten = 1;
        }

        /* Check collisions right after Pac-Man moves, which stops him walking
         * through a ghost that is moving the opposite way. */
        if (PacManGame_u8GhostsCollideWithPacMan())
        {
            if (PacManGame_u32GhostsHandleCollision() == 0)
            {
                pacman_vAudioPlayDeath();
                PacManGame_vLoseLife();
                PacManGame_vGhostsReset();
                s_xCurrentDirection = PACMAN_NONE;
                s_xRequestedDirection = PACMAN_NONE;
                moved = 0;
            }
        }

        /* Ghosts only move once the player has started moving. */
        if ((s_xCurrentDirection != PACMAN_NONE) ||
            (s_xRequestedDirection != PACMAN_NONE))
        {
            PacManGame_vGhostsUpdate();

            if (PacManGame_u8GhostsCollideWithPacMan())
            {
                if (PacManGame_u32GhostsHandleCollision() == 0)
                {
                    pacman_vAudioPlayDeath();
                    PacManGame_vLoseLife();
                    PacManGame_vGhostsReset();
                    s_xCurrentDirection = PACMAN_NONE;
                    s_xRequestedDirection = PACMAN_NONE;
                    moved = 0;
                }
            }
        }
    }

    pacman_vRedrawChangedPositions(oldPacManRow, oldPacManColumn, oldGhostRows,
                                   oldGhostColumns);
    pacman_vUpdateHud(0);

    /* One-shot jingles block for their own length; drop the streaming cursors
     * so waka-waka restarts cleanly afterwards. */
    if (powerPelletEaten)
    {
        pacman_vAudioStopStreaming();
        pacman_vAudioPlayPowerPellet();
    }

    /* Fill the rest of the tick with the looping effects, which is also what
     * paces this iteration at ~50 ms. */
    pacman_vAudioTick(moved && (PacManGame_u8GetLives() > 0),
                      PacManGame_u8IsFrightenedMode());
}
