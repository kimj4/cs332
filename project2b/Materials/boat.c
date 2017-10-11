/******************************************************************************
*
* LAST REVISED: 10/09/17 Ju Yun Kim
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "boat.h"

// int // kidsmolokai;
// int // // adultsmolokai;

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

    while (1) {

    // printf("here\n");
    if (myLocation == OAHU) {

        // check if boat is at oahu
        while (boatLocation != OAHU) {
            pthread_cond_wait(&boat, &lock);
        }


        // check if there is a slot available on the boat
        while (!((boatState == 0) || (boatState == KID))) {
            pthread_cond_wait(&boat, &lock);
        }

        // printf("### kidsOahu: %d\n", kidsOahu);

        if (myLocation == OAHU && kidsOahu == 1) {
            // printf("in the big conditional\n");
            pthread_cond_broadcast(&oahu);
            // pthread_cond_broadcast(&boat);
        }

        // printf("here\n");
        while (myLocation == OAHU && kidsOahu == 1 && adultsOahu > 0) {
            // printf("in the big wait\n" );
            // if last child on oahu but there are adults, then wait
            pthread_cond_wait(&boat, &lock);
        }
        boardBoat(KID, OAHU);
        boatState = boatState + KID;

        int last = 0;


        // if after boarding, there is no one on the island, cross and leave.
        if (kidsOahu == 2 && adultsOahu == 0) {
            // printf("I am now the last\n");
            last = 1;
        }

        if (boatState == KID) {
            // TODO: check whether this will always be okay.
            //         will there be cases where a single child will be rowing back alone?
            while(boatState == KID) {
                pthread_cond_wait(&onBoat, &lock);
            }


            boatCross(OAHU, MOLO);

            // decrement for the other kid as well
            kidsOahu--;
            kidsOahu--;

            boatLocation = MOLO;
            leaveBoat(KID, MOLO);
            // kidsmolokai++;
            myLocation = MOLO;
            boatState = boatState - KID;

            // signal the other kid in the boat to get up and go back
            pthread_cond_signal(&onBoat);
            continue;


        } else {
            // wake other kid on the boat to go to other island
            pthread_cond_broadcast(&onBoat);

            // wait until arrived
            while(boatState != KID) {
                pthread_cond_wait(&onBoat, &lock);
            }

            if (last) {
                leaveBoat(KID, MOLO);
                // kidsmolokai++;
                boatState = boatState - KID;
                break;
            } else {
                // row back
                boatCross(MOLO, OAHU);

                kidsOahu++;

                boatLocation = OAHU;
                leaveBoat(KID, OAHU);
                // kidsOahu++;
                boatState = boatState - KID;
                pthread_cond_broadcast(&boat);
                continue;
            }

        }
    } else {
        // should only happen after an adult lands on shore
        // if kid in on molo, wait on it until an adult wakes it up
        while (boatState != 0) {
            pthread_cond_wait(&molo, &lock);
        }
        boardBoat(KID, MOLO);
        boatState = boatState + KID;
        boatCross(MOLO, OAHU);
        // kidsmolokai--;
        kidsOahu++;
        myLocation = OAHU;
        boatLocation = OAHU;
        leaveBoat(KID, OAHU);
        boatState = boatState - KID;
        pthread_cond_broadcast(&boat);

        // printf("kidsOahu: %d\n", kidsOahu);
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


    while (!(kidsOahu == 1 && boatLocation == OAHU)) {
        // printf("in the first adult conditional\n" );
        pthread_cond_wait(&oahu, &lock);
    }

    // check if the boat is available
    while (boatState != 0) {
        pthread_cond_wait(&boat, &lock);
    }

    // if all of these conditions are met, get on the boat and go
    boardBoat(ADULT, OAHU);
    boatCross(OAHU, MOLO);
    leaveBoat(ADULT, MOLO);
    adultsOahu--;
    // // adultsmolokai++;
    boatLocation = MOLO;
    boatState = 0;

    pthread_cond_signal(&molo);

    // signals to wake main to check if everyone now across, you may choose to only do
    // this in one of the adult or child threads, as long as eventually both Oahu counts
    // go to 0 and you signal allDone somewhere!
    pthread_cond_signal(&allDone);
    pthread_mutex_unlock(&lock);

    return NULL;
}
