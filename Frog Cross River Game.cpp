#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <termios.h>
#include <fcntl.h>

#define ROW 10
#define COLUMN 50 
//Used to randomly generate reasonable log length
#define MAX_LENGTH 20
#define MIN_LENGTH 12

int STATUS;
pthread_mutex_t mutex;

struct Node{
	int x , y; 
	Node( int _x , int _y ) : x( _x ) , y( _y ) {}; 
	Node(){} ; 
} frog ; 

char map[ROW+10][COLUMN] ; 
long SleepTime[ROW-1];

// Determine a keyboard is hit or not. If yes, return 1. If not, return 0. 
int kbhit(void){
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);

	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}

//One function always running to render (print) according to the data structure 
void RENDER(char[ROW+1][COLUMN]){
    printf("\e[?25l");         //hide cursor
	printf("\033[H\033[2J");   //Clear console screen in C
	
		for (int i=0; i<=ROW; i++){
			
			for (int j=0; j<COLUMN; j++)
				printf("%c",map[i][j]);    //print Character here

            //Newline after each line is printed
			printf("\n");
		}

	return;
}

//One function always running to monitor user input 
int Capture(void){
	
    if(kbhit()){
		
		char Input=getchar();

		switch (Input)
		{
		case 'q':
			return 1;
			break;
		case 'Q':
			return 1;
			break;
		case 'w':
			return 2;
			break;
		case 'W':
			return 2;
			break;
		case 'a':
			return 3;
			break;
		case 'A':
			return 3;
			break;
		case 's':
			return 4;
			break;
		case 'S':
			return 4;
			break;
		case 'd':
			return 5;
			break;
		case 'D':
			return 5;
			break;
		default:
			break;
		}
	}
	
	return 10;    //Some keyboard inputs that do not affect game operation
}
void PrintResult(int STATUS){
	switch(STATUS){
		case 1:
		    printf("\033[H\033[2J");         // According to demo, we should clear the screen first
			printf("\e[?25h");               // Re-enable the cursor, or it will lose the cursor when input ./a.out again(Bad user experience)
			printf("You exit the game.\n");
			break;
		case 2: 
		    printf("\033[H\033[2J");
			printf("\e[?25h");
			printf("You win the game!!\n");
			break;
		case 3:
		    printf("\033[H\033[2J");
			printf("\e[?25h");
			printf("You lose the game!!\n");
			break;
		default:
			break;
			} 
	return;
}

//One function always running to change the underlying data structure
void *logs_move( void *t ){

    STATUS = 0;               //The normal status，the basic conditions for entering the loop
	int LINE = 1+(long)t;  //In the map, there is a bank in the first row, we need to add 1 to fit the serial number of the log
	//It will error "cast from pointer to smaller type 'int' loses information" if I choose the type of t as int

	pthread_mutex_lock(&mutex);	// I use mutex to lock the map[][]

	//Generate the LENGTH randomly from 12 to 20. I think it will be medium difficulty.
    int LENGTH = (rand() % (MAX_LENGTH-MIN_LENGTH))+ MIN_LENGTH;
	for (int i=0; i<LENGTH; i++)
		map[LINE][i] = '=';
	
	for (int i=LENGTH; i<COLUMN; i++)
		map[LINE][i] = ' ';


    int Step;
    if(LINE % 2 == 0)
		Step =1;

	if(LINE % 2 == 1)
		Step = COLUMN-1;

    // The moving direction follow the requirement of the homework guide(the first line move left-->LINE % 2 == 1-->COLUMN-1)

	SleepTime[LINE-1] = rand() % (450000 - 150000 + 1) + 150000;  //usleep 150000~450000 I think it will be medium difficulty after testing.

	pthread_mutex_unlock(&mutex);
	
	while(!STATUS){       //any status<>0 will cause stop
		
		pthread_mutex_lock(&mutex);

		/*  Move the logs  */
		if (Step == 1){
        int temp=map[LINE][COLUMN-1];
		for(int i=COLUMN-1;i>0;i--)
			map[LINE][i]=map[LINE][i-1];
		map[LINE][0]=temp;
		}
		if (Step == COLUMN-1){
        int temp=map[LINE][0];
		for(int i=1;i<COLUMN;i++)
			map[LINE][(i-1)]=map[LINE][i];
		map[LINE][COLUMN-1]=temp;
		}
	 	
	    if(frog.x==LINE)
			frog.y = (frog.y+Step)%COLUMN;
		//frog will also move if it is on the frog (It can only be on log,or it die)
		//The position of the code about the movement of the frog on the log is very important
		//It will be some bug if we move the frog on the log behind the switch (Result) 
		/**************************************************************************/
         
	    /*  Check keyboard hits, to change frog's position or quit the game. */
		int Result = Capture();  //return a correspongding nunber to indicate the status
        
        /*  Check game's status  */
		switch (Result)          //I use many RENDER() to refresh the screen，making data changes expressed in a timely manner and reducing game delay
		{
		case 1:
			STATUS = 1;	         //User quit the game
			break;

		case 2:                  //User move the frog up
		    if (map[frog.x-1][frog.y] == '|'){
				map[frog.x][frog.y] = '=';
				frog.x--;
				map[frog.x][frog.y] = '0';
				STATUS = 2;      // WIN!!!!!!
				RENDER(map);     
			}
			else if(map[frog.x-1][frog.y] == '='){
				map[frog.x][frog.y] = (frog.x==ROW)?'|':'=';   //include the situation when jump from bank
				frog.x--;
				map[frog.x][frog.y] = '0';
				RENDER(map);
			}
			else{
				STATUS = 3;      //Cause of death: drowning in water
			}  
			break;

		case 3:                  //User move the frog left
		    if (map[frog.x][frog.y-1] == '|'){
				map[frog.x][frog.y] = '|';
				frog.y--;
				map[frog.x][frog.y] = '0';
				RENDER(map);
			}
			else if (frog.x == ROW){
				STATUS = 0;      //Frog is still on the ground and safe, nothing happened
				RENDER(map);
			}
			else if(map[frog.x][frog.y-1] == '='){
				map[frog.x][frog.y] = '=';
				frog.y--;
				map[frog.x][frog.y] = '0';
				RENDER(map);
			}
			else{
				STATUS = 3;      //Cause of death: drowning in water
			}
			break;

		case 4:                  //User move the frog down
		    if (frog.x==ROW){
				STATUS = 0;      //Frog is still on the ground and safe, nothing happened
				RENDER(map);
			}
			else if(map[frog.x+1][frog.y] == '|' || map[frog.x+1][frog.y] == '='){
				map[frog.x][frog.y] = '=';     //When frog reach the opposite bank, the game is stopped, do not need to cover move down this time
				frog.x++;
				map[frog.x][frog.y] = '0';
				RENDER(map);
			}
			else{
				STATUS = 3;      //Cause of death: drowning in water
			}
			break;

		case 5:                  //User move the frog right(similar as case 2)
		    if (map[frog.x][frog.y+1] == '|'){
				map[frog.x][frog.y] = '|';
				frog.y++;
				map[frog.x][frog.y] = '0';
				RENDER(map);
			}
			else if (frog.x == ROW){
				STATUS = 0;      //Frog is still on the ground and safe, nothing happened
				RENDER(map);
			}
			else if (map[frog.x][frog.y+1] == '='){
				map[frog.x][frog.y] = '=';
				frog.y++;
				map[frog.x][frog.y] = '0';
				RENDER(map);
			}
			else{
				STATUS = 3;    //Cause of death: drowning in water
			}
			break;

		default:
			break;
		}

//Clarification on specification: 
//The frog will not die just because the log hits boundary
//But it will die when the frog hits the boundary for any reasons 
		if (frog.x!=ROW)        //frog die in the river boundary but not in bank boundary
			if (frog.y==0 || frog.y==(COLUMN-1))
				STATUS = 3; 	   //Cause of death: Hit the boundary
		
		RENDER(map);

		pthread_mutex_unlock(&mutex);
		usleep(SleepTime[LINE-1]);  //Randomly generate a suitable usleep time
	}
	/*  Print the map on the screen  */
	//I have printed a lot while processing.
	
	pthread_exit(NULL);
}

int main( int argc, char *argv[] ){

	// Initialize the river map and frog's starting position
	memset( map , 0, sizeof( map ) ) ;
	int i , j ; 
	for( i = 1; i < ROW; ++i ){	
		for( j = 0; j < COLUMN - 1; ++j )	
			map[i][j] = ' ' ;  
	}	

	for( j = 0; j < COLUMN; ++j )	
		map[ROW][j] = map[0][j] = '|' ;

	for( j = 0; j < COLUMN; ++j )	
		map[0][j] = map[0][j] = '|' ;

	frog = Node( ROW, (COLUMN-1) / 2 ) ; 
	map[frog.x][frog.y] = '0' ; 

	//Print the map into screen
	for( i = 0; i <= ROW; ++i)	
		puts( map[i] );

	/*  Create pthreads for wood move and frog control.  */
	pthread_mutex_init(&mutex,NULL);

    int res;
    pthread_t LOGS[ROW-1];
	
	for(long i=0; i<(ROW-1); i++)  
        res = pthread_create(&LOGS[i],NULL,logs_move,(void*)i);   
    //It will cause "warning: cast to 'void *' from smaller integer type int" if I choose the type of i as int
	
	for(int j=0; j<(ROW-1); j++)
        res = pthread_join(LOGS[j],NULL);   //Join the threads together
    
    PrintResult(STATUS);

    pthread_mutex_destroy(&mutex);
	pthread_exit(NULL);

	return 0;
}
