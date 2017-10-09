/******************************************************************************
*
* LAST REVISED: 10/6/17 Sherri Goings
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "boat.h"

void init() {
  /* Initialize mutex and condition variable objects */
  pthread_mutex_init(&lock, NULL);
  pthread_cond_init(&allReady, NULL);
  pthread_cond_init(&mayStart, NULL);
  pthread_cond_init(&allDone, NULL);
}

void* childThread(void* args) {
  //Count the number of kids on oahu, signal main to check if all here now
  pthread_mutex_lock(&lock);
  kidsOahu++; 
  pthread_cond_signal(&allReady);

  // wait until main signals that all here
  while (!start) {
    pthread_cond_wait(&mayStart, &lock);
  }

  /*
   * DUMMY CODE - Remove in final solution!
   * adult rows self to Molokai, boat magically returns (or there are infinite boats available)
   * updates Oahu count to show has crossed
   * KID, ADULT, OAHU, and MOLO are defined in the .h file and should be the only 4 
   * possible values for the arguments to the action functions.
   */
  boardBoat(KID, OAHU);
  boatCross(OAHU, MOLO);
  leaveBoat(KID, MOLO);
  kidsOahu--;
  /*** end of dummy code ***/

  // signals to wake main to check if everyone now across, you may choose to only do 
  // this in one of the adult or child threads, as long as eventually both Oahu counts 
  // go to 0 and you signal allDone somewhere!
  pthread_cond_signal(&allDone);  
  pthread_mutex_unlock(&lock);

  return NULL;
}

void* adultThread(void* args) {
  //Count the number of adults on oahu, signal main to check if all here now
  pthread_mutex_lock(&lock);
  adultsOahu++; 
  pthread_cond_signal(&allReady);

  // wait until main signals that all here
  while (!start) {
    pthread_cond_wait(&mayStart, &lock);
  }

  /*
   * DUMMY CODE - Remove in final solution!
   * adult rows self to Molokai, boat magically returns (or there are infinite boats available)
   * updates Oahu count to show has crossed
   * KID, ADULT, OAHU, and MOLO are defined in the .h file and should be the only 4 
   * possible values for the arguments to the action functions.
   */
  boardBoat(ADULT, OAHU);
  boatCross(OAHU, MOLO);
  leaveBoat(ADULT, MOLO);
  adultsOahu--;
  /*** end of dummy code ***/
  
  // signals to wake main to check if everyone now across, you may choose to only do 
  // this in one of the adult or child threads, as long as eventually both Oahu counts 
  // go to 0 and you signal allDone somewhere!
  pthread_cond_signal(&allDone);  
  pthread_mutex_unlock(&lock);
  
  return NULL;
}
