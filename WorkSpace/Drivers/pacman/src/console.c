#include "console.h"
#include "game.h"
#include <conio.h>
#include <inttypes.h>
#include <stdio.h>
#include <windows.h>

static PacmanDirection input_read_direction(void)
{
    int c;

    if (!_kbhit()) {
        return PACMAN_NONE;
    }

    c = _getch();

    if (c == 27 || c == 'q' || c == 'Q') {
        return PACMAN_EXIT;
    }

    if (c == 0 || c == 224) {
        c = _getch();
        switch (c) {
            case 72:
                return PACMAN_UP;
            case 80:
                return PACMAN_DOWN;
            case 75:
                return PACMAN_LEFT;
            case 77:
                return PACMAN_RIGHT;
            default:
                return PACMAN_NONE;
        }
    }

    switch (c) {
        case 'w':
        case 'W':
            return PACMAN_UP;
        case 's':
        case 'S':
            return PACMAN_DOWN;
        case 'a':
        case 'A':
            return PACMAN_LEFT;
        case 'd':
        case 'D':
            return PACMAN_RIGHT;
        default:
            return PACMAN_NONE;
    }
}

static int ghost_at_position(uint8_t row, uint8_t column)
{
    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        uint8_t ghost_row;
        uint8_t ghost_column;

        ghost_get_position(ghost, &ghost_row, &ghost_column);
        if (ghost_row == row && ghost_column == column) {
            return 1;
        }
    }

    return 0;
}

static char tile_to_character(PacmanTile tile)
{
    switch (tile) {
        case PACMAN_TILE_WALL:
            return '#';
        case PACMAN_TILE_PELLET:
            return '.';
        case PACMAN_TILE_POWER_PELLET:
            return 'o';
        case PACMAN_TILE_GATE:
            return '-';
        default:
            return ' ';
    }
}

static char ghost_to_character(GhostStatus status)
{
    switch (status) {
        case GHOST_STATUS_FLEEING:
            return 'F';
        case GHOST_STATUS_FLICKERING:
            return 'f';
        case GHOST_STATUS_EATEN:
            return 'e';
        default:
            return 'G';
    }
}

void game_run(void)
{
    PacmanDirection current_direction = PACMAN_NONE;
    PacmanDirection requested_direction = PACMAN_NONE;
    PacmanDirection new_direction;
    uint8_t pacman_moved;
    uint8_t pacman_started = 0;
    uint32_t score_popup = 0;
    uint8_t pacman_row;
    uint8_t pacman_column;

    pacman_start_game();

    for (;;) {
        printf("\x1b[2J\x1b[H");
        printf("Score: %" PRIu32 "  Lives: %u  Pellets: %u\n",
               pacman_get_score(), pacman_get_lives(),
               pacman_get_pellets_remaining());
        if (score_popup != 0) {
            printf("Ghost eaten! +%" PRIu32 "\n", score_popup);
            score_popup = 0;
        }
        if (pacman_level_complete()) {
            puts("LEVEL COMPLETE!");
        } else if (pacman_get_lives() == 0) {
            puts("GAME OVER!");
        }
        pacman_get_position(&pacman_row, &pacman_column);

        for (uint8_t row = 0; row < BOARD_HEIGHT; row++) {
            for (uint8_t column = 0; column < BOARD_WIDTH; column++) {
                if (row == pacman_row && column == pacman_column) {
                    putchar('P');
                } else if (ghost_at_position(row, column)) {
                    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
                        uint8_t ghost_row;
                        uint8_t ghost_column;

                        ghost_get_position(ghost, &ghost_row, &ghost_column);
                        if (ghost_row == row && ghost_column == column) {
                            putchar(ghost_to_character(ghost_get_status(ghost)));
                            break;
                        }
                    }
                } else {
                    putchar(tile_to_character(game_get_tile(row, column)));
                }
            }
            putchar('\n');
        }

        new_direction = input_read_direction();
        if (new_direction == PACMAN_EXIT) {
            break;
        }
        if (new_direction != PACMAN_NONE) {
            requested_direction = new_direction;
        }

        if (!pacman_level_complete() && pacman_get_lives() > 0) {
            pacman_moved = 0;
            if (requested_direction != PACMAN_NONE &&
                pacman_move(requested_direction)) {
                current_direction = requested_direction;
                pacman_moved = 1;
            } else if (pacman_move(current_direction)) {
                pacman_moved = 1;
            }

            if (pacman_moved) {
                pacman_started = 1;
            }
            if (pacman_started) {
                ghosts_update();
                if (ghosts_collide_with_pacman()) {
                    score_popup = ghosts_handle_collision();
                    if (score_popup == 0) {
                        pacman_lose_life();
                        ghosts_reset();
                        current_direction = PACMAN_NONE;
                        requested_direction = PACMAN_NONE;
                        pacman_started = 0;
                    }
                }
            }
        }
        Sleep(150);
    }
}
