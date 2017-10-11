/*
 * Ju Yun Kim
 * Carleton College 2017
 * CS 332 - Operating Systems
 * Solution for project2a
 *
 * One thing to note is that after a sulfuric acid molecule is created, in
 * almost all cases, it will be all productions then all deletions then another
 * molecule creation. This happens because H and O productions block when 2
 * and 4 are generated. Maybe this is not the right way to do it but this is
 * what I have now
 *
 * This was hard
 *
 */

#include "H2SO4.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

// used to handle error that occurs when reuse semaphore name that was not previously closed correctly
int checkSem(sem_t*, char*);

// things waiting on hydrogen production
sem_t* hydro_sem;

// things waiting on oxygen production
sem_t* oxy_sem;

// things waiting on sulfur to finish making the molecule
sem_t* sul_sem;

// tells hydrogen atoms when molecule formation is done, so that they can exit
sem_t* molecule_sem;

// ensures that all atoms from current molecule exits before another molecule can be made
sem_t* H2SO4_sem;

// keeps track of when hydrogens exit so that sulfur can exit
sem_t* hydro_exit_sem;

// resolves the problem where program hangs indefinitely even when all the
//  ingredient atoms are present
sem_t* lock;

// keeps track of the number of hydrogens so that hydrogen production can block
//  when a molecule is being formed
sem_t* h_count_sem;

// same with oxygen
sem_t* o_count_sem;

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

    lock = sem_open("locksmphr", O_CREAT|O_EXCL, 0466, 1);
    while (checkSem(lock, "locksmphr") == -1) {
      lock = sem_open("locksmphr", O_CREAT|O_EXCL, 0466, 1);
    }

    h_count_sem = sem_open("hcountsmphr", O_CREAT|O_EXCL, 0466, 2);
    while (checkSem(h_count_sem, "hcountsmphr") == -1) {
      h_count_sem = sem_open("hcountsmphr", O_CREAT|O_EXCL, 0466, 2);
    }

    o_count_sem = sem_open("ocountsmphr", O_CREAT|O_EXCL, 0466, 4);
    while (checkSem(o_count_sem, "ocountsmphr") == -1) {
      o_count_sem = sem_open("ocountsmphr", O_CREAT|O_EXCL, 0466, 4);
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

    sem_close(hydro_exit_sem);
    sem_unlink("hydroexitsmphr");

    sem_close(lock);
    sem_unlink("locksmphr");

    sem_close(h_count_sem);
    sem_unlink("hcountsmphr");

    sem_close(o_count_sem);
    sem_unlink("ocountsmphr");
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



void* oxygen(void* somevar) {
    int err0 = sem_wait(o_count_sem);
    if (err0 == -1) printf("error on oxygen wait for o_count_sem, error # %d\n", errno);


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
    int err0 = sem_wait(h_count_sem);
    if (err0 == -1) printf("error on hydrogen wait for h_count_sem, error # %d\n", errno);


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


    // sulfur needs to wait on 2 hydrogens
    int err00 = sem_wait(lock);
    if (err00==-1) printf("error on oxygen wait for lock, error # %d\n", errno);

    int err = sem_wait(hydro_sem);
    int err2 = sem_wait(hydro_sem);
    if (err==-1 || err2==-1) printf("error on oxygen wait for hydro_sem, error # %d\n", errno);

    // sulfur needs to wait on 4 oxygens
    int err3 = sem_wait(oxy_sem);
    int err4 = sem_wait(oxy_sem);
    int err5 = sem_wait(oxy_sem);
    int err6 = sem_wait(oxy_sem);
    if (err3 == -1 || err4 == -1 || err5 == -1 || err6 == -1) printf("error on hydrogen wait for oxy_sem, error # %d\n", errno);

    int err0 = sem_wait(H2SO4_sem);
    if (err0==1) printf("error on sulfur wait for H2SO4_sem, error # %d\n", errno);

    printf("*** H2SO4 molecule produced ***\n");
    fflush(stdout);

    sem_post(lock);

    sem_post(o_count_sem);
    sem_post(o_count_sem);
    sem_post(o_count_sem);
    sem_post(o_count_sem);
    sem_post(h_count_sem);
    sem_post(h_count_sem);



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

    sem_post(H2SO4_sem);


    return 0;
}
