/*
 * PACMAN_int.h
 *
 * Pac-Man game module for the arcade menu.
 *
 * Same shape as XO_int.h so main.c drives both the same way, with one
 * addition: Pac-Man is not purely key-driven, so main.c must also call
 * PACMAN_vUpdate() on every pass of its loop while this game is on screen.
 * That call is what advances the ghosts, redraws and paces the 50 ms tick.
 */

#ifndef APP_PACMAN_INT_H_
#define APP_PACMAN_INT_H_

#include "../LIB/STD_TYPES.h"

#define PACMAN_STATE_CONTINUE   1
#define PACMAN_STATE_EXIT       0

/* Enter the game: claim the DAC, start a fresh board, draw it and play the
 * start jingle. Blocks for the length of the jingle (about 4 s). */
void PACMAN_vInit(void);

/* Feed one decoded IR key. Returns PACMAN_STATE_EXIT when the player asked to
 * go back to the menu, PACMAN_STATE_CONTINUE otherwise. */
u8 PACMAN_u8HandleInput(u8 A_u8Key);

/* Advance one 50 ms game tick: move Pac-Man and the ghosts, resolve
 * collisions, repaint what changed and play this tick's audio. Does nothing
 * unless the game is currently on screen. Blocks for about 50 ms - IR keys
 * keep arriving meanwhile, since the receiver is interrupt driven. */
void PACMAN_vUpdate(void);

#endif /* APP_PACMAN_INT_H_ */
