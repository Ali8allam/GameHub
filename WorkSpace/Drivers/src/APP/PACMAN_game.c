/*
 * PACMAN_game.c
 *
 * Pac-Man game engine. Pure logic, no hardware access - see PACMAN_game_int.h.
 *
 * Ported from the standalone prototype in pacman/src/game.c. The rules are
 * unchanged; the differences are that it uses the project's own integer types
 * and naming convention, and that the breadth-first ghost pathfinder keeps its
 * scratch buffers in .bss instead of on the stack (they are ~3.4 KB and the
 * main stack is 1 KB - see ldscripts/sections.ld).
 */

#include "../LIB/STD_TYPES.h"

#include "PACMAN_game_int.h"

/* ------------------------------------------------------------------------ */
/* State                                                                     */
/* ------------------------------------------------------------------------ */

static u8 s_u8PacManRow = 23;
static u8 s_u8PacManColumn = 13;
static PacmanDirection s_xPacManDirection = PACMAN_NONE;

/* 0 = nothing, 1 = pellet, 2 = power pellet. */
static u8 s_u8Pellets[BOARD_HEIGHT][BOARD_WIDTH];
static u16 s_u16PelletsRemaining;
static u32 s_u32Score;
static u8 s_u8Lives;
static u8 s_u8GameInitialized;

typedef struct {
    u8 Row;
    u8 Column;
    PacmanDirection Direction;
    u8 Released;
    GhostStatus Status;
} GhostState;

static GhostState s_xGhosts[GHOST_COUNT];
static u8 s_u8FrightenedTicks;
static u8 s_u8GhostsEaten;

/* '#' wall, '.' pellet, 'o' power pellet, '-' ghost-house gate, ' ' void.
 * The blanks on BOARD_TUNNEL_ROW are the only walkable blanks. */
static const char s_cBoard[BOARD_HEIGHT][BOARD_WIDTH + 1] = {
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

static const GhostState s_xGhostStartPositions[GHOST_COUNT] = {
    {14, 13, PACMAN_LEFT,  0, GHOST_STATUS_NORMAL},
    {14, 14, PACMAN_RIGHT, 0, GHOST_STATUS_NORMAL},
    {14, 12, PACMAN_LEFT,  0, GHOST_STATUS_NORMAL},
    {14, 15, PACMAN_RIGHT, 0, GHOST_STATUS_NORMAL}
};

#define GHOST_EXIT_ROW              8
#define GHOST_EXIT_COLUMN           12
#define GHOST_CLYDE_CORNER_ROW      (BOARD_HEIGHT - 2)
#define GHOST_CLYDE_CORNER_COLUMN   1
#define GHOST_CLYDE_CHASE_DISTANCE  8

/* ------------------------------------------------------------------------ */
/* Board helpers                                                             */
/* ------------------------------------------------------------------------ */

static void pacman_vStartFrightenedMode(void)
{
    s_u8FrightenedTicks = GHOST_FRIGHTENED_TICKS;
    s_u8GhostsEaten = 0;

    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        if (s_xGhosts[ghost].Released &&
            (s_xGhosts[ghost].Status != GHOST_STATUS_EATEN))
        {
            s_xGhosts[ghost].Status = GHOST_STATUS_FLEEING;
        }
    }
}

static void pacman_vCollectPellet(u8 A_u8Row, u8 A_u8Column)
{
    if (s_u8Pellets[A_u8Row][A_u8Column] == 1)
    {
        s_u32Score += 10;
        s_u8Pellets[A_u8Row][A_u8Column] = 0;
        s_u16PelletsRemaining--;
    }
    else if (s_u8Pellets[A_u8Row][A_u8Column] == 2)
    {
        s_u32Score += 50;
        s_u8Pellets[A_u8Row][A_u8Column] = 0;
        s_u16PelletsRemaining--;
        pacman_vStartFrightenedMode();
    }
}

static void pacman_vEnsureInitialized(void)
{
    if (!s_u8GameInitialized)
    {
        PacManGame_vStartGame();
    }
}

static u8 pacman_u8TileIsWalkable(u8 A_u8Row, u8 A_u8Column)
{
    char tile = s_cBoard[A_u8Row][A_u8Column];

    return (tile != '#') && (tile != '-') &&
           ((tile != ' ') || (A_u8Row == BOARD_TUNNEL_ROW));
}

/* Step one tile from (row, column) in the given direction.
 * Returns 1 and fills the outputs when the destination can be entered.
 * A_u8GhostInHouse relaxes the check to "not a wall", which is what lets a
 * ghost walk out through the gate and across the void tiles of the house. */
static u8 pacman_u8NextPosition(u8 A_u8CurrentRow, u8 A_u8CurrentColumn,
                                PacmanDirection A_xDirection, u8 *A_pu8NextRow,
                                u8 *A_pu8NextColumn, u8 A_u8GhostInHouse)
{
    *A_pu8NextRow = A_u8CurrentRow;
    *A_pu8NextColumn = A_u8CurrentColumn;

    switch (A_xDirection)
    {
        case PACMAN_UP:
            (*A_pu8NextRow)--;
            break;

        case PACMAN_DOWN:
            (*A_pu8NextRow)++;
            break;

        case PACMAN_LEFT:
            if ((A_u8CurrentRow == BOARD_TUNNEL_ROW) && (A_u8CurrentColumn == 0))
            {
                *A_pu8NextColumn = BOARD_WIDTH - 1;
            }
            else
            {
                (*A_pu8NextColumn)--;
            }
            break;

        case PACMAN_RIGHT:
            if ((A_u8CurrentRow == BOARD_TUNNEL_ROW) &&
                (A_u8CurrentColumn == BOARD_WIDTH - 1))
            {
                *A_pu8NextColumn = 0;
            }
            else
            {
                (*A_pu8NextColumn)++;
            }
            break;

        default:
            return 0;
    }

    /* Stepping off the top or left edge wraps the u8 around, so one unsigned
     * range check catches both edges. */
    if ((*A_pu8NextRow >= BOARD_HEIGHT) || (*A_pu8NextColumn >= BOARD_WIDTH))
    {
        return 0;
    }

    if (A_u8GhostInHouse)
    {
        return s_cBoard[*A_pu8NextRow][*A_pu8NextColumn] != '#';
    }

    return pacman_u8TileIsWalkable(*A_pu8NextRow, *A_pu8NextColumn);
}

static void pacman_vDirectionVector(PacmanDirection A_xDirection, s16 *A_ps16Row,
                                    s16 *A_ps16Column)
{
    *A_ps16Row = 0;
    *A_ps16Column = 0;

    switch (A_xDirection)
    {
        case PACMAN_UP:     *A_ps16Row = -1;    break;
        case PACMAN_DOWN:   *A_ps16Row = 1;     break;
        case PACMAN_LEFT:   *A_ps16Column = -1; break;
        case PACMAN_RIGHT:  *A_ps16Column = 1;  break;
        default:                                break;
    }
}

/* ------------------------------------------------------------------------ */
/* Ghost AI                                                                  */
/* ------------------------------------------------------------------------ */

/* Classic targeting: Blinky chases directly, Pinky aims 4 tiles ahead, Inky
 * mirrors Blinky through a point 2 tiles ahead, Clyde chases until he gets
 * close and then retreats to his corner. Frightened ghosts run to the corner
 * furthest from Pac-Man. */
static void pacman_vGhostTarget(u8 A_u8Ghost, s16 *A_ps16TargetRow,
                                s16 *A_ps16TargetColumn)
{
    s16 directionRow;
    s16 directionColumn;
    s16 distance;

    pacman_vDirectionVector(s_xPacManDirection, &directionRow, &directionColumn);

    if ((s_xGhosts[A_u8Ghost].Status == GHOST_STATUS_FLEEING) ||
        (s_xGhosts[A_u8Ghost].Status == GHOST_STATUS_FLICKERING))
    {
        *A_ps16TargetRow = (s_u8PacManRow < BOARD_HEIGHT / 2)
                           ? (BOARD_HEIGHT - 2) : 1;
        *A_ps16TargetColumn = (s_u8PacManColumn < BOARD_WIDTH / 2)
                              ? (BOARD_WIDTH - 2) : 1;
        return;
    }

    distance = (s16)((s_u8PacManRow > s_xGhosts[A_u8Ghost].Row)
                     ? (s_u8PacManRow - s_xGhosts[A_u8Ghost].Row)
                     : (s_xGhosts[A_u8Ghost].Row - s_u8PacManRow)) +
               (s16)((s_u8PacManColumn > s_xGhosts[A_u8Ghost].Column)
                     ? (s_u8PacManColumn - s_xGhosts[A_u8Ghost].Column)
                     : (s_xGhosts[A_u8Ghost].Column - s_u8PacManColumn));

    switch (A_u8Ghost)
    {
        case 0:
            *A_ps16TargetRow = s_u8PacManRow;
            *A_ps16TargetColumn = s_u8PacManColumn;
            break;

        case 1:
            *A_ps16TargetRow = (s16)s_u8PacManRow + directionRow * 4;
            *A_ps16TargetColumn = (s16)s_u8PacManColumn + directionColumn * 4;
            break;

        case 2:
            *A_ps16TargetRow = (s16)s_u8PacManRow + directionRow * 2;
            *A_ps16TargetColumn = (s16)s_u8PacManColumn + directionColumn * 2;
            *A_ps16TargetRow = *A_ps16TargetRow * 2 - s_xGhosts[0].Row;
            *A_ps16TargetColumn = *A_ps16TargetColumn * 2 - s_xGhosts[0].Column;
            break;

        case 3:
            if (distance >= GHOST_CLYDE_CHASE_DISTANCE)
            {
                *A_ps16TargetRow = s_u8PacManRow;
                *A_ps16TargetColumn = s_u8PacManColumn;
            }
            else
            {
                *A_ps16TargetRow = GHOST_CLYDE_CORNER_ROW;
                *A_ps16TargetColumn = GHOST_CLYDE_CORNER_COLUMN;
            }
            break;

        default:
            *A_ps16TargetRow = s_u8PacManRow;
            *A_ps16TargetColumn = s_u8PacManColumn;
            break;
    }
}

/* Scripted walk out of the ghost house up to (GHOST_EXIT_ROW, _COLUMN). */
static PacmanDirection pacman_xGhostReleaseDirection(u8 A_u8Row, u8 A_u8Column)
{
    if (A_u8Row > 12)
    {
        return PACMAN_UP;
    }
    if ((A_u8Row == 12) && (A_u8Column < 13))
    {
        return PACMAN_RIGHT;
    }
    if ((A_u8Row == 12) && (A_u8Column > 13))
    {
        return PACMAN_LEFT;
    }
    if ((A_u8Row == 11) && (A_u8Column > 12))
    {
        return PACMAN_LEFT;
    }
    if ((A_u8Column == 13) || (A_u8Column == 12))
    {
        return PACMAN_UP;
    }

    return PACMAN_NONE;
}

static void pacman_vUpdateFrightenedMode(void)
{
    if (s_u8FrightenedTicks == 0)
    {
        return;
    }

    s_u8FrightenedTicks--;

    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        if ((s_xGhosts[ghost].Status == GHOST_STATUS_FLEEING) ||
            (s_xGhosts[ghost].Status == GHOST_STATUS_FLICKERING))
        {
            s_xGhosts[ghost].Status =
                (s_u8FrightenedTicks <= GHOST_FLICKER_TICKS)
                ? GHOST_STATUS_FLICKERING : GHOST_STATUS_FLEEING;
        }
    }

    if (s_u8FrightenedTicks == 0)
    {
        for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
        {
            if (s_xGhosts[ghost].Status == GHOST_STATUS_FLICKERING)
            {
                s_xGhosts[ghost].Status = GHOST_STATUS_NORMAL;
            }
        }
    }
}

/* Breadth-first search scratch space.
 *
 * These are static on purpose: as locals they add up to about 3.4 KB, and the
 * main stack is only 1 KB (ldscripts/sections.ld). The pathfinder is never
 * re-entered - it is called from PacManGame_vGhostsUpdate() only, never from
 * an interrupt - so sharing one buffer between ghosts is safe. */
static u8 s_u8BfsVisited[BOARD_HEIGHT][BOARD_WIDTH];
static u8 s_u8BfsFirstDirection[BOARD_HEIGHT][BOARD_WIDTH];
static u8 s_u8BfsQueueRows[BOARD_HEIGHT * BOARD_WIDTH];
static u8 s_u8BfsQueueColumns[BOARD_HEIGHT * BOARD_WIDTH];

/* First step of a shortest path from (start) to (target), or PACMAN_NONE when
 * the target is unreachable. Targets that fall outside the maze or land on a
 * wall are snapped to the nearest walkable tile first. */
static PacmanDirection pacman_xGhostPathDirection(u8 A_u8StartRow,
                                                  u8 A_u8StartColumn,
                                                  s16 A_s16TargetRow,
                                                  s16 A_s16TargetColumn)
{
    static const PacmanDirection directions[] = {
        PACMAN_UP, PACMAN_LEFT, PACMAN_DOWN, PACMAN_RIGHT
    };
    u8 *visitedFlat = &s_u8BfsVisited[0][0];
    u16 queueHead = 0;
    u16 queueTail = 0;
    u8 goalRow;
    u8 goalColumn;

    for (u16 i = 0; i < (u16)(BOARD_HEIGHT * BOARD_WIDTH); i++)
    {
        visitedFlat[i] = 0;
    }

    /* Clamp first: A_s16TargetRow/Column are signed and may sit off-board. */
    if (A_s16TargetRow < 0)
    {
        goalRow = 0;
    }
    else if (A_s16TargetRow >= BOARD_HEIGHT)
    {
        goalRow = BOARD_HEIGHT - 1;
    }
    else
    {
        goalRow = (u8)A_s16TargetRow;
    }

    if (A_s16TargetColumn < 0)
    {
        goalColumn = 0;
    }
    else if (A_s16TargetColumn >= BOARD_WIDTH)
    {
        goalColumn = BOARD_WIDTH - 1;
    }
    else
    {
        goalColumn = (u8)A_s16TargetColumn;
    }

    if (!pacman_u8TileIsWalkable(goalRow, goalColumn))
    {
        u16 closestDistance = 0xFFFFU;

        for (u8 row = 0; row < BOARD_HEIGHT; row++)
        {
            for (u8 column = 0; column < BOARD_WIDTH; column++)
            {
                u16 distance;

                if (!pacman_u8TileIsWalkable(row, column))
                {
                    continue;
                }

                distance = (u16)((row > goalRow) ? (row - goalRow)
                                                 : (goalRow - row)) +
                           (u16)((column > goalColumn) ? (column - goalColumn)
                                                       : (goalColumn - column));
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    goalRow = row;
                    goalColumn = column;
                }
            }
        }
    }

    s_u8BfsVisited[A_u8StartRow][A_u8StartColumn] = 1;
    s_u8BfsFirstDirection[A_u8StartRow][A_u8StartColumn] = (u8)PACMAN_NONE;
    s_u8BfsQueueRows[queueTail] = A_u8StartRow;
    s_u8BfsQueueColumns[queueTail] = A_u8StartColumn;
    queueTail++;

    while (queueHead < queueTail)
    {
        u8 row = s_u8BfsQueueRows[queueHead];
        u8 column = s_u8BfsQueueColumns[queueHead];
        queueHead++;

        if ((row == goalRow) && (column == goalColumn))
        {
            return (PacmanDirection)s_u8BfsFirstDirection[row][column];
        }

        for (u8 directionIndex = 0;
             directionIndex < (u8)(sizeof(directions) / sizeof(directions[0]));
             directionIndex++)
        {
            u8 nextRow;
            u8 nextColumn;
            PacmanDirection direction = directions[directionIndex];

            if (!pacman_u8NextPosition(row, column, direction, &nextRow,
                                       &nextColumn, 0) ||
                s_u8BfsVisited[nextRow][nextColumn])
            {
                continue;
            }

            s_u8BfsVisited[nextRow][nextColumn] = 1;
            s_u8BfsFirstDirection[nextRow][nextColumn] =
                ((row == A_u8StartRow) && (column == A_u8StartColumn))
                ? (u8)direction : s_u8BfsFirstDirection[row][column];
            s_u8BfsQueueRows[queueTail] = nextRow;
            s_u8BfsQueueColumns[queueTail] = nextColumn;
            queueTail++;
        }
    }

    return PACMAN_NONE;
}

/* ------------------------------------------------------------------------ */
/* Pac-Man                                                                   */
/* ------------------------------------------------------------------------ */

PacmanTile PacManGame_xGetTile(u8 A_u8Row, u8 A_u8Column)
{
    pacman_vEnsureInitialized();

    if ((A_u8Row >= BOARD_HEIGHT) || (A_u8Column >= BOARD_WIDTH))
    {
        return PACMAN_TILE_EMPTY;
    }

    if (s_cBoard[A_u8Row][A_u8Column] == '#')
    {
        return PACMAN_TILE_WALL;
    }
    if (s_cBoard[A_u8Row][A_u8Column] == '-')
    {
        return PACMAN_TILE_GATE;
    }
    if (s_u8Pellets[A_u8Row][A_u8Column] == 1)
    {
        return PACMAN_TILE_PELLET;
    }
    if (s_u8Pellets[A_u8Row][A_u8Column] == 2)
    {
        return PACMAN_TILE_POWER_PELLET;
    }

    return PACMAN_TILE_EMPTY;
}

u8 PacManGame_u8Move(PacmanDirection A_xDirection)
{
    u8 nextRow;
    u8 nextColumn;

    pacman_vEnsureInitialized();

    if (pacman_u8NextPosition(s_u8PacManRow, s_u8PacManColumn, A_xDirection,
                              &nextRow, &nextColumn, 0))
    {
        s_u8PacManRow = nextRow;
        s_u8PacManColumn = nextColumn;
        s_xPacManDirection = A_xDirection;
        pacman_vCollectPellet(nextRow, nextColumn);
        return 1;
    }

    return 0;
}

void PacManGame_vStartGame(void)
{
    s_u16PelletsRemaining = 0;
    s_u32Score = 0;
    s_u8Lives = PACMAN_STARTING_LIVES;
    s_u8FrightenedTicks = 0;
    s_u8GhostsEaten = 0;

    for (u8 row = 0; row < BOARD_HEIGHT; row++)
    {
        for (u8 column = 0; column < BOARD_WIDTH; column++)
        {
            if (s_cBoard[row][column] == '.')
            {
                s_u8Pellets[row][column] = 1;
                s_u16PelletsRemaining++;
            }
            else if (s_cBoard[row][column] == 'o')
            {
                s_u8Pellets[row][column] = 2;
                s_u16PelletsRemaining++;
            }
            else
            {
                s_u8Pellets[row][column] = 0;
            }
        }
    }

    /* Set before the calls below: they all go through
     * pacman_vEnsureInitialized() and would otherwise recurse. */
    s_u8GameInitialized = 1;

    PacManGame_vReset();
    pacman_vCollectPellet(s_u8PacManRow, s_u8PacManColumn);
    PacManGame_vGhostsReset();
}

void PacManGame_vReset(void)
{
    s_u8PacManRow = 23;
    s_u8PacManColumn = 13;
    s_xPacManDirection = PACMAN_NONE;
}

void PacManGame_vLoseLife(void)
{
    pacman_vEnsureInitialized();

    if (s_u8Lives > 0)
    {
        s_u8Lives--;
    }

    PacManGame_vReset();
}

void PacManGame_vGetPosition(u8 *A_pu8Row, u8 *A_pu8Column)
{
    *A_pu8Row = s_u8PacManRow;
    *A_pu8Column = s_u8PacManColumn;
}

u32 PacManGame_u32GetScore(void)
{
    pacman_vEnsureInitialized();
    return s_u32Score;
}

u8 PacManGame_u8GetLives(void)
{
    pacman_vEnsureInitialized();
    return s_u8Lives;
}

u16 PacManGame_u16GetPelletsRemaining(void)
{
    pacman_vEnsureInitialized();
    return s_u16PelletsRemaining;
}

u8 PacManGame_u8LevelComplete(void)
{
    pacman_vEnsureInitialized();
    return (s_u16PelletsRemaining == 0);
}

u8 PacManGame_u8IsFrightenedMode(void)
{
    pacman_vEnsureInitialized();
    return (s_u8FrightenedTicks > 0);
}

/* ------------------------------------------------------------------------ */
/* Ghosts                                                                    */
/* ------------------------------------------------------------------------ */

void PacManGame_vGhostsReset(void)
{
    s_u8FrightenedTicks = 0;
    s_u8GhostsEaten = 0;

    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        s_xGhosts[ghost] = s_xGhostStartPositions[ghost];
        s_xGhosts[ghost].Status = GHOST_STATUS_NORMAL;
    }
}

void PacManGame_vGhostsUpdate(void)
{
    u8 releasingGhost = GHOST_COUNT;

    pacman_vEnsureInitialized();
    pacman_vUpdateFrightenedMode();

    /* Only one ghost leaves the house at a time. */
    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        if (!s_xGhosts[ghost].Released)
        {
            releasingGhost = ghost;
            break;
        }
    }

    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        u8 bestRow;
        u8 bestColumn;
        PacmanDirection bestDirection;
        s16 targetRow;
        s16 targetColumn;

        if (!s_xGhosts[ghost].Released && (ghost != releasingGhost))
        {
            continue;
        }

        if (s_xGhosts[ghost].Status == GHOST_STATUS_EATEN)
        {
            s_xGhosts[ghost] = s_xGhostStartPositions[ghost];
            s_xGhosts[ghost].Status = GHOST_STATUS_NORMAL;
            continue;
        }

        if (!s_xGhosts[ghost].Released)
        {
            PacmanDirection releaseDirection = pacman_xGhostReleaseDirection(
                s_xGhosts[ghost].Row, s_xGhosts[ghost].Column);

            if ((releaseDirection == PACMAN_UP) && (s_xGhosts[ghost].Row > 12))
            {
                s_xGhosts[ghost].Row--;
            }
            else if (releaseDirection == PACMAN_RIGHT)
            {
                s_xGhosts[ghost].Column++;
            }
            else if (releaseDirection == PACMAN_LEFT)
            {
                s_xGhosts[ghost].Column--;
            }
            else if ((releaseDirection == PACMAN_UP) &&
                     (s_xGhosts[ghost].Row > GHOST_EXIT_ROW))
            {
                s_xGhosts[ghost].Row--;
            }

            s_xGhosts[ghost].Direction = releaseDirection;

            if ((s_xGhosts[ghost].Row == GHOST_EXIT_ROW) &&
                (s_xGhosts[ghost].Column == GHOST_EXIT_COLUMN))
            {
                s_xGhosts[ghost].Released = 1;
            }

            continue;
        }

        /* Frightened ghosts move 30% slower: skip 3 of every 10 updates. */
        if ((s_xGhosts[ghost].Status == GHOST_STATUS_FLEEING) ||
            (s_xGhosts[ghost].Status == GHOST_STATUS_FLICKERING))
        {
            if ((s_u8FrightenedTicks % 10 == 3) ||
                (s_u8FrightenedTicks % 10 == 6) ||
                (s_u8FrightenedTicks % 10 == 9))
            {
                continue;
            }
        }

        bestRow = s_xGhosts[ghost].Row;
        bestColumn = s_xGhosts[ghost].Column;

        pacman_vGhostTarget(ghost, &targetRow, &targetColumn);

        bestDirection = pacman_xGhostPathDirection(s_xGhosts[ghost].Row,
                                                   s_xGhosts[ghost].Column,
                                                   targetRow, targetColumn);
        if (bestDirection != PACMAN_NONE)
        {
            pacman_u8NextPosition(s_xGhosts[ghost].Row, s_xGhosts[ghost].Column,
                                  bestDirection, &bestRow, &bestColumn, 0);
        }

        s_xGhosts[ghost].Row = bestRow;
        s_xGhosts[ghost].Column = bestColumn;
        s_xGhosts[ghost].Direction = bestDirection;
    }
}

void PacManGame_vGhostGetPosition(u8 A_u8Ghost, u8 *A_pu8Row, u8 *A_pu8Column)
{
    pacman_vEnsureInitialized();

    if (A_u8Ghost >= GHOST_COUNT)
    {
        *A_pu8Row = 0;
        *A_pu8Column = 0;
        return;
    }

    *A_pu8Row = s_xGhosts[A_u8Ghost].Row;
    *A_pu8Column = s_xGhosts[A_u8Ghost].Column;
}

GhostStatus PacManGame_xGhostGetStatus(u8 A_u8Ghost)
{
    pacman_vEnsureInitialized();

    if (A_u8Ghost >= GHOST_COUNT)
    {
        return GHOST_STATUS_NORMAL;
    }

    return s_xGhosts[A_u8Ghost].Status;
}

u8 PacManGame_u8GhostIsReleased(u8 A_u8Ghost)
{
    pacman_vEnsureInitialized();

    if (A_u8Ghost >= GHOST_COUNT)
    {
        return 0;
    }

    return s_xGhosts[A_u8Ghost].Released;
}

u8 PacManGame_u8GhostsCollideWithPacMan(void)
{
    pacman_vEnsureInitialized();

    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        if ((s_xGhosts[ghost].Status != GHOST_STATUS_EATEN) &&
            (s_xGhosts[ghost].Row == s_u8PacManRow) &&
            (s_xGhosts[ghost].Column == s_u8PacManColumn))
        {
            return 1;
        }
    }

    return 0;
}

u32 PacManGame_u32GhostsHandleCollision(void)
{
    u32 totalPoints = 0;
    u8 normalHit = 0;

    for (u8 ghost = 0; ghost < GHOST_COUNT; ghost++)
    {
        if ((s_xGhosts[ghost].Row != s_u8PacManRow) ||
            (s_xGhosts[ghost].Column != s_u8PacManColumn))
        {
            continue;
        }

        if ((s_xGhosts[ghost].Status == GHOST_STATUS_FLEEING) ||
            (s_xGhosts[ghost].Status == GHOST_STATUS_FLICKERING))
        {
            u32 points;

            s_u8GhostsEaten++;
            /* Classic Pac-Man: 200, 400, 800, 1600 for successive ghosts. */
            points = 200UL << (s_u8GhostsEaten - 1U);
            s_u32Score += points;

            s_xGhosts[ghost].Status = GHOST_STATUS_EATEN;
            s_xGhosts[ghost].Row = s_xGhostStartPositions[ghost].Row;
            s_xGhosts[ghost].Column = s_xGhostStartPositions[ghost].Column;
            s_xGhosts[ghost].Direction = s_xGhostStartPositions[ghost].Direction;
            s_xGhosts[ghost].Released = 0;

            totalPoints += points;
        }
        else if (s_xGhosts[ghost].Status == GHOST_STATUS_NORMAL)
        {
            /* A normal ghost caught Pac-Man: the caller must take a life. */
            normalHit = 1;
        }
    }

    if (normalHit)
    {
        return 0;
    }

    return totalPoints;
}
