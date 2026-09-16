#include "game.h"
#include <stdio.h>

static int expect_position(uint8_t expected_row, uint8_t expected_column)
{
    uint8_t row;
    uint8_t column;

    pacman_get_position(&row, &column);
    if (row != expected_row || column != expected_column) {
        printf("expected position (%u, %u), got (%u, %u)\n",
               expected_row, expected_column, row, column);
        return 0;
    }

    return 1;
}

int main(void)
{
    pacman_start_game();
    if (pacman_get_score() != 10 ||
        pacman_get_lives() != PACMAN_STARTING_LIVES ||
        pacman_get_pellets_remaining() == 0 ||
        pacman_level_complete()) {
        return 1;
    }

    pacman_reset();
    if (!expect_position(23, 13)) {
        return 1;
    }

    if (!pacman_move(PACMAN_LEFT) || !expect_position(23, 12)) {
        return 1;
    }
    if (pacman_get_score() != 20 ||
        game_get_tile(23, 12) != PACMAN_TILE_EMPTY) {
        return 1;
    }

    pacman_lose_life();
    if (pacman_get_lives() != PACMAN_STARTING_LIVES - 1 ||
        !expect_position(23, 13)) {
        return 1;
    }

    uint8_t ghost_row;
    uint8_t ghost_column;
    ghost_get_position(0, &ghost_row, &ghost_column);
    if (ghost_row != 14 || ghost_column != 13 ||
        ghosts_collide_with_pacman()) {
        return 1;
    }
    ghosts_update();
    ghost_get_position(0, &ghost_row, &ghost_column);
    if (ghost_row >= BOARD_HEIGHT || ghost_column >= BOARD_WIDTH ||
        game_get_tile(ghost_row, ghost_column) == PACMAN_TILE_WALL) {
        return 1;
    }

    ghosts_reset();
    if (ghost_get_status(0) != GHOST_STATUS_NORMAL) {
        return 1;
    }
    for (int update = 0; update < 6; update++) {
        ghosts_update();
    }
    if (ghost_is_released(0)) {
        return 1;
    }
    ghosts_update();
    ghost_get_position(0, &ghost_row, &ghost_column);
    if (!ghost_is_released(0) || ghost_row != 8 || ghost_column != 12) {
        return 1;
    }

    ghosts_reset();
    for (int update = 0; update < 100; update++) {
        ghosts_update();
    }
    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        if (!ghost_is_released(ghost)) {
            ghost_get_position(ghost, &ghost_row, &ghost_column);
            printf("ghost %u stuck at (%u, %u)\n", ghost,
                   ghost_row, ghost_column);
            return 1;
        }
    }

    pacman_reset();
    if (pacman_move(PACMAN_UP) || !expect_position(23, 13)) {
        return 1;
    }

    pacman_reset();
    if (pacman_move(PACMAN_DOWN) || !expect_position(23, 13)) {
        return 1;
    }

    pacman_reset();
    if (pacman_move(PACMAN_NONE) || !expect_position(23, 13)) {
        return 1;
    }

    pacman_reset();
    for (int move = 0; move < 7; move++) {
        if (!pacman_move(PACMAN_LEFT)) {
            return 1;
        }
    }
    for (int move = 0; move < 9; move++) {
        if (!pacman_move(PACMAN_UP)) {
            return 1;
        }
    }
    for (int move = 0; move < 6; move++) {
        if (!pacman_move(PACMAN_LEFT)) {
            return 1;
        }
    }
    if (!expect_position(14, 0) ||
        !pacman_move(PACMAN_LEFT) || !expect_position(14, 27) ||
        !pacman_move(PACMAN_RIGHT) || !expect_position(14, 0)) {
        return 1;
    }

    /* Navigate to power pellet at (3, 1) */
    for (int move = 0; move < 6; move++) {
        if (!pacman_move(PACMAN_RIGHT)) return 1;
    }
    if (!expect_position(14, 6)) return 1;
    for (int move = 0; move < 6; move++) {
        if (!pacman_move(PACMAN_UP)) return 1;
    }
    if (!expect_position(8, 6)) return 1;
    for (int move = 0; move < 5; move++) {
        if (!pacman_move(PACMAN_LEFT)) return 1;
    }
    if (!expect_position(8, 1)) return 1;
    for (int move = 0; move < 5; move++) {
        if (!pacman_move(PACMAN_UP)) return 1;
    }
    if (!expect_position(3, 1)) return 1;

    /* Verify power pellet was eaten */
    /* Verify power pellet was eaten and ghost is fleeing */
    if (game_get_tile(3, 1) != PACMAN_TILE_EMPTY) {
        return 1;
    }
    if (ghost_get_status(0) != GHOST_STATUS_FLEEING) {
        return 1;
    }

    /* Verify frightened ghost moves ~70% speed (skips 3 out of 10 updates) */
    uint8_t moves = 0;
    for (int tick = 0; tick < 10; tick++) {
        uint8_t r1, c1, r2, c2;
        ghost_get_position(0, &r1, &c1);
        ghosts_update();
        ghost_get_position(0, &r2, &c2);
        if (r1 != r2 || c1 != c2) {
            moves++;
        }
    }
    if (moves != 7) {
        printf("expected 7 moves in 10 ticks for frightened ghost, got %u\n", moves);
        return 1;
    }

    puts("movement tests passed");
    return 0;
}
