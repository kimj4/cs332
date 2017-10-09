/******************************************************************************
* Main and helper functions for Island Crossing Project in Operating Systems
*
* LAST REVISED: 10/6/17 Sherri Goings
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "boat.h"

// shuffles order of array of integers
void shuffle(int* intArray, int arrayLen) {
  int i=0;
  for (i=0; i<arrayLen; i++) {
    int r = rand()%arrayLen;
    int temp = intArray[i];
    intArray[i] = intArray[r];
    intArray[r] = temp;
  }
}

/*
 * program requires 2 command line args: 
 * integer number of adults 0-9, integer number of children 2-9
 * optional 3rd arg is 1 for verbose
 */
int main(int argc, char *argv[]) {
	if (argc<3) {
	    printf("usage: executable_name <# adults> <# children> [<verbose>]\n");
	    printf("(set to 1 for more detailed output)]\n");
	    exit(1);
	}
	if (argv[1][1]!='\0' || argv[2][1]!='\0') {
	  printf("# adults/children must be digit 0-9\n");
	  exit(1);
	}
	// convert from char to int
	int totNumAdults = argv[1][0]-'0';
	int totNumKids = argv[2][0]-'0';
	verbose = 0;
	if (argc>=4) {
	  verbose = argv[3][0]-'0';
	}

	// seed the random number generator with the current time
	srand(time(NULL));

	// init mutex & cond vars
	init();

	const int total = totNumAdults + totNumKids;
	int order[total];
  
	// add desired number of each type of thread to order list
	int i;
	for (i=0; i<totNumAdults; i++) {
		order[i] = 1;
	}
	for (; i<total; i++) {
		order[i] = 2;
	}
	shuffle(order, total);

	// initialize shared variables
	kidsOahu = 0;
	adultsOahu = 0;
	start = 0;
	
	// now create threads in order indicated by shuffled order array
	pthread_t peeps[total];
	for (i=0; i<total; i++) {
		if (order[i]==1) pthread_create(&peeps[i], NULL, adultThread, NULL);
		else if (order[i]==2) pthread_create(&peeps[i], NULL, childThread, NULL);
		else printf("something went horribly wrong!!!\n");
	}

	//Wait for all threads to be created
	pthread_mutex_lock(&lock);
	while (kidsOahu != totNumKids || adultsOahu != totNumAdults){
	  pthread_cond_wait(&allReady, &lock);
	}

	//Allow the threads to start in earnest
	printf("\nAll %d adults and %d children on Oahu; crossing may now begin\n", adultsOahu, kidsOahu);
	start = 1;
	pthread_cond_broadcast(&mayStart);
	
	//Wait until everyone has crossed to Molokai (threads may not actually exit, but must update counts)
	while (kidsOahu > 0 || adultsOahu > 0) {
	  pthread_cond_wait(&allDone, &lock);
	}
	printf("All people on Malokai; main thread terminating!\n\n");
	pthread_mutex_unlock(&lock);
}

/* print statement for whenever someone gets on the boat 
 * 1st arg is type of person boarding - CHILD (real value 4) or ADULT (real value 3)
 * 2nd argument is island boat is currently on - OAHU (real value 1) or MOLO (real value 2)
*/
void boardBoat(int person, int island) {
  char* pers;
  char* isl;
  if (person == ADULT) pers = "adult";
  else pers = "child";
  if (island == OAHU) isl = "Oahu";
  else isl = "Molokai";
  printf("%s boards boat on %s\n", pers, isl);
}

/* print statement for whenever boat travels from one island to other
 * 1st arg is island boat is leaving from - OAHU (real value 1) or MOLO (real value 2)
 * 2nd argument is island boat is traveling to - OAHU (real value 1) or MOLO (real value 2)
*/
void boatCross(int from, int to) {
  char* islFrom;
  char* islTo;
  if (from == OAHU) islFrom = "Oahu";
  else islFrom = "Molokai";
  if (to == OAHU) islTo = "Oahu";
  else islTo = "Molokai";
  printf("boat is rowed across from %s to %s\n", islFrom, islTo);
}

/* print statement for whenever someone exits the boat 
 * 1st arg is type of person hopping off - CHILD (real value 4) or ADULT (real value 3)
 * 2nd argument is island boat is currently on - OAHU (real value 1) or MOLO (real value 2)
*/
void leaveBoat(int person, int island) {
  char* pers;
  char* isl;
  if (person == ADULT) pers = "adult";
  else pers = "child";
  if (island == OAHU) isl = "Oahu";
  else isl = "Molokai";
  printf("%s exits boat on %s\n", pers, isl);
}
