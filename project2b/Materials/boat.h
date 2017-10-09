/******************************************************************************
*
* LAST REVISED: 10/6/17 Sherri Goings
******************************************************************************/

// used as arguments to action functions, makes easier to read and avoids errors
// due to mixing up argument values
#define OAHU 1
#define MOLO 2
#define ADULT 3
#define KID 4

// shared lock and condition variables used by threads and main
pthread_mutex_t lock;
pthread_cond_t mayStart;
pthread_cond_t allReady;
pthread_cond_t allDone;

// shared data needed by threads and main
int kidsOahu;
int adultsOahu;
int start;

// set by optional command line argument, can be used for optional output
int verbose;

// initializes locks, condition variables
void init();

// 2 types of threads
void* childThread(void*);
void* adultThread(void*);

// functions to uniformly print actions taken by threads
void boardBoat(int, int); 
void boatCross(int, int);
void leaveBoat(int, int);


