/*
 * student.c
 * This file contains the CPU scheduler for the simulation.
 * original base code from http://www.cc.gatech.edu/~rama/CS2200
 * Last modified 10/20/2017 by Sherri Goings
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"
#include "student.h"

// Local helper function
static void schedule(unsigned int cpu_id);

/*
 * here's another way to do the thing I've used #define for in a couple of the past projects
 * which is to associate a word with each "state" of something, instead of having to
 * remember what integer value goes with what actual state, e.g. using MOLO and OAHU
 * instead of 1 and 2 to designate an island in the boat project.
 *
 * enum is useful C language construct to associate desriptive words with integer values
 * in this case the variable "alg" is created to be of the given enum type, which allows
 * statements like "if alg == FIFO { ...}", which is much better than "if alg == 1" where
 * you have to remember what algorithm is meant by "1"...
 * just including it here to introduce you to the idea if you haven't seen it before!
 */
typedef enum {
    FIFO = 0,
    RoundRobin,
    StaticPriority
} scheduler_alg;

scheduler_alg alg;

// declare other global vars
int time_slice = -1;
int cpu_count;


/*
 * main() parses command line arguments, initializes globals, and starts simulation
 */
int main(int argc, char *argv[])
{
    /* Parse command line args - must include num_cpus as first, rest optional
     * Default is to simulate using just FIFO on given num cpus, if 2nd arg given:
     * if -r, use round robin to schedule (must be 3rd arg of time_slice)
     * if -p, use static priority to schedule
     */
    if (argc == 2) {
		alg = FIFO;
		printf("running with basic FIFO\n");
	}
	else if (argc > 2 && strcmp(argv[2],"-r")==0 && argc > 3) {
		alg = RoundRobin;
		time_slice = atoi(argv[3]);
		printf("running with round robin, time slice = %d\n", time_slice);
	}
	else if (argc > 2 && strcmp(argv[2],"-p")==0) {
		alg = StaticPriority;
		printf("running with static priority\n");
	}
	else {
        fprintf(stderr, "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler (must also give time slice)\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
	fflush(stdout);

    /* atoi converts string to integer */
    cpu_count = atoi(argv[1]);

    /* Allocate the current[] array of cpus and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    int i;
    for (i=0; i<cpu_count; i++) {
        current[i] = NULL;
    }
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);

    /* Initialize other necessary synch constructs */
    pthread_mutex_init(&ready_mutex, NULL);
    pthread_cond_init(&ready_empty, NULL);

    /* Start the simulator in the library */
    printf("starting simulator\n");
    fflush(stdout);
    start_simulator(cpu_count);


    return 0;
}

/*
 * idle() is called by the simulator when the idle process is scheduled.
 * It blocks until a process is added to the ready queue, and then calls
 * schedule() to select the next process to run on the CPU.
 *
 * THIS FUNCTION IS ALREADY COMPLETED - DO NOT MODIFY
 */
extern void idle(unsigned int cpu_id)
{
  pthread_mutex_lock(&ready_mutex);
  while (head == NULL) {
    pthread_cond_wait(&ready_empty, &ready_mutex);
  }
  pthread_mutex_unlock(&ready_mutex);
  schedule(cpu_id);
}

/*
 * schedule() is your CPU scheduler. It currently implements basic FIFO scheduling -
 * 1. calls getReadyProcess to select and remove a runnable process from your ready queue
 * 2. updates the current array to show this process (or NULL if there was none) as
 *    running on the given cpu
 * 3. sets this process state to running (unless its the NULL process)
 * 4. calls context_switch to actually start the chosen process on the given cpu
 *    - note if proc==NULL the idle process will be run
 *    - note the final arg of -1 means there is no clock interrupt
 *	context_switch() is prototyped in os-sim.h. Look there for more information.
 *  a basic getReadyProcess() is implemented below, look at the comments for info.
 *
 * TO-DO: handle scheduling with a time-slice when necessary
 *
 * THIS FUNCTION IS PARTIALLY COMPLETED - REQUIRES MODIFICATION
 */
static void schedule(unsigned int cpu_id) {
    pcb_t* proc = getReadyProcess();
    printf("schedule: here\n");

    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = proc;
    pthread_mutex_unlock(&current_mutex);

    if (proc!=NULL) {
        printf("schedule: process returned is not null\n");
        proc->state = PROCESS_RUNNING;
    } else {
        printf("schedule: prcoess returned is null\n");

    }

    // implementing clock interrupts when running round robin
    if (alg == FIFO) {
        context_switch(cpu_id, proc, -1);
    } else if (alg == RoundRobin) {
        context_switch(cpu_id, proc, time_slice);
    } else if (alg == StaticPriority) {
        context_switch(cpu_id, proc, time_slice);
    }
    printf("schedule: end\n");

}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 * cpu_id is the index of this cpu in the "current" array of cpu's.
 *
 * This function should get the process currently running on the given cpu,
 * change the process state to ready, place the process back in the
 * ready queue (for FIFO just use addReadyProcess), and finally call
 * schedule() for this cpu to select a new runnable process.
 *
 * THIS FUNCTION MUST BE IMPLEMENTED FOR ROUND ROBIN OR PRIORITY SCHEDULING
 */
extern void preempt(unsigned int cpu_id) {
    // get process on cpu, set it to ready state
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_READY;

    // put process on the ready queue
    addReadyProcess(current[cpu_id]);
    pthread_mutex_unlock(&current_mutex);
    // call schedule
    schedule(cpu_id);

}


/*
 * yield() is called by the simulator when a process performs an I/O request
 * note this is different than the concept of yield in user-level threads!
 * In this context, yield sets the state of the process to waiting (on I/O),
 * then calls schedule() to select a new process to run on this CPU.
 * args: int - id of CPU process wishing to yield is currently running on.
 *
 * THIS FUNCTION IS ALREADY COMPLETED - DO NOT MODIFY
 */
extern void yield(unsigned int cpu_id) {
    // use lock to ensure thread-safe access to current process
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is called by the simulator when a process completes.
 * marks the process as terminated, then calls schedule() to select
 * a new process to run on this CPU.
 * args: int - id of CPU process wishing to terminate is currently running on.
 *
 * THIS FUNCTION IS ALREADY COMPLETED - DO NOT MODIFY
 */
extern void terminate(unsigned int cpu_id) {
    // use lock to ensure thread-safe access to current process
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}

/*
 * wake_up() is called for a new process and when an I/O request completes.
 * The current implementation handles basic FIFO scheduling by simply
 * marking the process as READY, and calling addReadyProcess to put it in the
 * ready queue.  No locks are needed to set the process state as its not possible
 * for anyone else to also access it at the same time as wake_up
 *
 * TO-DO: If the scheduling algorithm is static priority, wake_up() may need
 * to preempt the CPU with the lowest priority process to allow it to
 * execute the process which just woke up.  However, if any CPU is
 * currently running idle, or all of the CPUs are running processes
 * with a higher priority than the one which just woke up, wake_up()
 * should not preempt any CPUs. To preempt a process, use force_preempt().
 * Look in os-sim.h for its prototype and parameters.
 *
 * THIS FUNCTION IS PARTIALLY COMPLETED - REQUIRES MODIFICATION
 */
extern void wake_up(pcb_t *process) {
    printf("wake_up: top\n");
    // check cpus
    // pthread_mutex_lock(&current_mutex);
    int i;
    int lowest_priority = 11;
    int lowest_priority_cpu_id = -1;
    int empty_cpu_idx = -1;
    int done = 0;


    for (i = 0; i < cpu_count; i++) {
        printf("wake_up: before getting current_mutex\n");
        pthread_mutex_lock(&current_mutex);
        printf("wake_up: after getting current_mutex\n");
        if (current[i] == NULL) {
            printf("found a free cpu\n");
            // if an idling cpu is found, do nothing
            // process->state = PROCESS_RUNNING;
            // context_switch(i, process, time_slice);
            done = 1;
            break;
        } else if (current[i]->static_priority < lowest_priority) {
            printf("found a lower priority running process!\n");

            // otherwise keep track of a process to replace
            lowest_priority = current[i]->static_priority;
            lowest_priority_cpu_id = i;
        }
        pthread_mutex_unlock(&current_mutex);
    }

    if (!done){
        if (lowest_priority < process->static_priority) {
            if (lowest_priority_cpu_id != -1) {

                process->state = PROCESS_READY;
                addReadyProcess(process);
                // if there is a process with lower prio, force preempt the lowest
                //    of them all
                printf("about to call force_preempt\n");
                printf("%i\n", lowest_priority_cpu_id);

                // current[lowest_priority_cpu_id]->state = PROCESS_READY;
                // addReadyProcess(current[lowest_priority_cpu_id]);

                force_preempt(lowest_priority_cpu_id);
                printf("after force_preempt\n");
                // process->state = PROCESS_RUNNING;

                // current[lowest_priority_cpu_id] = process;

                // get process on cpu, set it to ready state

                // put process on the ready queue

                // call schedule
                // schedule(lowest_priority_cpu_id);


            }
        } else {
            // if there are no running processes with lower prio, then just wait
            //   in the ready queue
            process->state = PROCESS_READY;
            addReadyProcess(process);
        }
    } else {
        // if there are no running processes with lower prio, then just wait
        //   in the ready queue
        process->state = PROCESS_READY;
        addReadyProcess(process);
    }

    // pthread_mutex_unlock(&current_mutex);
    // printf("at the end of wake_up\n");



}


/* The following 2 functions implement a FIFO ready queue of processes */

/*
 * addReadyProcess adds a process to the end of a pseudo linked list (each process
 * struct contains a pointer next that you can use to chain them together)
 * it takes a pointer to a process as an argument and has no return
 */
static void addReadyProcess(pcb_t* proc) {
    printf("addReadyProcess: before waiting on ready_mutex\n");
  // ensure no other process can access ready list while we update it
  pthread_mutex_lock(&ready_mutex);

  // add this process to the end of the ready list
  if (head == NULL) {
    head = proc;
    tail = proc;
    // if list was empty may need to wake up idle process
    pthread_cond_signal(&ready_empty);
  }
  else {
    tail->next = proc;
    tail = proc;
  }

  // ensure that this proc points to NULL
  proc->next = NULL;

  pthread_mutex_unlock(&ready_mutex);
}


/*
 * getReadyProcess removes a process from the front of a pseudo linked list (each process
 * struct contains a pointer next that you can use to chain them together)
 * it takes no arguments and returns the first process in the ready queue, or NULL
 * if the ready queue is empty
 *
 * TO-DO: handle priority scheduling
 *
 * THIS FUNCTION IS PARTIALLY COMPLETED - REQUIRES MODIFICATION
 */
static pcb_t* getReadyProcess(void) {

    printf("getReadyProcess: before waiting on ready_mutex\n");
    // ensure no other process can access ready list while we update it
    pthread_mutex_lock(&ready_mutex);


    if (alg == FIFO || alg == RoundRobin) {
        // if list is empty, unlock and return null
        if (head == NULL) {
      	  pthread_mutex_unlock(&ready_mutex);
      	  return NULL;
        }

        // get first process to return and update head to point to next process
        pcb_t* first = head;
        head = first->next;

        // if there was no next process, list is now empty, set tail to NULL
        if (head == NULL) tail = NULL;
        pthread_mutex_unlock(&ready_mutex);
        return first;
    } else if (alg == StaticPriority) {
        // look for the ready process with highest priority
        if (head == NULL) {
            tail = NULL;
            pthread_mutex_unlock(&ready_mutex);
            return NULL;
        } else {
            int max_prio_idx = 0;
            int max_prio_val = 0;
            int cur_idx = 0;
            pcb_t* cur_pcb = head;
            while(cur_pcb != NULL) {
                printf("cur_pcb static_priority is %i\n", cur_pcb->static_priority);
                if ((cur_pcb->static_priority) > max_prio_val) {
                    printf("I'm here!\n");
                    max_prio_val = cur_pcb->static_priority;
                    max_prio_idx = cur_idx;
                }
                cur_pcb = cur_pcb->next;
                cur_idx ++;

            }
            printf("max_prio_val up top is %i\n", max_prio_val);
            printf("max_prio_idx up top is %i\n", max_prio_idx);

            cur_idx = 0;
            cur_pcb = head;
            int cur_val = -1;
            pcb_t* prev = head;
            // identify previous node
            // identify node to be popped
            // pop the desired node, set next of prev to the next of poppped node
            while(cur_idx != max_prio_idx) {
                // navigate to the node to be returned
                cur_pcb = cur_pcb->next;
                cur_idx++;
                if (prev->next == cur_pcb) {
                    // should only be true when beginning searching
                    continue;
                } else {
                    prev = prev->next;
                }

            }

            printf("found prio val is %i\n", cur_pcb->static_priority);
            printf("found cur_idx is %i\n", cur_idx);
            // if (cur_pcb == NULL) tail = NULL;
            if (cur_pcb == prev) {
                printf("prev and cur are the same\n");
                head = NULL;
                tail = NULL;
            } else {
                printf("*****************prev and cur are not the same\n");
                // here, we are popping
                prev->next = cur_pcb->next;
            }
            printf("is cur_pcb null? : %i\n", cur_pcb == NULL);

            pthread_mutex_unlock(&ready_mutex);
            return cur_pcb;
        }
    }

    // pthread_mutex_unlock(&ready_mutex);
    // return first;
}
