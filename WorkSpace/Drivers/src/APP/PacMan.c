#include "PacMan_int.h"

static uint8_t pacman_row = 23;
static uint8_t pacman_column = 13;
static PacmanDirection pacman_direction = PACMAN_NONE;
static uint8_t pellets[BOARD_HEIGHT][BOARD_WIDTH];
static uint16_t pellets_remaining;
static uint32_t pacman_score;
static uint8_t pacman_lives;
static uint8_t game_initialized;

typedef struct {
    uint8_t row;
    uint8_t column;
    PacmanDirection direction;
    uint8_t released;
    GhostStatus status;
} GhostState;

static GhostState ghosts[GHOST_COUNT];
static uint8_t frightened_ticks;
static uint8_t ghosts_eaten;

static const char board[BOARD_HEIGHT][BOARD_WIDTH + 1] = {
    "############################",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#o####.#####.##.#####.####o#",
    "#.####.#####.##.#####.####.#",
    "#..........................#",
    "#.####.##.########.##.####.#",
    "#.####.##.########.##.####.#",
    "#......##....##....##......#",
    "######.##### ## #####.######",
    "######.##### ## #####.######",
    "######.##          ##.######",
    "######.## ###--### ##.######",
    "######.## #      # ##.######",
    "      .   #      #   .      ",
    "######.## #      # ##.######",
    "######.## ######## ##.######",
    "######.##          ##.######",
    "######.## ######## ##.######",
    "######.## ######## ##.######",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#.####.#####.##.#####.####.#",
    "#o..##................##..o#",
    "###.##.##.########.##.##.###",
    "###.##.##.########.##.##.###",
    "#......##....##....##......#",
    "#.##########.##.##########.#",
    "#.##########.##.##########.#",
    "#..........................#",
    "############################"
};

static const GhostState ghost_start_positions[GHOST_COUNT] = {
    {14, 13, PACMAN_LEFT, 0, GHOST_STATUS_NORMAL},
    {14, 14, PACMAN_RIGHT, 0, GHOST_STATUS_NORMAL},
    {14, 12, PACMAN_LEFT, 0, GHOST_STATUS_NORMAL},
    {14, 15, PACMAN_RIGHT, 0, GHOST_STATUS_NORMAL}
};

#define GHOST_EXIT_ROW 8
#define GHOST_EXIT_COLUMN 12
#define GHOST_CLYDE_CORNER_ROW (BOARD_HEIGHT - 2)
#define GHOST_CLYDE_CORNER_COLUMN 1
#define GHOST_CLYDE_CHASE_DISTANCE 8

static void start_frightened_mode(void)
{
    frightened_ticks = GHOST_FRIGHTENED_TICKS;
    ghosts_eaten = 0;

    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        if (ghosts[ghost].released &&
            ghosts[ghost].status != GHOST_STATUS_EATEN) {
            ghosts[ghost].status = GHOST_STATUS_FLEEING;
        }
    }
}

static void collect_pellet(uint8_t row, uint8_t column)
{
    if (pellets[row][column] == 1) {
        pacman_score += 10;
        pellets[row][column] = 0;
        pellets_remaining--;
    } else if (pellets[row][column] == 2) {
        pacman_score += 50;
        pellets[row][column] = 0;
        pellets_remaining--;
        start_frightened_mode();
    }
}

static void ensure_game_initialized(void)
{
    if (!game_initialized) {
        pacman_start_game();
    }
}

PacmanTile game_get_tile(uint8_t row, uint8_t column)
{
    ensure_game_initialized();

    if (row >= BOARD_HEIGHT || column >= BOARD_WIDTH) {
        return PACMAN_TILE_EMPTY;
    }

    if (board[row][column] == '#') {
        return PACMAN_TILE_WALL;
    }
    if (board[row][column] == '-') {
        return PACMAN_TILE_GATE;
    }
    if (pellets[row][column] == 1) {
        return PACMAN_TILE_PELLET;
    }
    if (pellets[row][column] == 2) {
        return PACMAN_TILE_POWER_PELLET;
    }

    return PACMAN_TILE_EMPTY;
}

static uint8_t tile_is_walkable(uint8_t row, uint8_t column)
{
    char tile = board[row][column];

    return tile != '#' && tile != '-' &&
           (tile != ' ' || row == BOARD_TUNNEL_ROW);
}

static uint8_t next_position(uint8_t current_row, uint8_t current_column,
                             PacmanDirection direction, uint8_t *next_row,
                             uint8_t *next_column, uint8_t ghost_in_house)
{
    *next_row = current_row;
    *next_column = current_column;

    switch (direction) {
        case PACMAN_UP:
            (*next_row)--;
            break;
        case PACMAN_DOWN:
            (*next_row)++;
            break;
        case PACMAN_LEFT:
            if (current_row == BOARD_TUNNEL_ROW && current_column == 0) {
                *next_column = BOARD_WIDTH - 1;
            } else {
                (*next_column)--;
            }
            break;
        case PACMAN_RIGHT:
            if (current_row == BOARD_TUNNEL_ROW &&
                current_column == BOARD_WIDTH - 1) {
                *next_column = 0;
            } else {
                (*next_column)++;
            }
            break;
        default:
            return 0;
    }

    if (*next_row >= BOARD_HEIGHT || *next_column >= BOARD_WIDTH) {
        return 0;
    }

    if (ghost_in_house) {
        return board[*next_row][*next_column] != '#';
    }

    return tile_is_walkable(*next_row, *next_column);
}

static void direction_vector(PacmanDirection direction, int16_t *row,
                             int16_t *column)
{
    *row = 0;
    *column = 0;

    switch (direction) {
        case PACMAN_UP:
            *row = -1;
            break;
        case PACMAN_DOWN:
            *row = 1;
            break;
        case PACMAN_LEFT:
            *column = -1;
            break;
        case PACMAN_RIGHT:
            *column = 1;
            break;
        default:
            break;
    }
}

static void ghost_target(uint8_t ghost, int16_t *target_row,
                         int16_t *target_column)
{
    int16_t direction_row;
    int16_t direction_column;
    int16_t distance;

    direction_vector(pacman_direction, &direction_row,
                     &direction_column);

    if (ghosts[ghost].status == GHOST_STATUS_FLEEING ||
        ghosts[ghost].status == GHOST_STATUS_FLICKERING) {
        *target_row = pacman_row < BOARD_HEIGHT / 2 ? BOARD_HEIGHT - 2 : 1;
        *target_column = pacman_column < BOARD_WIDTH / 2 ? BOARD_WIDTH - 2 : 1;
        return;
    }
    distance = (int16_t)(pacman_row > ghosts[ghost].row
                         ? pacman_row - ghosts[ghost].row
                         : ghosts[ghost].row - pacman_row) +
               (int16_t)(pacman_column > ghosts[ghost].column
                         ? pacman_column - ghosts[ghost].column
                         : ghosts[ghost].column - pacman_column);

    switch (ghost) {
        case 0:
            *target_row = pacman_row;
            *target_column = pacman_column;
            break;
        case 1:
            *target_row = (int16_t)pacman_row + direction_row * 4;
            *target_column = (int16_t)pacman_column + direction_column * 4;
            break;
        case 2:
            *target_row = (int16_t)pacman_row + direction_row * 2;
            *target_column = (int16_t)pacman_column + direction_column * 2;
            *target_row = *target_row * 2 - ghosts[0].row;
            *target_column = *target_column * 2 - ghosts[0].column;
            break;
        case 3:
            if (distance >= GHOST_CLYDE_CHASE_DISTANCE) {
                *target_row = pacman_row;
                *target_column = pacman_column;
            } else {
                *target_row = GHOST_CLYDE_CORNER_ROW;
                *target_column = GHOST_CLYDE_CORNER_COLUMN;
            }
            break;
        default:
            *target_row = pacman_row;
            *target_column = pacman_column;
            break;
    }
}

static PacmanDirection ghost_release_direction(uint8_t row, uint8_t column)
{
    if (row > 12) {
        return PACMAN_UP;
    }
    if (row == 12 && column < 13) {
        return PACMAN_RIGHT;
    }
    if (row == 12 && column > 13) {
        return PACMAN_LEFT;
    }
    if (row == 11 && column > 12) {
        return PACMAN_LEFT;
    }
    if (column == 13 || column == 12) {
        return PACMAN_UP;
    }

    return PACMAN_NONE;
}

static void update_frightened_mode(void)
{
    if (frightened_ticks == 0) {
        return;
    }

    frightened_ticks--;
    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        if (ghosts[ghost].status == GHOST_STATUS_FLEEING ||
            ghosts[ghost].status == GHOST_STATUS_FLICKERING) {
            ghosts[ghost].status = frightened_ticks <= GHOST_FLICKER_TICKS
                ? GHOST_STATUS_FLICKERING : GHOST_STATUS_FLEEING;
        }
    }

    if (frightened_ticks == 0) {
        for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
            if (ghosts[ghost].status == GHOST_STATUS_FLICKERING) {
                ghosts[ghost].status = GHOST_STATUS_NORMAL;
            }
        }
    }
}

static PacmanDirection ghost_path_direction(uint8_t start_row,
                                            uint8_t start_column,
                                            int16_t target_row,
                                            int16_t target_column)
{
    static const PacmanDirection directions[] = {
        PACMAN_UP, PACMAN_LEFT, PACMAN_DOWN, PACMAN_RIGHT
    };
    uint8_t visited[BOARD_HEIGHT][BOARD_WIDTH] = {{0}};
    PacmanDirection first_direction[BOARD_HEIGHT][BOARD_WIDTH];
    uint8_t queue_rows[BOARD_HEIGHT * BOARD_WIDTH];
    uint8_t queue_columns[BOARD_HEIGHT * BOARD_WIDTH];
    uint16_t queue_head = 0;
    uint16_t queue_tail = 0;
    uint8_t goal_row;
    uint8_t goal_column;

    if (target_row < 0) {
        goal_row = 0;
    } else if (target_row >= BOARD_HEIGHT) {
        goal_row = BOARD_HEIGHT - 1;
    } else {
        goal_row = (uint8_t)target_row;
    }
    if (target_column < 0) {
        goal_column = 0;
    } else if (target_column >= BOARD_WIDTH) {
        goal_column = BOARD_WIDTH - 1;
    } else {
        goal_column = (uint8_t)target_column;
    }

    if (!tile_is_walkable(goal_row, goal_column)) {
        uint16_t closest_distance = UINT16_MAX;

        for (uint8_t row = 0; row < BOARD_HEIGHT; row++) {
            for (uint8_t column = 0; column < BOARD_WIDTH; column++) {
                uint16_t distance;

                if (!tile_is_walkable(row, column)) {
                    continue;
                }
                distance = (uint16_t)(row > goal_row
                                      ? row - goal_row : goal_row - row) +
                           (uint16_t)(column > goal_column
                                      ? column - goal_column
                                      : goal_column - column);
                if (distance < closest_distance) {
                    closest_distance = distance;
                    goal_row = row;
                    goal_column = column;
                }
            }
        }
    }

    visited[start_row][start_column] = 1;
    first_direction[start_row][start_column] = PACMAN_NONE;
    queue_rows[queue_tail] = start_row;
    queue_columns[queue_tail] = start_column;
    queue_tail++;

    while (queue_head < queue_tail) {
        uint8_t row = queue_rows[queue_head];
        uint8_t column = queue_columns[queue_head];
        queue_head++;

        if (row == goal_row && column == goal_column) {
            return first_direction[row][column];
        }

        for (uint8_t direction_index = 0;
             direction_index < sizeof(directions) / sizeof(directions[0]);
             direction_index++) {
            uint8_t next_row;
            uint8_t next_column;
            PacmanDirection direction = directions[direction_index];

            if (!next_position(row, column, direction, &next_row,
                               &next_column, 0) ||
                visited[next_row][next_column]) {
                continue;
            }

            visited[next_row][next_column] = 1;
            first_direction[next_row][next_column] =
                row == start_row && column == start_column
                    ? direction : first_direction[row][column];
            queue_rows[queue_tail] = next_row;
            queue_columns[queue_tail] = next_column;
            queue_tail++;
        }
    }

    return PACMAN_NONE;
}

uint8_t pacman_move(PacmanDirection direction)
{
    uint8_t row = pacman_row;
    uint8_t column = pacman_column;
    uint8_t next_row;
    uint8_t next_column;

    ensure_game_initialized();

    if (next_position(row, column, direction, &next_row, &next_column, 0)) {
        pacman_row = next_row;
        pacman_column = next_column;
        pacman_direction = direction;
        collect_pellet(next_row, next_column);
        return 1;
    }

    return 0;
}

void pacman_start_game(void)
{
    pellets_remaining = 0;
    pacman_score = 0;
    pacman_lives = PACMAN_STARTING_LIVES;
    frightened_ticks = 0;
    ghosts_eaten = 0;

    for (uint8_t row = 0; row < BOARD_HEIGHT; row++) {
        for (uint8_t column = 0; column < BOARD_WIDTH; column++) {
            if (board[row][column] == '.') {
                pellets[row][column] = 1;
                pellets_remaining++;
            } else if (board[row][column] == 'o') {
                pellets[row][column] = 2;
                pellets_remaining++;
            } else {
                pellets[row][column] = 0;
            }
        }
    }

    game_initialized = 1;
    pacman_reset();
    collect_pellet(pacman_row, pacman_column);
    ghosts_reset();
}

void pacman_reset(void)
{
    pacman_row = 23;
    pacman_column = 13;
    pacman_direction = PACMAN_NONE;
}

void pacman_lose_life(void)
{
    ensure_game_initialized();

    if (pacman_lives > 0) {
        pacman_lives--;
    }

    pacman_reset();
}

void pacman_get_position(uint8_t *row, uint8_t *column)
{
    *row = pacman_row;
    *column = pacman_column;
}

void ghosts_reset(void)
{
    frightened_ticks = 0;
    ghosts_eaten = 0;
    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        ghosts[ghost] = ghost_start_positions[ghost];
        ghosts[ghost].status = GHOST_STATUS_NORMAL;
    }
}

void ghosts_update(void)
{
    ensure_game_initialized();
    update_frightened_mode();

    uint8_t releasing_ghost = GHOST_COUNT;
    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        if (!ghosts[ghost].released) {
            releasing_ghost = ghost;
            break;
        }
    }

    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        if (!ghosts[ghost].released && ghost != releasing_ghost) {
            continue;
        }

        if (ghosts[ghost].status == GHOST_STATUS_EATEN) {
            ghosts[ghost] = ghost_start_positions[ghost];
            ghosts[ghost].status = GHOST_STATUS_NORMAL;
            continue;
        }

        if (!ghosts[ghost].released) {
            PacmanDirection release_direction = ghost_release_direction(
                ghosts[ghost].row, ghosts[ghost].column);
            if (release_direction == PACMAN_UP && ghosts[ghost].row > 12) {
                ghosts[ghost].row--;
            } else if (release_direction == PACMAN_RIGHT) {
                ghosts[ghost].column++;
            } else if (release_direction == PACMAN_LEFT) {
                ghosts[ghost].column--;
            } else if (release_direction == PACMAN_UP &&
                       ghosts[ghost].row > GHOST_EXIT_ROW) {
                ghosts[ghost].row--;
            }
            ghosts[ghost].direction = release_direction;

            if (ghosts[ghost].row == GHOST_EXIT_ROW &&
                ghosts[ghost].column == GHOST_EXIT_COLUMN) {
                ghosts[ghost].released = 1;
            }
            continue;
        }

        uint8_t best_row = ghosts[ghost].row;
        uint8_t best_column = ghosts[ghost].column;
        PacmanDirection best_direction = ghosts[ghost].direction;
        int16_t target_row;
        int16_t target_column;

        ghost_target(ghost, &target_row, &target_column);

        best_direction = ghost_path_direction(ghosts[ghost].row,
                                              ghosts[ghost].column,
                                              target_row, target_column);
        if (best_direction != PACMAN_NONE) {
            next_position(ghosts[ghost].row, ghosts[ghost].column,
                          best_direction, &best_row, &best_column, 0);
        }

        ghosts[ghost].row = best_row;
        ghosts[ghost].column = best_column;
        ghosts[ghost].direction = best_direction;
    }
}

void ghost_get_position(uint8_t ghost, uint8_t *row, uint8_t *column)
{
    ensure_game_initialized();

    if (ghost >= GHOST_COUNT) {
        *row = 0;
        *column = 0;
        return;
    }

    *row = ghosts[ghost].row;
    *column = ghosts[ghost].column;
}

GhostStatus ghost_get_status(uint8_t ghost)
{
    ensure_game_initialized();

    if (ghost >= GHOST_COUNT) {
        return GHOST_STATUS_NORMAL;
    }

    return ghosts[ghost].status;
}

uint8_t ghost_is_released(uint8_t ghost)
{
    ensure_game_initialized();

    if (ghost >= GHOST_COUNT) {
        return 0;
    }

    return ghosts[ghost].released;
}

uint8_t ghosts_collide_with_pacman(void)
{
    ensure_game_initialized();

    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        if (ghosts[ghost].row == pacman_row &&
            ghosts[ghost].column == pacman_column) {
            return 1;
        }
    }

    return 0;
}

uint32_t ghosts_handle_collision(void)
{
    for (uint8_t ghost = 0; ghost < GHOST_COUNT; ghost++) {
        if (ghosts[ghost].row != pacman_row ||
            ghosts[ghost].column != pacman_column) {
            continue;
        }

        if (ghosts[ghost].status == GHOST_STATUS_FLEEING ||
            ghosts[ghost].status == GHOST_STATUS_FLICKERING) {
            ghosts_eaten++;
            pacman_score += 100U * ghosts_eaten;
            ghosts[ghost].status = GHOST_STATUS_EATEN;
            ghosts[ghost].row = ghost_start_positions[ghost].row;
            ghosts[ghost].column = ghost_start_positions[ghost].column;
            ghosts[ghost].direction = ghost_start_positions[ghost].direction;
            ghosts[ghost].released = 1;
            return 100U * ghosts_eaten;
        }

        if (ghosts[ghost].status == GHOST_STATUS_NORMAL) {
            return 0;
        }
    }

    return 0;
}

uint32_t pacman_get_score(void)
{
    ensure_game_initialized();
    return pacman_score;
}

uint8_t pacman_get_lives(void)
{
    ensure_game_initialized();
    return pacman_lives;
}

uint16_t pacman_get_pellets_remaining(void)
{
    ensure_game_initialized();
    return pellets_remaining;
}

uint8_t pacman_level_complete(void)
{
    ensure_game_initialized();
    return pellets_remaining == 0;
}
