#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEBUG 0

FILE *f;
char startChar = 'S';
char endChar = 'G';
char wallChar = '#';
char openChar = 'o';
char pathChar = ' ';
int x = 11;
int y = 11;
int currentCell[] = {1,1};
int visitedCells = 1;
int cellStackTop = 0;
int endLocation[3]; // x, y, max cellStackTop value
int cellStack[100000000][2];
int numberOfRemainingCells = 0;
int remainingCells[100000000][2];
char cells[20002][20002]; 
int count = 0;
	
void initMaze(){
	int i=0;
	int j=0;
	for(i = 0; i<=y; i++){
		for(j=0; j<=x; j++){
			if(i==0 || j==0 || i==y || j==x){
				cells[i][j]=wallChar;
			}else if(i%2==1 && j%2==1 && (j+1)!=x && (i+1)!=y){
				cells[i][j]=openChar;
			}else{
				cells[i][j]=wallChar;
			}
		}
	}
}

void printMaze(){
	//FILE *f = fopen("log.txt","a");
  int i=0;
  int j=0;
  for(i =0; i<y; i++){
    for(j=0; j<x; j++){
      fprintf(f,"%c",cells[i][j]);
  //    printf("%c",cells[i][j]);
    }
    fprintf(f,"\n");
//		printf("\n");
  }
//	printf("\n\n\n");
//	fclose(f);
}

int searchMaze(){
	int i=1;
	int j=1;
	numberOfRemainingCells = 0;
	for(i =1; i<=y; i++){
	    for(j=1; j<=x; j++){
	      if(cells[i][j] == openChar){
					remainingCells[numberOfRemainingCells][0]= i;
					remainingCells[numberOfRemainingCells][1]= j; 
			
					numberOfRemainingCells++;
					if(count != (y*x)/10){ // Only check 100 times when building maze to cut down on time
						j=x+1;
						i=y+1;
					}
				}
	    }
	  }
	if(count == (y*x)/10){
        	printf("Number of remaining cells: %d\n",numberOfRemainingCells);
		count = 0;
        }
	count++;
	return numberOfRemainingCells;
}

int moveDirection(){
	if(DEBUG){
		printf("Attempting to move \n");
	}

	int valid[5] = {0,0,0,0,0}; //Right, Left, Up, Down 
	int moving = 0;
	if(currentCell[0] < y-1){ // dont check past bounds
		if(cells[currentCell[0]+2][currentCell[1]] == openChar){
			valid[0] = 1;
			moving = 1;
		}
	}
	if(currentCell[0] > 1){
	 	if(cells[currentCell[0]-2][currentCell[1]] == openChar){
		    	valid[1] = 1;
			moving =1;
		}
	}
	if(currentCell[1] < x-1){
		if(cells[currentCell[0]][currentCell[1]+2] == openChar){
    			valid[2] = 1;
			moving = 1;
		}
	}

	if(currentCell[1] > 1){
		if(cells[currentCell[0]][currentCell[1]-2] == openChar){
    			valid[3] = 1;
			moving = 1;
		}
	}
	if(moving == 0){
		if(DEBUG){
			printf("Cannot Move!");
		}

		cellStackTop--;
		if(cellStackTop == 0){
			return 0;
		}else{
			currentCell[0] = cellStack[cellStackTop][0];
			currentCell[1] = cellStack[cellStackTop][1];
		}
		return 1;
		
	}else{
		if(DEBUG){	
			printf("Right: %d, Left: %d, Up: %d, Down %d\n", valid[0], valid[1],valid[2],valid[3]);
		}

		int r = 4;
		while(valid[r]==0){
			r = rand(); //number between 0-3	
			r = r%4;
		}

		cellStackTop++;
		cellStack[cellStackTop][0]=currentCell[0];
		cellStack[cellStackTop][1]=currentCell[1];

		switch(r){
			case 0:
				//right
				cells[currentCell[0]++][currentCell[1]]=pathChar;
				cells[currentCell[0]++][currentCell[1]]=pathChar;
				if(DEBUG){
					printf("Moving Right\n");
				}
				break;
			case 1:
				//Left
				cells[currentCell[0]--][currentCell[1]]=pathChar;
	      			cells[currentCell[0]--][currentCell[1]]=pathChar;
				if(DEBUG){
					printf("Moving Left\n");
				}
				break;
			case 2:
				//Up
				cells[currentCell[0]][currentCell[1]++]=pathChar;
				cells[currentCell[0]][currentCell[1]++]=pathChar;
				if(DEBUG){
					printf("Moving Up\n");
				}
				break;
			case 3:
				//down
				cells[currentCell[0]][currentCell[1]--]=pathChar;
	      			cells[currentCell[0]][currentCell[1]--]=pathChar;
				if(DEBUG){
					printf("Moving Down\n");
				}
				break;
			default:
				//error
				if(DEBUG){
					printf("ERROR in Rand number gen\n");
				}
				break;
		}

    if(cellStackTop > endLocation[2]){
      			if(DEBUG){
				printf("End Location Change!");
			}
			cells[endLocation[0]][endLocation[1]]=pathChar;
			cells[currentCell[0]][currentCell[1]]=endChar;
      
      endLocation[0]=currentCell[0];
			endLocation[1]=currentCell[1];
			endLocation[2]=cellStackTop;
    }
	
		return moving;
	}

}

void genMaze(){
	initMaze();
	char prevVal=pathChar;
	int r = 0;

	// start maze
	cells[1][1]=pathChar;
	cellStackTop = 1;
	currentCell[0]=cellStack[cellStackTop][0]=1;
	currentCell[1]=cellStack[cellStackTop][1]=1;

	while(searchMaze() > 0){
		prevVal=cells[currentCell[0]][currentCell[1]];	
		cells[currentCell[0]][currentCell[1]] = '1';
		//printMaze();
		if(prevVal==endChar){
			cells[currentCell[0]][currentCell[1]] = prevVal;
		}else{
			cells[currentCell[0]][currentCell[1]] = pathChar;
		}	
		if(DEBUG){
			printf("Cell Stack Top: %d\n",cellStackTop);			
		}

		if(moveDirection() == 0){
			if(DEBUG){
				printf("\n\nDONE!\n\n");
			}
			break;
		}
			// if we can't move a valid direction with the current cell
		/*	printf("Cannot find direction to move");
			r = rand(); 
			r= r % numberOfRemainingCells;
			currentCell[0] = remainingCells[r][0];
			currentCell[1] = remainingCells[r][1];
			cells[currentCell[0]][currentCell[1]] = pathChar;	*/
		
	}
		
  // Set start point and print final layout of maze
  cells[0][0]=wallChar;
  cells[1][1]=startChar;
}


int main(int argc, char** argv){
/*	printf("Number of Columns: ");
	scanf("%d",&x);
	printf("Number of Rows: ");
	scanf("%d",&y);*/
  if(argc > 1){
    if(sscanf(argv[1],"%d",&x) != 1) return;
    if(sscanf(argv[2],"%d",&y) != 1) return;
  }

	srand(time(NULL));
	f = fopen("log.txt","w");
	printf("generating maze\n");
	genMaze();
	fclose(f);
  
  char* maze_file_name = malloc(80 * sizeof(char));
  snprintf(maze_file_name,80,"%dx%d_maze",x,y);
  f = fopen(maze_file_name,"w+");
  printf("Printing Maze\n");
  printMaze();
  fclose(f);
  free(maze_file_name);
  printf("Done!\n\n");

	return 1;
}
