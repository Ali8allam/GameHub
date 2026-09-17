#include "../LIB/STD_TYPES.h"
#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"
#include "../MCAL/SPI/SPI_int.h"
#include "../MCAL/SYSTICK/SYSTICK_int.h"
#include "../HAL/ST7735/ST7735_int.h"
#include "../HAL/IR/IR_int.h"
#include "../HAL/DAC/DAC_int.h"
#include "../HAL/DAC/DAC_prv.h"
#include "../../include/game.h"

#define BLACK 0x0000
#define WALL_BLUE 0xB7FF
#define FRIGHTENED_BLUE 0x001F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define RED 0xF800
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define ORANGE 0xFD20
#define GREEN 0x07E0
#define CELL 4
#define BOARD_X 8
#define BOARD_Y 0

static u16 ghost_color(u8 ghost, GhostStatus status)
{
    if (status == GHOST_STATUS_FLEEING || status == GHOST_STATUS_FLICKERING) {
        return status == GHOST_STATUS_FLICKERING ? WHITE : FRIGHTENED_BLUE;
    }
    if (status == GHOST_STATUS_EATEN) {
        return WHITE;
    }
    return ghost == 0 ? RED : ghost == 1 ? MAGENTA : ghost == 2 ? CYAN : ORANGE;
}

static void draw_cell(u8 row, u8 column)
{
    u8 pacman_row;
    u8 pacman_column;
    u16 color = BLACK;
    PacmanTile tile;
    u8 entity = 0;

    HST7735_vFillRect((u8)(BOARD_X + column * CELL),
                      (u8)(BOARD_Y + row * CELL),
                      CELL, CELL, BLACK);
    pacman_get_position(&pacman_row, &pacman_column);
    tile = game_get_tile(row, column);

    if (row == pacman_row && column == pacman_column) {
        color = YELLOW;
        entity = 1;
    } else {
        for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++) {
            u8 ghost_row;
            u8 ghost_column;
            ghost_get_position(ghost, &ghost_row, &ghost_column);
            if (ghost_row == row && ghost_column == column) {
                color = ghost_color(ghost, ghost_get_status(ghost));
                entity = 1;
                break;
            }
        }
    }

    if (color == BLACK) {
        if (tile == PACMAN_TILE_WALL) {
            color = WALL_BLUE;
        } else if (tile == PACMAN_TILE_PELLET) {
            color = WHITE;
        } else if (tile == PACMAN_TILE_POWER_PELLET) {
            color = WHITE;
        } else if (tile == PACMAN_TILE_GATE) {
            color = MAGENTA;
        }
    }

    {
        u8 size = entity || tile == PACMAN_TILE_WALL ||
                  tile == PACMAN_TILE_POWER_PELLET ||
                  tile == PACMAN_TILE_GATE ? CELL : 2;
        u8 offset = entity || tile == PACMAN_TILE_WALL ||
                tile == PACMAN_TILE_POWER_PELLET ||
                tile == PACMAN_TILE_GATE ? 0 : 1;
        HST7735_vFillRect((u8)(BOARD_X + column * CELL + offset),
                          (u8)(BOARD_Y + row * CELL + offset),
                          size, size, color);
    }
}

static u32 g_last_score = 0xFFFFFFFF;
static u8 g_last_lives = 0xFF;

static void u32_to_str(u32 val, char *buf)
{
    char temp[12];
    u8 i = 0;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0) {
        temp[i++] = (char)('0' + (val % 10));
        val /= 10;
    }
    for (u8 j = 0; j < i; j++) {
        buf[j] = temp[i - 1 - j];
    }
    buf[i] = '\0';
}

static const u8 pacman_life_icon[7] = {
    0x1C, /* ..###.. */
    0x3E, /* .#####. */
    0x3C, /* .####.. */
    0x38, /* .###... */
    0x3C, /* .####.. */
    0x3E, /* .#####. */
    0x1C  /* ..###.. */
};

static void update_hud(u8 force_redraw)
{
    u32 current_score = pacman_get_score();
    u8 current_lives = pacman_get_lives();

    if (force_redraw) {
        /* Draw blue divider line between maze and HUD */
        HST7735_vFillRect(0, 125, ST7735_WIDTH, 1, WALL_BLUE);
        HST7735_vDrawString(4, 130, "SCORE:", CYAN, BLACK);
        HST7735_vDrawString(4, 144, "LIVES:", CYAN, BLACK);
        g_last_score = 0xFFFFFFFF;
        g_last_lives = 0xFF;
    }

    if (current_score != g_last_score) {
        char score_str[12];
        u32_to_str(current_score, score_str);
        HST7735_vFillRect(44, 130, 80, 7, BLACK);
        HST7735_vDrawString(44, 130, score_str, YELLOW, BLACK);
        g_last_score = current_score;
    }

    if (current_lives != g_last_lives) {
        for (u8 i = 0; i < 3; i++) {
            u8 x_pos = 44 + i * 10;
            if (i < current_lives) {
                HST7735_vDrawIcon7x7(x_pos, 144, pacman_life_icon, YELLOW, BLACK);
            } else {
                HST7735_vFillRect(x_pos, 144, 7, 7, BLACK);
            }
        }

        if (current_lives == 0) {
            HST7735_vDrawString(78, 144, "OVER!", RED, BLACK);
        } else if (pacman_level_complete()) {
            HST7735_vDrawString(78, 144, "WIN!", GREEN, BLACK);
        } else {
            HST7735_vFillRect(78, 144, 36, 7, BLACK);
        }

        g_last_lives = current_lives;
    }
}

static void draw_board(void)
{
    HST7735_vFillRect(0, BOARD_Y, ST7735_WIDTH, BOARD_HEIGHT * CELL, BLACK);
    for (u8 row = 0; row < BOARD_HEIGHT; row++) {
        for (u8 column = 0; column < BOARD_WIDTH; column++) {
            draw_cell(row, column);
        }
    }
    update_hud(1);
}

static void capture_positions(u8 *pacman_row, u8 *pacman_column,
                              u8 ghost_rows[GHOST_COUNT],
                              u8 ghost_columns[GHOST_COUNT])
{
    pacman_get_position(pacman_row, pacman_column);
    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++) {
        ghost_get_position(ghost, &ghost_rows[ghost], &ghost_columns[ghost]);
    }
}

static void redraw_changed_positions(u8 old_pacman_row,
                                     u8 old_pacman_column,
                                     const u8 old_ghost_rows[GHOST_COUNT],
                                     const u8 old_ghost_columns[GHOST_COUNT])
{
    u8 pacman_row;
    u8 pacman_column;
    u8 ghost_rows[GHOST_COUNT];
    u8 ghost_columns[GHOST_COUNT];

    capture_positions(&pacman_row, &pacman_column, ghost_rows, ghost_columns);
    draw_cell(old_pacman_row, old_pacman_column);
    draw_cell(pacman_row, pacman_column);
    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++) {
        draw_cell(old_ghost_rows[ghost], old_ghost_columns[ghost]);
        draw_cell(ghost_rows[ghost], ghost_columns[ghost]);
    }
}

/* DAC data pins, ordered DAC bit 0 (LSB) -> bit 7 (MSB). See DAC_prv.h. */
static const GPIOx_PinConfig_t g_dac_pins[HDAC_PIN_COUNT] = HDAC_PINS;

static void audio_init(void)
{
    /* The DAC uses PB3/PB4, which are JTAG-only pins by default. Release them
     * to GPIO before configuring the bus, otherwise the two least-significant
     * DAC bits would be undrivable and the waveform would be badly distorted. */
    MGPIO_vReleaseJTAGPins();
    HDAC_vInit(g_dac_pins, HDAC_PIN_COUNT);
}

int main(void)
{
    PacmanDirection current_direction = PACMAN_NONE;
    PacmanDirection requested_direction = PACMAN_NONE;
    MSYSTIC_Config_t systick_config = {INT_ENABLE, CLK_SRC_AHB_8};
    u8 old_pacman_row;
    u8 old_pacman_column;
    u8 old_ghost_rows[GHOST_COUNT];
    u8 old_ghost_columns[GHOST_COUNT];

    MRCC_vInit();
    MRCC_vEnableClk(RCC_APB2, RCC_GPIOA);
    MRCC_vEnableClk(RCC_APB2, RCC_GPIOB);
    MRCC_vEnableClk(RCC_APB2, 12);
    MRCC_vEnableClk(RCC_APB2, RCC_AFIO);
    MSYSTICK_vInit(&systick_config);
    MSPI_vInit();
    HST7735_vInit();
    HIR_vInit();
    audio_init();
    HST7735_vFillScreen(BLACK);
    pacman_start_game();
    draw_board();
    HDAC_vPlayStart();

    while (1) {
        u8 command;

        capture_positions(&old_pacman_row, &old_pacman_column,
                          old_ghost_rows, old_ghost_columns);
        if (HIR_u8GetCommand(&command)) {
            if (command == IR_CODE_UP) {
                requested_direction = PACMAN_UP;
            } else if (command == IR_CODE_DOWN) {
                requested_direction = PACMAN_DOWN;
            } else if (command == IR_CODE_LEFT) {
                requested_direction = PACMAN_LEFT;
            } else if (command == IR_CODE_RIGHT) {
                requested_direction = PACMAN_RIGHT;
            } else if (command == IR_CODE_START) {
                pacman_start_game();
                current_direction = PACMAN_NONE;
                requested_direction = PACMAN_NONE;
                HST7735_vFillScreen(BLACK);
                draw_board();
                HDAC_vPlayStart();
            }
        }

        {   /* Audio is driven from the same 50 ms cadence as the game logic. */
            u8 moved = 0;
            u8 power_pellet_eaten = 0;
            u8 was_frightened = pacman_is_frightened_mode();

            if (!pacman_level_complete() && pacman_get_lives() > 0) {
                /* If a new direction was requested, try to move that way.
                 * If successful, adopt it and clear requested_direction.
                 * If blocked by a wall, keep going in current_direction.
                 * If no new request, continuously move in current_direction. */
                if (requested_direction != PACMAN_NONE) {
                    if (pacman_move(requested_direction)) {
                        current_direction = requested_direction;
                        requested_direction = PACMAN_NONE;
                        moved = 1;
                    } else if (current_direction != PACMAN_NONE) {
                        moved = pacman_move(current_direction);
                    }
                } else if (current_direction != PACMAN_NONE) {
                    moved = pacman_move(current_direction);
                }

                /* Frightened mode starting now means a power pellet was
                 * eaten on this tick, so play its jingle once. */
                if (!was_frightened && pacman_is_frightened_mode()) {
                    power_pellet_eaten = 1;
                }

                /* Check collision immediately after Pac-Man moves (prevents pass-through) */
                if (ghosts_collide_with_pacman()) {
                    if (ghosts_handle_collision() == 0) {
                        HDAC_vPlayDeath();
                        pacman_lose_life();
                        ghosts_reset();
                        current_direction = PACMAN_NONE;
                        requested_direction = PACMAN_NONE;
                        moved = 0;
                    }
                }

                /* Ghosts move every tick once player starts moving (if Pac-Man is still alive) */
                if (current_direction != PACMAN_NONE || requested_direction != PACMAN_NONE) {
                    ghosts_update();

                    /* Check collision again after ghosts move */
                    if (ghosts_collide_with_pacman()) {
                        if (ghosts_handle_collision() == 0) {
                            HDAC_vPlayDeath();
                            pacman_lose_life();
                            ghosts_reset();
                            current_direction = PACMAN_NONE;
                            requested_direction = PACMAN_NONE;
                            moved = 0;
                        }
                    }
                }
            }
            redraw_changed_positions(old_pacman_row, old_pacman_column,
                                     old_ghost_rows, old_ghost_columns);
            update_hud(0);

            /* One-shot jingles block for their own duration; drop the
             * streaming cursor so waka-waka restarts cleanly afterwards. */
            if (power_pellet_eaten) {
                HDAC_vStopStreaming();
                HDAC_vPlayPowerPellet();
            }

            /* Fill the remainder of the tick with the looping sound effects,
             * which also provides this iteration's ~50 ms of delay. */
            HDAC_vTickAudio(moved && pacman_get_lives() > 0,
                            pacman_is_frightened_mode());
        }
    }
}