/*
 * PACMAN_game_int.h
 *
 * Pac-Man game engine: board, pellets, Pac-Man and the four ghosts.
 *
 * This layer is pure logic - it owns no hardware and performs no I/O, so it
 * can be driven by any front end. PACMAN_prg.c is this project's front end
 * (TFT rendering, IR input and DAC audio).
 *
 * Coordinates are (row, column) tile indices into a BOARD_HEIGHT x BOARD_WIDTH
 * grid, row 0 at the top.
 */

#ifndef APP_PACMAN_GAME_INT_H_
#define APP_PACMAN_GAME_INT_H_

#include "../LIB/STD_TYPES.h"

/* ------------------------------------------------------------------------ */
/* Board geometry                                                            */
/* ------------------------------------------------------------------------ */

/* Kept as plain signed constants: they are mixed with signed ghost-target
 * arithmetic, and an unsigned suffix here would silently turn a negative
 * target into a huge positive one during comparison. */
#define BOARD_WIDTH             28
#define BOARD_HEIGHT            31

/* The only row where the left and right edges wrap into each other. */
#define BOARD_TUNNEL_ROW        14

/* ------------------------------------------------------------------------ */
/* Rules                                                                     */
/* ------------------------------------------------------------------------ */

#define GHOST_COUNT             4U
#define PACMAN_STARTING_LIVES   3U

/* Length of the frightened window after a power pellet, counted in calls to
 * PacManGame_vGhostsUpdate(). The front end ticks the game every 50 ms, so
 * 200 ticks is roughly 10 s of play. Must fit in a u8. */
#define GHOST_FRIGHTENED_TICKS  200U

/* Ghosts switch to the flickering "about to recover" colour once this many
 * frightened ticks remain (40 ticks is roughly the last 2 s). */
#define GHOST_FLICKER_TICKS     40U

/* ------------------------------------------------------------------------ */
/* Types                                                                     */
/* ------------------------------------------------------------------------ */

typedef enum {
    PACMAN_NONE = 0,
    PACMAN_UP,
    PACMAN_DOWN,
    PACMAN_LEFT,
    PACMAN_RIGHT
} PacmanDirection;

typedef enum {
    PACMAN_TILE_EMPTY = 0,
    PACMAN_TILE_WALL,
    PACMAN_TILE_PELLET,
    PACMAN_TILE_POWER_PELLET,
    PACMAN_TILE_GATE
} PacmanTile;

typedef enum {
    GHOST_STATUS_NORMAL = 0,
    GHOST_STATUS_FLEEING,
    GHOST_STATUS_FLICKERING,
    GHOST_STATUS_EATEN
} GhostStatus;

/* ------------------------------------------------------------------------ */
/* Pac-Man                                                                   */
/* ------------------------------------------------------------------------ */

/* Restart from scratch: full board of pellets, score 0, lives back to
 * PACMAN_STARTING_LIVES, ghosts back in the house. */
void PacManGame_vStartGame(void);

/* Put Pac-Man back on his starting tile without touching score or pellets. */
void PacManGame_vReset(void);

/* Try to step one tile. Returns 1 if the move happened, 0 if a wall blocked
 * it. A successful step also eats whatever pellet is on the new tile. */
u8 PacManGame_u8Move(PacmanDirection A_xDirection);

/* Take one life (if any remain) and reset Pac-Man's position. */
void PacManGame_vLoseLife(void);

void PacManGame_vGetPosition(u8 *A_pu8Row, u8 *A_pu8Column);
u32 PacManGame_u32GetScore(void);
u8 PacManGame_u8GetLives(void);
u16 PacManGame_u16GetPelletsRemaining(void);
u8 PacManGame_u8LevelComplete(void);
u8 PacManGame_u8IsFrightenedMode(void);

/* What should be drawn on a tile: walls and the ghost-house gate come from the
 * static maze, pellets from the live board state. Out-of-range tiles read as
 * PACMAN_TILE_EMPTY. */
PacmanTile PacManGame_xGetTile(u8 A_u8Row, u8 A_u8Column);

/* ------------------------------------------------------------------------ */
/* Ghosts                                                                    */
/* ------------------------------------------------------------------------ */

/* Send every ghost back to the house and cancel frightened mode. */
void PacManGame_vGhostsReset(void);

/* Advance every released ghost by one tile and age frightened mode by one
 * tick. Call once per game tick, only while Pac-Man is moving. */
void PacManGame_vGhostsUpdate(void);

void PacManGame_vGhostGetPosition(u8 A_u8Ghost, u8 *A_pu8Row, u8 *A_pu8Column);
GhostStatus PacManGame_xGhostGetStatus(u8 A_u8Ghost);
u8 PacManGame_u8GhostIsReleased(u8 A_u8Ghost);

/* 1 if any ghost that is not already eaten shares Pac-Man's tile. */
u8 PacManGame_u8GhostsCollideWithPacMan(void);

/* Resolve the collision found by PacManGame_u8GhostsCollideWithPacMan().
 * Returns the points scored for eating frightened ghosts, or 0 when a normal
 * ghost caught Pac-Man - in which case the caller must take a life. */
u32 PacManGame_u32GhostsHandleCollision(void);

#endif /* APP_PACMAN_GAME_INT_H_ */
