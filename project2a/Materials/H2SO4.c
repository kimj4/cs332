#include "H2SO4.h"

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



void* oxygen(void* somevar) {
    return 0;
}

void* hydrogen(void* somevar) {
    return 0;
}

void* sulfur(void* somevar) {
    return 0;
}

void openSems() {

}

void closeSems() {

}
