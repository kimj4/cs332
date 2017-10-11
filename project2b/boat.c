/******************************************************************************
*
* LAST REVISED: 10/09/17 Ju Yun Kim
* Carleton College
* CS 332 Operating Systems
* Project 2B
*
* The solution to the boat problem using condition variables
* general logic goes like this;
*  - Send two children from Oahu to Molokai, make one get off
*  - The remaining one comes back to Oahu for all the rest of the kids
*  - Repeat until there are no more adults on Oahu:
*    - When all of the kids have been moved, bring one back to Oahu
*    - An adult rows to Molokai, and gets a kid to bring the boat back to Oahu
*    - The kid brings the one child still on Oahu back to Molokai
*    - A kid brings the boat back to Oahu
*  - The last pair of kids go together to Molokai and both get off
*
*
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "boat.h"


// stores either OAHU or MOLO
int boatLocation;

// stores ADULT, KID, KID + KID (3, 4, or 8)
int boatState;

pthread_cond_t oahu;
pthread_cond_t molo;
pthread_cond_t boat;
pthread_cond_t onBoat;

void init() {
    // kidsmolokai = 0;
    // // adultsmolokai = 0;
    boatLocation = OAHU;
    boatState = 0;


    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&allReady, NULL);
    pthread_cond_init(&mayStart, NULL);
    pthread_cond_init(&allDone, NULL);


    pthread_cond_init(&oahu, NULL);
    pthread_cond_init(&molo, NULL);
    pthread_cond_init(&boat, NULL);
    pthread_cond_init(&onBoat, NULL);

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

    int myLocation = OAHU;

    // repeat forever until broken (by the finishing thread)
    while (1) {
        if (myLocation == OAHU) {

            // wait until the boat is at Oahu
            while (boatLocation != OAHU) {
                pthread_cond_wait(&boat, &lock);
            }

            // wait until there is room for me
            while (!((boatState == 0) || (boatState == KID))) {
                pthread_cond_wait(&boat, &lock);
            }

            // when I am the only child, wake up the adults
            if (myLocation == OAHU && kidsOahu == 1) {
                pthread_cond_broadcast(&oahu);
            }

            // if I am the last kid on oahu, but there are adults, wait for the
            //   adults to cross
            while (myLocation == OAHU && kidsOahu == 1 && adultsOahu > 0) {
                pthread_cond_wait(&boat, &lock);
            }

            boardBoat(KID, OAHU);
            boatState = boatState + KID;

            int last = 0;

            // The last two kids on the island, after the adults have gone,
            //   are marked as such so that they can both get off at Molokai
            if (kidsOahu == 2 && adultsOahu == 0) {
                last = 1;
            }

            // if boat is empty, you get on, and you're the captain
            if (boatState == KID) {
                // 2 children are a given, and the algorithm never allows a
                //   single child to cross Ohau --> Molokai so wait for the
                //   second child.
                while(boatState == KID) {
                    pthread_cond_wait(&onBoat, &lock);
                }


                boatCross(OAHU, MOLO);

                kidsOahu--;
                // cover the other kid as well
                kidsOahu--;

                boatLocation = MOLO;

                // the captain always gets off at Molokai
                leaveBoat(KID, MOLO);
                myLocation = MOLO;
                boatState = boatState - KID;

                // signal the other kid in the boat to get up and go back
                pthread_cond_signal(&onBoat);

                // go back to the top of the infinite while loop, and you should
                //  now be covered in the myLocation == MOLO case
                continue;

            // I am a passenger on the boat. Once I get on, signal the captain
            //   to row over
            } else {
                pthread_cond_broadcast(&onBoat);

                // wait until arrived
                while(boatState != KID) {
                    pthread_cond_wait(&onBoat, &lock);
                }

                // if I was one of the last two kids on the island after all the
                //   adults have left, land on Molokai, and signal end
                if (last) {
                    leaveBoat(KID, MOLO);
                    boatState = boatState - KID;
                    break;
                // if there are still some kids left on Oahu, go back for them
                //   and wake them up.
                } else {
                    // row back
                    boatCross(MOLO, OAHU);
                    boatLocation = OAHU;
                    kidsOahu++;
                    leaveBoat(KID, OAHU);
                    boatState = boatState - KID;
                    pthread_cond_broadcast(&boat);
                    continue;
                }

            }
        // if I'm on Molokai, then I wait until I need to bring the boat back
        //    to Molokai after an adult comes
        } else {
            // if kid in on molo, wait on it until an adult wakes it up
            while (boatState != 0) {
                pthread_cond_wait(&molo, &lock);
            }
            boardBoat(KID, MOLO);
            boatState = boatState + KID;
            boatCross(MOLO, OAHU);
            kidsOahu++;
            myLocation = OAHU;
            boatLocation = OAHU;
            leaveBoat(KID, OAHU);
            boatState = boatState - KID;
            pthread_cond_broadcast(&boat);
            continue;
        }
    }


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


    // wait until all but one kid is done crossing
    while (!(kidsOahu == 1 && boatLocation == OAHU)) {
        pthread_cond_wait(&oahu, &lock);
    }

    // check if the boat is available
    while (boatState != 0) {
        pthread_cond_wait(&boat, &lock);
    }

    // if all of these conditions are met, get on the boat and go
    boardBoat(ADULT, OAHU);
    boatState = ADULT;
    adultsOahu--;
    boatCross(OAHU, MOLO);
    boatLocation = MOLO;
    leaveBoat(ADULT, MOLO);
    boatState = 0;

    // wake up a kid on Molokai to take the boat back
    pthread_cond_signal(&molo);

    // signals to wake main to check if everyone now across, you may choose to only do
    // this in one of the adult or child threads, as long as eventually both Oahu counts
    // go to 0 and you signal allDone somewhere!
    pthread_cond_signal(&allDone);
    pthread_mutex_unlock(&lock);

    return NULL;
}
