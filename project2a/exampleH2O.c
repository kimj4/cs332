/* Example of using semaphores - MAC compatible methods only 
 * sem_open, sem_wait, sem_post
 * 
 * written for use with H2SO4Test.c 
 * - compile with "gcc exampleSems.c H2SO4Test.c"
 * - run with "./a.out nH 0 nO" where nH is the number of hydrogen atoms to create, 
 * and nO the number of oxygen atoms.  For example "./a.out 4 0 2" will create 2 molecules of H2O
 * 
 * Author: Sherri Goings
 * Last modified: 9/28/2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "H2SO4.h"

// used to handle error that occurs when reuse semaphore name that was not previously closed correctly
int checkSem(sem_t*, char*);

// used to "spin" for some amount of time
void delay(int);

// declare hydrogen semaphore as global variable so shared and accessible from both threads
sem_t* hydro_sem;


void openSems() {
  // create the hydrogen semaphore, very important to use last 3 arguments as shown here
  // first argument is simply filename for semaphore, any name is fine but must be a valid path
  hydro_sem = sem_open("hydrosmphr", O_CREAT|O_EXCL, 0466, 0);
  while (checkSem(hydro_sem, "hydrosmphr") == -1) {
    hydro_sem = sem_open("hydrosmphr", O_CREAT|O_EXCL, 0466, 0);
  }
  
}

void closeSems() {
  // important to BOTH close the semaphore object AND unlink the semaphore file 
  sem_close(hydro_sem);
  sem_unlink("hydrosmphr");
}

/*
 * Produces an O atom, checks if 2 H atoms have already been 
 * produced, if not waits for them to be produced, then creates H2O molecule and exits
 * uses no arguments, always returns 0
 */
void* oxygen(void* args) {

  // produce an oxygen atom, takes a random amount of work with an upper bound
  delay(rand()%5000);
  printf("oxygen produced\n");
  fflush(stdout);

  // oxygen waits (calls down) twice on the hydrogen semaphore
  // meaning it cannot continue until at least 2 hydrogen atoms
  // have been produced
  int err = sem_wait(hydro_sem);
  int err2 = sem_wait(hydro_sem);
  if (err==-1 || err2==-1) printf("error on oxygen wait for hydro_sem, error # %d\n", errno);
  
  // if here, know 2 hydrogen atoms have been made already so produce a water molecule
  printf("*** H20 molecule produced ***\n");
  fflush(stdout);

  printf("oxygen exited\n");
  fflush(stdout);
  return (void*) 0;
}

/*
 * Produces an H atom after a random delay, notifies sem that another H is here, exits
 * no arguments, always returns 0
 */
void* hydrogen(void* args) {
  
  // produce a hydrogen atom, takes a random amount of work with an upper bound
  delay(rand()%5000);
  printf("hydrogen produced\n");
  fflush(stdout);

  // post (call up) on hydrogen semaphore to signal that a hydrogen atom
  // has been produced, then immediately exit
  sem_post(hydro_sem);
  
  printf("hydrogen exited\n");
  fflush(stdout);
  return (void*) 0;
}

/* included only so can compile/run with H2SO4 test code */
void* sulfur(void* args) {
  return (void*) 0;
}


/*
 * NOP function to simply use up CPU time
 * arg limit is number of times to run each loop, so runs limit^2 total loops
 */
void delay( int limit )
{
  int j, k;

  for( j=0; j < limit; j++ )
    {
      for( k=0; k < limit; k++ )
        {
        }
    }
}

/* 
 * opening semaphores using C on a unix system creates an actual semaphore file that is not
 * automatically deleted when the program exits.  As long as you close the semaphore AND
 * unlink the filename you gave in sem_open, you won't have any problems, but if you forget, or if
 * your program crashes in the middle or you have to quit using ctrl-c or something similar, you
 * will get an error when you try to run your program again because the semaphore file will already
 * exist.
 * The following code handles the above issue by deleting the sempahore file if it already existed
 * and then creating a new one.  It also handles issues where you are not allowed to create/open a
 * new file, e.g. you do not have permission at the given location.
 */
int checkSem(sem_t* sema, char* filename) {
  if (sema==SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore %s already exists, unlinking and reopening\n", filename);
      fflush(stdout);
      sem_unlink(filename);
      return -1;
    }
    else {
      printf("semaphore %s could not be opened, error # %d\n", filename, errno);
      fflush(stdout);
      exit(1);
    }
  }
  return 0;
}
