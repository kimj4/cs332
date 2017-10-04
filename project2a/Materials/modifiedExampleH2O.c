// TODO: For this to meet all the requirements, it needs to:
// - None of the constituent atoms can exit before the production statement.
// - Hydrogen must exit before oxygen
// - All three atoms of the first molecule must exit before the next one can be created.

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
sem_t* oxy_sem;
sem_t* water_sem;


void openSems() {
  // create the hydrogen semaphore, very important to use last 3 arguments as shown here
  // first argument is simply filename for semaphore, any name is fine but must be a valid path
  hydro_sem = sem_open("hydrosmphr", O_CREAT|O_EXCL, 0466, 0);
  while (checkSem(hydro_sem, "hydrosmphr") == -1) {
    hydro_sem = sem_open("hydrosmphr", O_CREAT|O_EXCL, 0466, 0);
  }

  // JK: new semaphore
  oxy_sem = sem_open("oxysmphr", O_CREAT|O_EXCL, 0466, 0);
  while (checkSem(oxy_sem, "oxysmphr") == -1) {
    oxy_sem = sem_open("oxysmphr", O_CREAT|O_EXCL, 0466, 0);
  }


  // JK: This is going to be used as a mutex, so initialize with value 1
  water_sem = sem_open("watersmphr", O_CREAT|O_EXCL, 0466, 1);
  while (checkSem(water_sem, "watersmphr") == -1) {
    water_sem = sem_open("watersmphr", O_CREAT|O_EXCL, 0466, 1);
  }


}

void closeSems() {
  // important to BOTH close the semaphore object AND unlink the semaphore file
  sem_close(hydro_sem);
  sem_unlink("hydrosmphr");

  sem_close(oxy_sem);
  sem_unlink("oxysmphr");

  sem_close(water_sem);
  sem_unlink("watersmphr");
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

  // JK: acquire the lock basically
  int err3 = sem_wait(water_sem);
  if (err3==-1) printf("error on oxygen wait for water_sem, error # %d\n", errno);

  // if here, know 2 hydrogen atoms have been made already so produce a water molecule
  printf("*** H20 molecule produced ***\n");
  fflush(stdout);

  // JK: now that the h2o production message has been printed, wake the hydrogens
  sem_post(oxy_sem);
  sem_post(oxy_sem);

  // JK: Now wait twice on hydrogen so that hydrogens can exit first and then
  //     wake the oxygen to exit.
  int err4 = sem_wait(hydro_sem);
  int err5 = sem_wait(hydro_sem);
  if (err4==-1 || err5 == -1) printf("error on oxygen wait for hydro_sem, error # %d\n", errno);

  printf("oxygen exited\n");
  fflush(stdout);

  // JK: release the lock so that new molecules can be created only after
  //     the oxygen in the current molecule exits
  sem_post(water_sem);

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

  // JK: after waking oxygen, sleep on oxygen.
  int err = sem_wait(oxy_sem);
  if (err == -1) printf("error on hydrogen wait for oxy_sem, error # %d\n", errno);

  printf("hydrogen exited\n");

  // JK: when hydrogen is done exiting, try waking up a waiting oxygen so it can
  //     exit
  sem_post(hydro_sem);

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
