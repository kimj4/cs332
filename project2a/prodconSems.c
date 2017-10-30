/* Producer/Consumer implemented using only semaphores 
 * (though 1 semaphore is used specifically as a mutex lock)
 * Author: Sherri Goings
 * Last Modified: 10/2/2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

// Basic queue of strings implemented as circular array
// first is the index to pop from (consume)
// size is the number of items in the queue, so first+size is the index to push to (produce)
// capacity is the total number of slots in the array, if first reaches capacity, it wraps back around to 0.
// it is the job of the semaphores to ensure that we never attempt to push to a full buffer or pop from an empty buffer.
typedef struct Buffer Buffer;
struct Buffer {
  char** buff;
  unsigned int first, size, capacity;
};

// functions for threads
void* producer(void*);
void* consumer(void*);

// helper functions
void addItem(char* item);
char* getItem();
int checkSem(sem_t*, char*);
void delay(int);

const int buffSize = 5;
Buffer itemQ;

sem_t* empty_slots;
sem_t* full_slots;
sem_t* mutex;

int main() {
  
  // allocate buffer to hold buffSize strings of max length 10 chars each
  char** buffer = (char**) malloc( buffSize * sizeof(char*) );
  int i;
  for (i=0; i<buffSize; i++) {
    buffer[i] = (char*) malloc( 10 );
  }

  // set initial buffer values to reflect empty buffer with capacity as defined in buffSize
  itemQ.buff = buffer;
  itemQ.first = 0;
  itemQ.size = 0;
  itemQ.capacity = buffSize;

  // create full/empty slot semaphores, initially no full slots and buffSize empty slots
  full_slots = sem_open("fullslots", O_CREAT|O_EXCL, 0466, 0);
  while (checkSem(full_slots, "fullslots") == -1) {
    full_slots = sem_open("fullslots", O_CREAT|O_EXCL, 0466, 0);
  }
  empty_slots = sem_open("emptyslots", O_CREAT|O_EXCL, 0466, buffSize);
  while (checkSem(empty_slots, "emptyslots") == -1) {
    empty_slots = sem_open("emptyslots", O_CREAT|O_EXCL, 0466, buffSize);
  }

  // create mutex semaphore as lock to buffer access itself - required to protect values
  // first, size, and capacity that are used to index buffer from being modified
  // by multiple threads at once
  // like all mutex locks, initially open, i.e. semaphore value of 1
  mutex = sem_open("mutex", O_CREAT|O_EXCL, 0466, 1);
  while (checkSem(mutex, "mutex") == -1) {
    mutex = sem_open("mutex", O_CREAT|O_EXCL, 0466, 1);
  }
 
  // simple example - create 2 each of producer and consumer
  pthread_t prod1, prod2, con1, con2;
  pthread_create(&prod1, NULL, producer, "P1");
  pthread_create(&con1, NULL, consumer, "C1");
  pthread_create(&prod2, NULL, producer, "P2");
  pthread_create(&con2, NULL, consumer, "C2");

  // quit when everyone done (note this means that if don't consume as much as produce, or visa versa, will hang
  pthread_join(con1, NULL);
  pthread_join(con2, NULL);
  pthread_join(prod1, NULL);
  pthread_join(prod2, NULL);

  // close and unlink all semaphores
  sem_close(full_slots);
  sem_unlink("fullslots");
  sem_close(empty_slots);
  sem_unlink("emptyslots");
  sem_close(mutex);
  sem_unlink("mutex");
  
  return 0;
}

/* producer waits until there are empty slots to put items, then waits to acquire lock
 * produces actual item and adds to buffer, releases lock and signals that there is
 * now another full slot
 */
void* producer(void* args) {
  printf("%s created\n", (char*) args);
  fflush(stdout);

  // produce up to 100 things
  int i;
  for (i=0; i<10; i++) {
    // random delay between each production
    delay(rand()%5000);
 
    sem_wait(empty_slots);
    sem_wait(mutex);

    char* product = (char*) malloc(10);
    sprintf(product, "%s-%d", (char*) args, i);
    addItem(product);
    printf("%s added %s to buffer\n", (char*) args, product);
    fflush(stdout);
    
    sem_post(mutex);
    sem_post(full_slots);
  }
  return (void*) 0;
}

/* consumer waits until there are full slots to get an item from, then waits to acquire lock
 * removes actual item from buffer, releases lock and signals that there is
 * now another empty slot
 */
void* consumer(void* args) {
  printf("%s created\n", (char*) args);
  fflush(stdout);

  // consume 15 things
  int i;
  for (i=0; i<10; i++) {

    // random delay between each consumption
    delay(rand()%5000);
 
    sem_wait(full_slots);
    sem_wait(mutex);

    char* item = (char*) malloc(10);
    item = getItem();
    printf("%s consumed by %s\n", item, (char*) args);
    fflush(stdout);
    
    sem_post(mutex);
    sem_post(empty_slots);
    
  }
  printf("%s exiting\n", (char*) args);
  fflush(stdout);
  return (void*) 0;
}

/* places item in back of circular array at next available index, increments size */
void addItem(char* item) {
  itemQ.buff[(itemQ.first+itemQ.size)%itemQ.capacity] = item;
  itemQ.size++;
}

/* gets item from front of circular array, moves first up one, decrements size */
char* getItem() {
  char* item = itemQ.buff[itemQ.first];
  itemQ.first = (itemQ.first+1)%itemQ.capacity;
  itemQ.size--;
  return item;
}

/* in case program crashes in any way and so doesn't call close on one or more semaphores,
 * this will unlink and reopen the already existing semaphores so that they do not cause an error
 */
int checkSem(sem_t* sema, char* filename) {
  if (sema==SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore %s already exists, unlinking and reopening\n", filename);
      fflush(stdout);
      int unlink_err = sem_unlink(filename);
      if (unlink_err != 0) {
	printf("%s error unlinking semaphore %s, if permission issue, try running w/sudo\n", strerror(errno), filename); 
      }
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


