#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "maze_types.h"
#include <semaphore.h>

#define DEBUG 0
#define MAX_THREADS 4096

// Custom type for in-maze solvers
typedef struct cursor {
  //Position
  int x;
  int y;
  
  // Facing direction
  dir_t facing;

  // Original facing direction after a move
  dir_t face_origin;

} cursor_t;

// Custom type for threaded maze solvers
typedef struct cursor_alt {
  //Position
  int x;
  int y;
  
  // Facing direction
  dir_t facing;

  // Function return value
  maze_component_t type;
} cursor2_t;

// The maze
maze_t maze;

sem_t type_sem;
sem_t thread_sem;
pthread_mutex_t type_lock;

/**
 * @brief     Returns the cell indices on the cursor's right
 */
int* right_hand(cursor_t *me){
  int* next = malloc(2*sizeof(int));
  next[0] = me->x;
  next[1] = me->y;
  
  switch(me->facing){
    case NORTH:
      next[0]++;
      return next;
    case EAST:
      next[1]++;
      return next;
    case SOUTH:
      next[0]--;
      return next;     
    case WEST:
      next[1]--;
      return next;
    default:
      perror("Illegal facing state");
      exit(0);
  }

  return NULL;
}

/**
 * @brief     Turns the cursor to the left
 */
void turn_left(cursor_t *me){
  if(DEBUG) printf("%d turning left ", me->facing);
  switch(me->facing){
    case NORTH:
      me->facing = WEST;
      if(DEBUG) printf("%d\n",me->facing);
      return;
    case EAST:
      me->facing = NORTH;
      if(DEBUG) printf("%d\n",me->facing);
      return;
    case SOUTH:
      me->facing = EAST;
      if(DEBUG) printf("%d\n",me->facing);
      return;
    case WEST:
      me->facing = SOUTH;
      if(DEBUG) printf("%d\n",me->facing);
      return;
    default:
      perror("Illegal facing state");
      exit(0);
  }
}

/**
 * @brief     Moves the cursor to the cell on the right, changing the facing
 * direction approriately.
 */
void go_right(cursor_t *me, int *next, int been_there){

  // Set the cell component we are leaving, as long as we aren't at the start
  if(maze.cells[me->y][me->x].type != START) {
    if(been_there)
      maze.cells[me->y][me->x].type = WRONG;
    else
      maze.cells[me->y][me->x].type = VISIT;
  }

  // Turn right
  switch(me->facing){
    case NORTH:
      me->facing = EAST;
      break;
    case EAST:
      me->facing = SOUTH;
      break;
    case SOUTH:
      me->facing = WEST;
      break;
    case WEST:
      me->facing = NORTH;
      break;
    default:
      perror("Illegal facing state");
      exit(0);
  }

  // Then go right
  me->x = next[0];
  me->y = next[1];

  if(DEBUG) printf("Going right: (%d,%d)\n", me->x, me->y);

  // Reset face origin
  me->face_origin = me->facing;
}

/**
 * @brief    Naive Maze Solver - Right Hand Rule
 * A naive solution to solve the maze, follows the right hand rule to determine
 * the path to the goal. Will check if the cell to the right of the current
 * facing direction is empty, if it is we turn in that direction and move into
 * that cell.
 */
int right_hand_maze_solver(){
  /// Naive solution - right hand rule
  // Set initial position and direction to face
  cursor_t me;
  me.x = maze.startX;
  me.y = maze.startY;
  me.facing = me.face_origin = EAST;
  int *on_right;
  int goal_found = 1;
  int moved = 0;

  // Loop until we reach the goal  
  while(maze.cells[me.y][me.x].type != GOAL){
    // Check the cell to the right of the current facing direction
    on_right = right_hand(&me);
    
    // Check if cell to right is within the maze
    if(on_right[0] > -1 && on_right[0] < maze.width && 
       on_right[1] > -1 && on_right[1] < maze.height) {

      // Determine behavior based on cell contents
      switch(maze.cells[on_right[1]][on_right[0]].type){
        case WALL:
        case START:
          // Turn left to check the next cell
          if(DEBUG) printf("Found wall at (%d,%d) turning left.\n",on_right[0],on_right[1]);
          turn_left(&me);
          moved = 0;
          break;
        case GOAL:
        case BLANK:
          // Empty space or goal, go right updating cell we just vacated
          if(DEBUG) printf("Found space at (%d,%d)\n",on_right[0],on_right[1]);
          go_right(&me, on_right, 0);
          moved = 1;
          break;
        case VISIT:
          // Current cell is dead-end, move back and update cell we just vacated
          go_right(&me, on_right, 1);
          moved = 1;
          break;
        case WRONG:
        case PATH:
          // Loop detected
          // Turn left to check the next cell
          if(DEBUG) printf("Loop at (%d,%d) turning left.\n",on_right[0],on_right[1]);
          turn_left(&me);
          moved = 0;
          break;
        default:
          perror("Invalid maze component");
          exit(0);
      }

      if(!moved && me.facing == me.face_origin) {
        // We've gone in a complete circle in the current cell, 
        // Goal is unreachable
        goal_found = 0;
        break;
      }
    } else {
      // Turn left to attempt to 're-enter' the maze
      turn_left(&me);
      moved = 0;
    }
  }

  if(goal_found){
    // Goal was found, update visited cells with path
    // Very clunky way
    // go through entire maze matrix replacing visits with paths
    int i, j;
    for(i = 0; i < maze.height; i++){
      for(j = 0; j < maze.width; j++){
        if(maze.cells[i][j].type == VISIT) maze.cells[i][j].type = PATH;
      }
    }
  }

  return goal_found;    
}

/**
 * @brief      Protected type set of a maze cell for threading implementation
 */
void set_cell_type(int y, int x, maze_component_t type){
  pthread_mutex_lock(&type_lock);
  sem_wait(&type_sem);
  maze.cells[y][x].type = type;
  sem_post(&type_sem);
  pthread_mutex_unlock(&type_lock);
}

/**
 * @brief      Recursive breadth-first search algorithm
 */
void bfs_recur(void* params){
  cursor2_t* node = (cursor2_t*) params;
  int current_y = node->y;
  int current_x = node->x;
  dir_t went = node->facing;

  maze_component_t current_type = maze.cells[current_y][current_x].type;

  if(current_type == WALL || current_type == START || current_type == GOAL || current_type == VISIT){
    node->type = current_type;
    return;
  }

  set_cell_type(current_y,current_x,VISIT);

  cursor2_t* east_params = malloc(sizeof(cursor2_t));
  east_params->y = current_y;
  east_params->x = current_x+1;
  east_params->facing = EAST;

  cursor2_t* south_params = malloc(sizeof(cursor2_t));
  south_params->y = current_y+1;
  south_params->x = current_x;
  south_params->facing = SOUTH;

  cursor2_t* west_params = malloc(sizeof(cursor2_t));
  west_params->y = current_y;
  west_params->x = current_x-1;
  west_params->facing = WEST;

  cursor2_t* north_params = malloc(sizeof(cursor2_t));
  north_params->y = current_y-1;
  north_params->x = current_x;
  north_params->facing = NORTH;

  // Initialize an array of threads
  int i;
	pthread_t threads[3];

  switch(went){
    case NORTH:
      // Start threaded search for goal
//      sem_wait(&thread_sem);
      pthread_create(&threads[0], NULL, bfs_recur, (void *) east_params);
      
  //    sem_wait(&thread_sem);
      pthread_create(&threads[1], NULL, bfs_recur, (void *) west_params);

    //  sem_wait(&thread_sem);
      pthread_create(&threads[2], NULL, bfs_recur, (void *) north_params);

	    // Wait for all threads to finish
	    for(i = 0; i < 3; i++){
		    pthread_join(threads[i], NULL);
	    }

      //sem_post(&thread_sem);
      //sem_post(&thread_sem);
      //sem_post(&thread_sem);

      if(east_params->type == PATH || east_params->type == GOAL ||
         west_params->type == PATH || west_params->type == GOAL ||
         north_params->type == PATH || north_params->type == GOAL) {
        set_cell_type(current_y,current_x,PATH);
        node->type = PATH;
        return; // Found a path/goal
      }
      break;
    case SOUTH:
      // Start threaded search for goal
      //sem_wait(&thread_sem);
      pthread_create(&threads[0], NULL, bfs_recur, (void *) east_params);

      //sem_wait(&thread_sem);
      pthread_create(&threads[1], NULL, bfs_recur, (void *) south_params);

      //sem_wait(&thread_sem);
      pthread_create(&threads[2], NULL, bfs_recur, (void *) west_params);

	    // Wait for all threads to finish
	    for(i = 0; i < 3; i++){
		    pthread_join(threads[i], NULL);
	    }

      //sem_post(&thread_sem);
      //sem_post(&thread_sem);
      //sem_post(&thread_sem);

      if(east_params->type == PATH || east_params->type == GOAL ||
         south_params->type == PATH || south_params->type == GOAL ||
         west_params->type == PATH || west_params->type == GOAL) {
        set_cell_type(current_y,current_x,PATH);
        node->type = PATH;
        return; // Found a path/goal
      }
      break;
    case EAST:
      // Start threaded search for goal
      //sem_wait(&thread_sem);
      pthread_create(&threads[0], NULL, bfs_recur, (void *) east_params);

      //sem_wait(&thread_sem);
      pthread_create(&threads[1], NULL, bfs_recur, (void *) south_params);

      //sem_wait(&thread_sem);
      pthread_create(&threads[2], NULL, bfs_recur, (void *) north_params);

	    // Wait for all threads to finish
	    for(i = 0; i < 3; i++){
		    pthread_join(threads[i], NULL);
	    }

      //sem_post(&thread_sem);
      //sem_post(&thread_sem);
      //sem_post(&thread_sem);

      if(east_params->type == PATH || east_params->type == GOAL ||
         south_params->type == PATH || south_params->type == GOAL ||
         north_params->type == PATH || north_params->type == GOAL) {
        set_cell_type(current_y,current_x,PATH);
        node->type = PATH;
        return; // Found a path/goal
      }
      break;
    case WEST:
      // Start threaded search for goal
      //sem_wait(&thread_sem);
      pthread_create(&threads[0], NULL, bfs_recur, (void *) south_params);

      //sem_wait(&thread_sem);
      pthread_create(&threads[1], NULL, bfs_recur, (void *) west_params);

      //sem_wait(&thread_sem);
      pthread_create(&threads[2], NULL, bfs_recur, (void *) north_params);

	    // Wait for all threads to finish
	    for(i = 0; i < 3; i++){
		    pthread_join(threads[i], NULL);
	    }

      //sem_post(&thread_sem);
      //sem_post(&thread_sem);
      //sem_post(&thread_sem);

      if(south_params->type == PATH || south_params->type == GOAL ||
         west_params->type == PATH || west_params->type == GOAL ||
         north_params->type == PATH || north_params->type == GOAL) {
        set_cell_type(current_y,current_x,PATH);
        node->type = PATH;
        return; // Found a path/goal
      }
      break;
    default:
      perror("Invalid State");
      exit(0);
  }

  // No path found set to bad path and return as such
  set_cell_type(current_y,current_x,WRONG);
  node->type = WRONG;
  return;
}

/**
 * @brief     Solves the maze using a breadth-first search algorithm.
 */
int bfs_maze_solver(){
  
  int startY = maze.startY;
  int startX = maze.startX;

  cursor2_t* east_params = malloc(sizeof(cursor2_t));
  east_params->y = startY;
  east_params->x = startX+1;
  east_params->facing = EAST;

  cursor2_t* south_params = malloc(sizeof(cursor2_t));
  south_params->y = startY+1;
  south_params->x = startX;
  south_params->facing = SOUTH;

  cursor2_t* west_params = malloc(sizeof(cursor2_t));
  west_params->y = startY;
  west_params->x = startX-1;
  west_params->facing = WEST;

  cursor2_t* north_params = malloc(sizeof(cursor2_t));
  north_params->y = startY-1;
  north_params->x = startX;
  north_params->facing = NORTH;

  // Initialize an array of threads
	pthread_t threads[4];

  // Start threaded search for goal
  //sem_wait(&thread_sem);
  pthread_create(&threads[0], NULL, bfs_recur, (void *) east_params);

  //sem_wait(&thread_sem);
  pthread_create(&threads[1], NULL, bfs_recur, (void *) south_params);

  //sem_wait(&thread_sem);
  pthread_create(&threads[2], NULL, bfs_recur, (void *) west_params);

  //sem_wait(&thread_sem);
  pthread_create(&threads[3], NULL, bfs_recur, (void *) north_params);

	// Wait for all threads to finish
  int i;
	for(i = 0; i < 4; i++){
		pthread_join(threads[i], NULL);
	}

  //sem_post(&thread_sem);
  //sem_post(&thread_sem);
  //sem_post(&thread_sem);
  //sem_post(&thread_sem);

  if(east_params->type == PATH || east_params->type == GOAL ||
     south_params->type == PATH || south_params->type == GOAL ||
     west_params->type == PATH || west_params->type == GOAL ||
     north_params->type == PATH || north_params->type == GOAL)
    return 1; // Found a solution

  return 0;
}

/**
 * @brief     Sets the maze component at the given indices with the correct
 * maze component.
 */
void set_maze_component(maze_cell_t* cell, char component){

  // Initialize BFS components of cell
  cell->parent[0] = -1;
  cell->parent[1] = -1;
  cell->state = UNDISCOVERED;

  // Set cell component type
  switch(component){
    case WALL:
      cell->type = WALL;
      return;
    case BLANK:
      cell->type = BLANK;
      return;
    case START:
      cell->type = START;
      return;
    case GOAL:
      cell->type = GOAL;
      return;
    default:
      perror("Invalid maze component");
      exit(0);
  }
  
}

/**
 * @brief     A maze solver program
 * This program takes in a basic text file representation of a maze with the 
 * following accepted characters:
 * 
 * # - wall
 *   - Open space (space character)
 * S - Entry point into the maze (Case Insensitive)
 * G - End point out of the maze (Case Insensitive) 
 *
 * Ideally the maze perimeter will be specified with walls, but the solver will
 * still determine a solution without. All mazes will be rectangular in shape,
 * the program dynamically determines the size of the maze and will exit early
 * if the maze width is not consistent or an invalid character is included.
 * 
 * A sample maze is given below along with the solution generated
 * by this program when using the default right hand maze solver.
 * 
 * ##########
 * #S   #   #
 * ### ## # #
 * #      #G#
 * ##########
 *
 * Solution:
 * ##########
 * #S-- #---#
 * ###-##-#-#
 * #**----#G#
 * ##########
 *
 */
int main(int argc, char** argv){

  if(argc < 2){
    perror("No maze data file specified");
    return -1;
  }

	char* maze_file_name = argv[1];
  int isBFS = 0;
  char* maze_solver_method;
  pthread_mutex_init(&type_lock, NULL);
  sem_init(&type_sem,0,1);
  sem_init(&thread_sem,0,MAX_THREADS);

  if(argc == 3){
    maze_solver_method = argv[2];

    if(strncmp(maze_solver_method,"-t",1) != 0 && strncmp(maze_solver_method,"-T",1) != 0){
      perror("Invalid solver option. Valid options: [-t,-T] or none for right-hand rule");
      exit(0);
    }

    isBFS = 1;
  }

  /// Open the maze data file
	FILE *maze_file = NULL;
  maze_file = fopen(maze_file_name,"r");
  if(maze_file == NULL){
    perror("Error: maze data file failed to open");
    return -1;
  }


  /// Read in maze data
  /// Determine maze size and validate data, determine start and goal locations
  char cell;
  int i = 0;
  int j = 0;
  if(DEBUG) printf("Input Maze:\n");
  while(fscanf(maze_file,"%c",&cell) != EOF){
    switch(cell){
      case START:
        maze.startX = j;
        maze.startY = i;
      case GOAL:
        if(cell != START){
          maze.goalX = i;
          maze.goalY = j;
        }
      case WALL: case BLANK:
        if(DEBUG) printf("%c",cell);
        j++;
        break;
      case '\n': 
        if(i == 0) maze.width = j;
        if(maze.width != j){
          perror("Invalid maze dimensions");
          return -1;
        }
        i++;
        j = 0;
        if(DEBUG) printf("%c",cell);
        break;
      default:
        perror("Invalid character in maze");
        return -1;
    }
  }
  maze.height = i;
  if(DEBUG) printf("Start: (%d,%d)\nGoal: (%d,%d)\n",maze.startX,maze.startY,maze.goalX,maze.goalY);

  /// Size the maze matrix and read in the maze data
  rewind(maze_file);

  maze.cells = (maze_cell_t**) calloc(maze.height, sizeof(maze_cell_t*));
  for(i = 0; i < maze.height; i++){
    maze.cells[i] = (maze_cell_t*) calloc(maze.width, sizeof(maze_cell_t));

    for(j = 0; j < maze.width; j++){
      fscanf(maze_file, "%c", &cell);
      set_maze_component(&maze.cells[i][j], cell);
    }
    fscanf(maze_file, "%c", &cell);
  }

  /// Solve maze using selected rule
  if(isBFS){
    printf("Solving with BFS\n");
    if(!bfs_maze_solver())
      printf("No solution.\n");
  }else{
    printf("Solving with Right-Hand\n");
    if(!right_hand_maze_solver())
      printf("No solution.\n");
  }

  if(maze_file != NULL)
    fclose(maze_file);

  /// Output maze solution to file
  // Open file to store maze solution
  FILE *solution_file = NULL;
  char* addon = "_solution";
  char* solution_file_name = (char*) calloc(sizeof(char), (strlen(maze_file_name) + strlen(addon) + 1));

  strncat(solution_file_name, maze_file_name, strlen(maze_file_name));
  strncat(solution_file_name, addon, strlen(addon));

  solution_file = fopen(solution_file_name,"w+");

  // Print maze
  if(DEBUG) printf("\nSolution:\n");
  for(i = 0; i < maze.height; i++){
    for(j = 0; j < maze.width; j++){
      if(DEBUG) printf("%c",maze.cells[i][j].type);
      fprintf(solution_file,"%c",maze.cells[i][j].type);
    }
    fprintf(solution_file,"\n");
    if(DEBUG) printf("\n");
  }

  // Cleanup
  sem_destroy(&type_sem);
  sem_destroy(&thread_sem);
  pthread_mutex_destroy(&type_lock);
  free(solution_file_name);

  if(solution_file != NULL)
    fclose(solution_file);
}

















