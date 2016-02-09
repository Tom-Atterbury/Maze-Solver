/**
 * @addtogroup common Common
 * @brief     Common headers needed by the maze programs
 * @{
 */
/**
 * @file      maze_types.h
 * @author    Tom Atterbury
 * @brief     Basic types used for maze programs
 */


#ifndef MAZE_TYPES_H
#define MAZE_TYPES_H

#include <stdint.h>

/// Maze components
typedef enum {
  WALL = '#', 
  BLANK = ' ', 
  START = 'S', 
  GOAL = 'G', 
  VISIT = '!', 
  WRONG = '*', 
  PATH = 'o'
} maze_component_t;

/// BFS State enumeration
typedef enum{
  UNDISCOVERED,
  DISCOVERED,
  PROCESSED
} state_t;
            
/// Maze Cell struct definition
typedef struct maze_cell{
  maze_component_t type;
  state_t state;
  int parent[2];
} maze_cell_t;

/// The maze struct definition
typedef struct maze {
  maze_cell_t** cells;
  int width;
  int height;
  int startX;
  int startY;
  int goalX;
  int goalY;
} maze_t;

/// Directions
typedef enum {NORTH, EAST, SOUTH, WEST} dir_t;

#endif

