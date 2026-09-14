#ifndef PACMAN_GAME_H
#define PACMAN_GAME_H
#include <stdint.h>

#define BOARD_WIDTH 28
#define BOARD_HEIGHT 31
#define BOARD_TUNNEL_ROW 14
#define PACMAN_STARTING_LIVES 3
#define GHOST_COUNT 4
#define GHOST_FRIGHTENED_TICKS 67
#define GHOST_FLICKER_TICKS 20

typedef enum {
	PACMAN_NONE = 0,
	PACMAN_UP,
	PACMAN_DOWN,
	PACMAN_LEFT,
	PACMAN_RIGHT,
	PACMAN_EXIT
} PacmanDirection;

typedef enum {
	PACMAN_TILE_EMPTY,
	PACMAN_TILE_WALL,
	PACMAN_TILE_PELLET,
	PACMAN_TILE_POWER_PELLET,
	PACMAN_TILE_GATE
} PacmanTile;

typedef enum {
	GHOST_STATUS_NORMAL,
	GHOST_STATUS_FLEEING,
	GHOST_STATUS_FLICKERING,
	GHOST_STATUS_EATEN
} GhostStatus;

uint8_t pacman_move(PacmanDirection direction);
void pacman_start_game(void);
void pacman_reset(void);
void pacman_lose_life(void);
void pacman_get_position(uint8_t *row, uint8_t *column);
uint32_t pacman_get_score(void);
uint8_t pacman_get_lives(void);
uint16_t pacman_get_pellets_remaining(void);
uint8_t pacman_level_complete(void);
PacmanTile game_get_tile(uint8_t row, uint8_t column);
void ghosts_reset(void);
void ghosts_update(void);
void ghost_get_position(uint8_t ghost, uint8_t *row, uint8_t *column);
GhostStatus ghost_get_status(uint8_t ghost);
uint8_t ghost_is_released(uint8_t ghost);
uint8_t ghosts_collide_with_pacman(void);
uint32_t ghosts_handle_collision(void);

#endif
