#include "H2SO4.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

/*
 * Interface between test program and solution functions
 * for H2S04 molecule creation problem
 * Note - this will be used for grading, be sure your solution .c file
 * compiles and runs correctly with this .h and H2SO4Test.c!
 * Author: Sherri Goings
 * Last modified: 4/18/2016
 */


// notes on project2
// Don't change the header file
// must use the header file
// Can use any code in exampleH2O.c
// compile
//    gcc H2SO4.c H2SO4Test.c -o molecules
// run
//    3 integers: number of hydrogen, sulfur, oxygen in order
//    ./molecules 2 1 4
// no need to modify test program
// can if needed for more thorough testing

// Use semaphores for synchronization
// methods: sem_open, sem_wait, sem_post
// see exampleH2O.c file
// Remember that you can use it for both basic mutual exclusion locking and counting

// H2SO4 is sulfuric acid
// Write the synchronization needed for three different types of threads to interact
//   in a manner that resembles atom interactions to form sulfuric acid
// Each thread corresponds to one of the 3 atoms

// First thing for all atoms: output that this atom is produced
// General idea: use semaphores to synch atoms into forming molecules

// Can use any many semaphores as needed
// Should be declared globally
// Open all of them in openSems
// Should close/unlink all of them in closeSems

// To form H2SO4, 2 hydrogren threads, 1 sulfur thread, 4 oxygen threads must be present
// If any are missing, the remaining ones must wait (no busy waiting) until the missing ones arrive

// As each new thread is created, program checks if there are enough to form a H2SO4 group
// If some atoms are missing, arriving thread should be blocked on a semaphore's wait queue
// No limit to how many can be waiting

// When all required are present, print that a molecule formed
// Then allow all threads in that molecule to depart
// print when each one leaves
// Molecule printing needs to come before leaving printing

// When the group departs,
// hydrogen leaves First
// then sulfur
// then oxygen
// if there are more than the required number of threads of the same type waiting, their order doesn't matter
// don't alow creating new molecules before the current one leaves

// if the requirements are met the molecule must be formed
// The last atom can be blocked temporarily, but must guarantee that it won't be waiting indefinitely
// Program can hang indefinitely at the end if there are waiting atoms that don't meet the requirements

// used to handle error that occurs when reuse semaphore name that was not previously closed correctly
int checkSem(sem_t*, char*);


sem_t* hydro_sem;
sem_t* oxy_sem;
sem_t* sul_sem;
sem_t* molecule_sem;
sem_t* H2SO4_sem;
sem_t* hydro_exit_sem;


void* oxygen(void* somevar) {
    printf("oxygen produced\n");
    fflush(stdout);

    sem_post(oxy_sem);

    // must wait for sulfur to exit before oxygen can exit.
    int err = sem_wait(sul_sem);
    if (err==1) printf("error on oxygen wait for sul_sem, error # %d\n", errno);

    printf("oxygen exited\n");
    fflush(stdout);

    return 0;
}

void* hydrogen(void* somevar) {
    printf("hydrogen produced\n");
    fflush(stdout);

    sem_post(hydro_sem);

    // must wait on molecule to be formed before exiting
    int err = sem_wait(molecule_sem);
    if (err==1) printf("error on hydrogen wait for molecule_sem, error # %d\n", errno);

    printf("hydrogen exited\n");
    fflush(stdout);

    // allow sulfur to exit
    sem_post(hydro_exit_sem);

    return 0;
}

void* sulfur(void* somevar) {
    printf("sulfur produced\n");
    fflush(stdout);

    // ???: acquire the lock so that only one sulfur atom can be waiting on the
    //      hydrogen and oxygen atom at a time
    int err0 = sem_wait(H2SO4_sem);
    if (err0==1) printf("error on sulfur wait for H2SO4_sem, error # %d\n", errno);

    // sulfur needs to wait on 2 hydrogens
    int err = sem_wait(hydro_sem);
    int err2 = sem_wait(hydro_sem);
    if (err==-1 || err2==-1) printf("error on oxygen wait for hydro_sem, error # %d\n", errno);

    // sulfur needs to wait on 4 oxygens
    int err3 = sem_wait(oxy_sem);
    int err4 = sem_wait(oxy_sem);
    int err5 = sem_wait(oxy_sem);
    int err6 = sem_wait(oxy_sem);
    if (err3 == -1 || err4 == -1 || err5 == -1 || err6 == -1) printf("error on hydrogen wait for oxy_sem, error # %d\n", errno);

    // int err0 = sem_wait(H2SO4_sem);
    // if (err0==1) printf("error on sulfur wait for H2SO4_sem, error # %d\n", errno);

    printf("*** H2SO4 molecule produced ***\n");
    fflush(stdout);

    // ???: release the lock as soon as a molecule is formed and allow another
    //      sulfur atom to start calling down on hydrogen and oxygen
    sem_post(H2SO4_sem);

    // allow the two waiting hydrogens to exit
    sem_post(molecule_sem);
    sem_post(molecule_sem);

    // exit after the hydrogens exit
    int err7 = sem_wait(hydro_exit_sem);
    int err8 = sem_wait(hydro_exit_sem);
    if (err7==-1 || err8==-1) printf("error on oxygen wait for hydro_sem, error # %d\n", errno);

    printf("sulfur exited\n");
    fflush(stdout);

    // allow the four oxygens to exit
    sem_post(sul_sem);
    sem_post(sul_sem);
    sem_post(sul_sem);
    sem_post(sul_sem);

    // sem_post(H2SO4_sem);


    return 0;
}

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

    sul_sem = sem_open("sulsmphr", O_CREAT|O_EXCL, 0466, 0);
    while (checkSem(sul_sem, "sulsmphr") == -1) {
      sul_sem = sem_open("sulsmphr", O_CREAT|O_EXCL, 0466, 0);
    }

    molecule_sem = sem_open("moleculesmphr", O_CREAT|O_EXCL, 0466, 0);
    while (checkSem(molecule_sem, "moleculesmphr") == -1) {
        molecule_sem = sem_open("moleculesmphr", O_CREAT|O_EXCL, 0466, 0);
    }

    hydro_exit_sem = sem_open("hydroexitsmphr", O_CREAT|O_EXCL, 0466, 0);
    while (checkSem(hydro_exit_sem, "hydroexitsmphr") == -1) {
        hydro_exit_sem = sem_open("hydroexitsmphr", O_CREAT|O_EXCL, 0466, 0);
    }

    // JK: This is going to be used as a mutex, so initialize with value 1
    H2SO4_sem = sem_open("H2SO4smphr", O_CREAT|O_EXCL, 0466, 1);
    while (checkSem(H2SO4_sem, "H2SO4smphr") == -1) {
      H2SO4_sem = sem_open("H2SO4smphr", O_CREAT|O_EXCL, 0466, 1);
    }
}

void closeSems() {
    // important to BOTH close the semaphore object AND unlink the semaphore file
    sem_close(hydro_sem);
    sem_unlink("hydrosmphr");

    sem_close(oxy_sem);
    sem_unlink("oxysmphr");

    sem_close(sul_sem);
    sem_unlink("sulsmphr");

    sem_close(molecule_sem);
    sem_unlink("moleculesmphr");

    sem_close(H2SO4_sem);
    sem_unlink("H2SO4smphr");
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
