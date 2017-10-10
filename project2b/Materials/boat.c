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

int kidsMolokai;
int adultsMolokai;

// stores either OAHU or MOLO
int boatLocation;

// stores ADULT, KID, KID + KID (3, 4, or 8)
int boatState;

pthread_cond_t oahu;
pthread_cond_t boat;
pthread_cond_t onBoat;

void init() {
    kidsMolokai = 0;
    adultsMolokai = 0;
    boatLocation = OAHU;
    boatState = 0;


    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&allReady, NULL);
    pthread_cond_init(&mayStart, NULL);
    pthread_cond_init(&allDone, NULL);


    pthread_cond_init(&oahu, NULL);
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


    while (1) {

    // printf("here\n");

    // check if boat is at oahu
    while (boatLocation != OAHU) {
        pthread_cond_wait(&boat, &lock);
    }



    // check if there is a slot available on the boat
    while (!((boatState == 0) || (boatState == KID))) {
        pthread_cond_wait(&boat, &lock);
    }

    // printf("here\n");

    // check that there will still be one kid left on oahu
    if (adultsOahu > 0) {
        while (kidsOahu == 1) {
            pthread_cond_wait(&boat, &lock);
        }
    }


    boardBoat(KID, OAHU);
    kidsOahu--;
    boatState = boatState + KID;

    if (boatState == KID) {
        // TODO: check whether this will always be okay.
        //         will there be cases where a single child will be rowing back alone?
        while(boatState == KID) {
            pthread_cond_wait(&onBoat, &lock);
        }


        boatCross(OAHU, MOLO);
        boatLocation = MOLO;
        leaveBoat(KID, MOLO);
        kidsMolokai++;
        boatState = boatState - KID;

        // signal the other kid in the boat to get up and go back
        pthread_cond_signal(&onBoat);


    } else {
        // wake other kid on the boat to go to other island
        pthread_cond_broadcast(&onBoat);

        // wait until arrived
        while(boatState != KID) {
            pthread_cond_wait(&onBoat, &lock);
        }

        // row back
        boatCross(MOLO, OAHU);
        boatLocation = OAHU;
        leaveBoat(KID, OAHU);
        kidsOahu++;
        boatState = boatState - KID;

        // while()

        // awake other kids waiting on the boat
        pthread_cond_broadcast(&boat);
        pthread_mutex_unlock(&lock);

    }

    }








        // /*
        // * DUMMY CODE - Remove in final solution!
        // * adult rows self to Molokai, boat magically returns (or there are infinite boats available)
        // * updates Oahu count to show has crossed
        // * KID, ADULT, OAHU, and MOLO are defined in the .h file and should be the only 4
        // * possible values for the arguments to the action functions.
        // */
        // boardBoat(KID, OAHU);
        // boatCross(OAHU, MOLO);
        // leaveBoat(KID, MOLO);
        // kidsOahu--;
        // /*** end of dummy code ***/

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

    // adults wait until there is only one kid on oahu
    while (kidsOahu != 1) {
        pthread_cond_wait(&oahu, &lock);
    }

    // check if the boat is at Oahu
    while (boatState != OAHU) {
        pthread_cond_wait(&boat, &lock);
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
    adultsMolokai++;
    boatLocation = MOLO;
    boatState = 0;




        // /*
        // * DUMMY CODE - Remove in final solution!
        // * adult rows self to Molokai, boat magically returns (or there are infinite boats available)
        // * updates Oahu count to show has crossed
        // * KID, ADULT, OAHU, and MOLO are defined in the .h file and should be the only 4
        // * possible values for the arguments to the action functions.
        // */
        // boardBoat(ADULT, OAHU);
        // boatCross(OAHU, MOLO);
        // leaveBoat(ADULT, MOLO);
        // adultsOahu--;
        // /*** end of dummy code ***/

    // signals to wake main to check if everyone now across, you may choose to only do
    // this in one of the adult or child threads, as long as eventually both Oahu counts
    // go to 0 and you signal allDone somewhere!
    pthread_cond_signal(&allDone);
    pthread_mutex_unlock(&lock);

    return NULL;
}
