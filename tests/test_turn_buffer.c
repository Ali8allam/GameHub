/* Regression tests for classic-style turn buffering.
 *
 * Pac-Man must remember a direction the player requested even when that turn is
 * currently blocked, keep travelling in the old direction, and apply the turn at
 * the first tile where it becomes legal.
 *
 * The firmware main loop (src/APP/pacman_tft.c) implements this as:
 *
 *     if (requested != NONE) {
 *         if (pacman_move(requested)) { current = requested; requested = NONE; }
 *         else if (current != NONE) { pacman_move(current); }
 *     } else if (current != NONE) {
 *         pacman_move(current);
 *     }
 *
 * These tests drive that same logic so the behaviour is locked in. */
#include "game.h"
#include <stdio.h>

static int failures;

static void check(int condition, const char *what)
{
    printf("  [%s] %s\n", condition ? "PASS" : "FAIL", what);
    if (!condition) {
        failures++;
    }
}

/* Emulate one tick of the firmware's movement block. */
static void tick(PacmanDirection *current, PacmanDirection *requested)
{
    if (*requested != PACMAN_NONE) {
        if (pacman_move(*requested)) {
            *current = *requested;
            *requested = PACMAN_NONE;
        } else if (*current != PACMAN_NONE) {
            pacman_move(*current);
        }
    } else if (*current != PACMAN_NONE) {
        pacman_move(*current);
    }
}

static void get(uint8_t *row, uint8_t *column)
{
    pacman_get_position(row, column);
}

/* Row 23 has openings above at columns 6, 12, 15 and 21 only, so a queued UP
 * from column 13 must survive until column 15. */
static void test_turn_buffered_until_open(void)
{
    PacmanDirection current = PACMAN_RIGHT;
    PacmanDirection requested = PACMAN_UP;
    uint8_t row;
    uint8_t column;

    printf("turn requested while blocked is applied at the first opening\n");
    pacman_start_game();
    pacman_reset();

    /* Columns 13 and 14 have a wall above; column 15 is open. */
    tick(&current, &requested);
    get(&row, &column);
    check(row == 23 && column == 14, "kept moving right through column 14");
    check(requested == PACMAN_UP, "request still queued after column 14");

    tick(&current, &requested);
    get(&row, &column);
    check(row == 23 && column == 15, "kept moving right into column 15");

    tick(&current, &requested);
    get(&row, &column);
    check(row == 22 && column == 15, "turned up as soon as it was legal");
    check(requested == PACMAN_NONE, "request cleared after being applied");
}

/* A request that is legal immediately must take effect at once. */
static void test_immediate_turn(void)
{
    PacmanDirection current = PACMAN_LEFT;
    PacmanDirection requested = PACMAN_UP;
    uint8_t row;
    uint8_t column;

    printf("turn requested at an opening is applied immediately\n");
    pacman_start_game();
    pacman_reset();
    pacman_move(PACMAN_LEFT);           /* (23, 12): UP is open there */
    get(&row, &column);
    check(row == 23 && column == 12, "setup placed Pac-Man at (23,12)");

    tick(&current, &requested);
    get(&row, &column);
    check(row == 22 && column == 12, "turned up on the same tick");
}

/* Classic Pac-Man allows reversing direction instantly. */
static void test_reversal(void)
{
    PacmanDirection current;
    PacmanDirection requested;
    uint8_t row;
    uint8_t column;

    printf("instant reversal still works\n");
    pacman_start_game();
    pacman_reset();
    current = PACMAN_RIGHT;
    requested = PACMAN_NONE;
    tick(&current, &requested);
    get(&row, &column);
    check(column == 14, "moved right to column 14");

    requested = PACMAN_LEFT;
    tick(&current, &requested);
    get(&row, &column);
    check(column == 13, "reversed back to column 13");
}

/* With no queued request, Pac-Man should run until a wall stops him. */
static void test_run_until_wall(void)
{
    PacmanDirection current = PACMAN_LEFT;
    PacmanDirection requested = PACMAN_NONE;
    uint8_t row;
    uint8_t column;

    printf("with no request Pac-Man keeps going until a wall\n");
    pacman_start_game();
    pacman_reset();

    for (int i = 0; i < 20; i++) {
        tick(&current, &requested);
    }
    get(&row, &column);
    check(row == 23 && column == 6, "stopped against the wall at column 6");
}

int main(void)
{
    test_turn_buffered_until_open();
    test_immediate_turn();
    test_reversal();
    test_run_until_wall();

    if (failures) {
        printf("\n%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("\nturn buffering tests passed\n");
    return 0;
}