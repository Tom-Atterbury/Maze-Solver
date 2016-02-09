#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "maze_types.h"

#define DEBUG 0
#define SCALE 2

/* Modified from code written by Ben Bullock provided at:
           http://www.lemoda.net/c/write-png/ 

   for use in CprE 308 maze project
*/

/* A coloured pixel. */

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} pixel_t;

/* A picture. */
    
typedef struct  {
    pixel_t *pixels;
    size_t width;
    size_t height;
} bitmap_t;

// Maze object to store maze 
maze_t maze;

/*
 * Given "bitmap", this returns the pixel of bitmap at the point 
 * ("x", "y"). 
 */
static pixel_t * pixel_at (bitmap_t * bitmap, int x, int y) {
    return bitmap->pixels + bitmap->width * y + x;
}
    
/*
 * Write "bitmap" to a PNG file specified by "path"; returns 0 on
 * success, non-zero on error. 
 */
static int save_png_to_file (bitmap_t *bitmap, char *path) {
    FILE * fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte ** row_pointers = NULL;
    /* "status" contains the return value of this function. At first
       it is set to a value which means 'failure'. When the routine
       has finished its work, it is set to a value which means
       'success'. */
    int status = -1;
    /* The following number is set by trial and error only. I cannot
       see where it it is documented in the libpng manual.
    */
    int pixel_size = 3;
    int depth = 8;
    
    fp = fopen (path, "wb");
    if (! fp) {
        goto fopen_failed;
    }

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }
    
    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }
    
    /* Set up error handling. */

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }
    
    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  bitmap->width,
                  bitmap->height,
                  depth,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);
    
    /* Initialize rows of PNG. */

    row_pointers = png_malloc (png_ptr, bitmap->height * sizeof (png_byte *));
    for (y = 0; y < bitmap->height; ++y) {
        png_byte *row = 
            png_malloc (png_ptr, sizeof (uint8_t) * bitmap->width * pixel_size);
        row_pointers[y] = row;
        for (x = 0; x < bitmap->width; ++x) {
            pixel_t * pixel = pixel_at (bitmap, x, y);
            *row++ = pixel->red;
            *row++ = pixel->green;
            *row++ = pixel->blue;
        }
    }
    
    /* Write the image data to "fp". */

    png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* The routine has successfully written the file, so we set
       "status" to a value which indicates success. */

    status = 0;
    
    for (y = 0; y < bitmap->height; y++) {
        png_free (png_ptr, row_pointers[y]);
    }
    png_free (png_ptr, row_pointers);
    
 png_failure:
 png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);
 png_create_write_struct_failed:
    fclose (fp);
 fopen_failed:
    return status;
}

/* Given "value" and "max", the maximum value which we expect "value"
   to take, this returns an integer between 0 and 255 proportional to
   "value" divided by "max". */
static int pix (int value, int max) {
    if (value < 0)
        return 0;
    return (int) (256.0 *((double) (value)/(double) max));
}

/**
 * @brief     Sets the maze component at the given indices with the correct
 * maze component.
 */
void set_maze_component(maze_cell_t* cell, char component){

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
    case VISIT:
      cell->type = VISIT;
      return;
    case WRONG:
      cell->type = WRONG;
      return;
    case PATH:
      cell->type = PATH;
      return;
    default:
      perror("Invalid maze component");
      exit(0);
  }
  
}

void read_in_maze(char* maze_file_name){

  /// Open the maze data file
	FILE *maze_file = NULL;
  maze_file = fopen(maze_file_name,"r");
  if(maze_file == NULL){
    perror("Error: maze data file failed to open");
    exit(0);
  }

  /// Read in maze data
  /// Determine maze size and validate data, determine start and goal locations
  char cell;
  int i = 0;
  int j = 0;
  if(DEBUG) printf("Input Maze:\n");
  while(fscanf(maze_file,"%c",&cell) != EOF){
    switch(cell){
      case START: case GOAL: case WALL: case BLANK:
      case VISIT: case WRONG: case PATH:
        if(DEBUG) printf("%c",cell);
        j++;
        break;
      case '\n':
        if(i == 0) maze.width = j;
        if(maze.width != j){
          perror("Invalid maze dimensions");
          exit(0);
        }
        i++;
        j = 0;
        if(DEBUG) printf("%c",cell);
        break;
      default:
        perror("Invalid character in maze");
        exit(0);
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

  fclose(maze_file);
}

int main (int argc, char** argv) {

  if(argc < 2){
    perror("No maze data file specified");
    exit(0);
  }

	char* maze_file_name = argv[1];
  if(DEBUG) printf("%s\n",maze_file_name);

  // Read in maze data from file
  read_in_maze(maze_file_name);

  bitmap_t maze_image;
  int x;
  int y;

  /// Create an image.
  maze_image.width = maze.width * SCALE;
  maze_image.height = maze.height * SCALE;

  maze_image.pixels = calloc (sizeof (pixel_t), maze_image.width * maze_image.height);
  int i,j;

  for (y = 0; y < maze.height; y++) {
    for (x = 0; x < maze.width; x++) {
      for (i = 0; i < SCALE; i++) {
        for (j = 0; j < SCALE; j++) {
          pixel_t * pixel = pixel_at (& maze_image, SCALE*x+i, SCALE*y+j);
          switch(maze.cells[y][x].type){
            case WALL:
              pixel->red = pixel->green = pixel->blue = 0;
              break;
            case BLANK:
              pixel->red = pixel->green = pixel->blue = 255;
              break;
            case START:
              pixel->red = 0;
              pixel->green = 204;
              pixel->blue = 0;
              break;
            case GOAL:
              pixel->red = 204;
              pixel->green = 0;
              pixel->blue = 0;
              break;
            case VISIT:
              pixel->red = 153;
              pixel->green = 153;
              pixel->blue = 102;
              break;
            case WRONG:
              pixel->red = 102;
              pixel->green = 0;
              pixel->blue = 51;
              break;
            case PATH:
              pixel->red = 51;
              pixel->green = 102;
              pixel->blue = 255;
              break;
            default:
              perror("Invalid maze component");
              exit(0);
          }
        }
      }
    }
  }

  char* addon = ".png";
  char* image_file_name = (char*) calloc(sizeof(char), (strlen(maze_file_name) + strlen(addon) + 1));

  strncat(image_file_name, maze_file_name, strlen(maze_file_name));
  strncat(image_file_name, addon, strlen(addon));

  /// Write the image to a file
  save_png_to_file (& maze_image, image_file_name);

  free(image_file_name);

  return 0;
}
