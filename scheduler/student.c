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
    StaticPriority,
    MLF
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
	} else if (argc > 2 && strcmp(argv[2], "-m") == 0) {
        alg = MLF;
        time_slice = atoi(argv[3]);
        printf("running with multi-level feedback\n");
    }
	else {
        fprintf(stderr, "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p | -m <time slice>]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler (must also give time slice)\n"
            "         -p : Static Priority Scheduler\n"
            "         -m : multi-level feedback scheduler\n\n ");
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

void print_ready_queue_size() {
    pthread_mutex_lock(&ready_mutex);
    int count = 0;
    pcb_t* cur_pcb = head;
    // printf("\n");
    while(cur_pcb != NULL) {
        // printf("%s\n", cur_pcb->name);
        if (cur_pcb->state == PROCESS_READY) {
            count ++;

        }
        cur_pcb = cur_pcb->next;
    }
    pthread_mutex_unlock(&ready_mutex);
    printf("ready queue size: %i\n", count);
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
    // print_ready_queue_size();
    // printf("is head null? : %p\n", head);
    // printf("is tail null? : %p\n", tail);
    // printf("are head and tail both null?: %i \n", head == NULL && tail == NULL);
    pcb_t* proc = getReadyProcess();
    // printf("%s\n", proc->name);
    // printf("schedule: here\n");

    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = proc;
    pthread_mutex_unlock(&current_mutex);

    if (proc!=NULL) {
        // printf("schedule: process returned is not null\n");
        proc->state = PROCESS_RUNNING;
    } else {
        // printf("schedule: prcoess returned is null\n");
    }

    // implementing clock interrupts when running round robin
    if (alg == FIFO) {
        context_switch(cpu_id, proc, -1);
    } else if (alg == RoundRobin) {
        context_switch(cpu_id, proc, time_slice);
    } else if (alg == StaticPriority) {
        context_switch(cpu_id, proc, time_slice);
    } else if (alg == MLF) {
        context_switch(cpu_id, proc, time_slice);
    }
    // printf("schedule: end\n");
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
    // printf("preempt: before getting current_mutex\n");
    pthread_mutex_lock(&current_mutex);
    // printf("preempt: after getting current_mutex\n");

    current[cpu_id]->state = PROCESS_READY;
    // processing coming back from clock interrupt gets prio lowered
    if (current[cpu_id]->temp_priority > 0) {
        current[cpu_id]->temp_priority--;
        // printf("prio gets lowered\n" );
    }

    // put process on the ready queue
    addReadyProcess(current[cpu_id]);
    pthread_mutex_unlock(&current_mutex);
    // printf("preempt: released current_mutex\n");
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

    if (alg == StaticPriority) {
        // check cpus
        // pthread_mutex_lock(&current_mutex);
        int i;
        int lowest_priority = 11;
        int lowest_priority_cpu_id = -1;
        int done = 0;

        // printf("wake_up: before getting current_mutex\n");
        pthread_mutex_lock(&current_mutex);
        // printf("wake_up: after getting current_mutex\n");
        for (i = 0; i < cpu_count; i++) {
            // Look fo a cpu that's either idle or running a process of lower prio
            if (current[i] == NULL) {
                // if there's a idle cpu, then the proccess can just go into that.
                //  so just break out of the loop and put it in the ready queue
                done = 1;
                break;
            } else if (current[i]->static_priority < lowest_priority) {
                // otherwise, find the CPU that's running the lowest prio process
                lowest_priority = current[i]->static_priority;
                lowest_priority_cpu_id = i;
            }
        }
        pthread_mutex_unlock(&current_mutex);
        // printf("wake_up: released current_mutex\n");

        if (!done){
            // perform the following if you didn't find an empty cpu.
            if (lowest_priority < process->static_priority) {
                // if this process is of higher prio than whatever is running
                if (lowest_priority_cpu_id != -1) {
                    // printf("wake_up: before getting current_mutex second time\n");
                    pthread_mutex_lock(&current_mutex);
                    // printf("wake_up: after getting current_mutex second time\n");

                    // put this process in the ready state
                    process->state = PROCESS_READY;
                    addReadyProcess(process);

                    // take the currently running process back into the ready queue
                    current[lowest_priority_cpu_id]->state = PROCESS_READY;
                    addReadyProcess(current[lowest_priority_cpu_id]);

                    pthread_mutex_unlock(&current_mutex);
                    // printf("wake_up: released current_mutex second time\n");

                    force_preempt(lowest_priority_cpu_id);
                } else {
                    printf("ERROR: found a lower prio but not an index!");
                }
            } else {
                // if there are no running processes with lower prio, then just wait
                //   in the ready queue
                process->state = PROCESS_READY;
                addReadyProcess(process);
            }
        } else {
            // if there are no idling cpus and there are no less-important running
            //  processes, just wait in the ready queue
            process->state = PROCESS_READY;
            addReadyProcess(process);
        }
    } else if (alg == MLF) {
        // printf("wake_up: top of MLF\n");
        if (process->state == PROCESS_NEW) {
            // new processes are of highest prio
            process->state = PROCESS_READY;
            process->temp_priority = 4;
            addReadyProcess(process);
            // printf("this process was new\n");
        } else if (process->state == PROCESS_WAITING) {
            // printf("this process was waiting\n");
            // this is coming from IO, so it gets higher probability
            if (process->temp_priority < 4) {
                process->temp_priority++;
            }
            process->state = PROCESS_READY;
            addReadyProcess(process);
        } else if (process->state  == PROCESS_TERMINATED) {
            printf("################  terminated process\n");
        } else if (process->state == PROCESS_RUNNING) {
            printf("################  running  process\n");
        } else {
            printf("##################################################################\n");
            if (process->temp_priority < 4) {
                process->temp_priority++;
            }
            process->state = PROCESS_READY;
            addReadyProcess(process);
        }
        // printf("wake_up: bottom of MLF\n");

    } else {
        process->state = PROCESS_READY;
        addReadyProcess(process);
    }

}

/*****************************************************************************/
/* The following 2 functions implement a FIFO ready queue of processes */
/*****************************************************************************/

/*
 * addReadyProcess adds a process to the end of a pseudo linked list (each process
 * struct contains a pointer next that you can use to chain them together)
 * it takes a pointer to a process as an argument and has no return
 */
static void addReadyProcess(pcb_t* proc) {
    // printf("addReadyProcess: before waiting on ready_mutex\n");
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
    // print_ready_queue_size();
    // printf("in the while\n");


    // printf("getReadyProcess: before waiting on ready_mutex\n");
    // ensure no other process can access ready list while we update it
    pthread_mutex_lock(&ready_mutex);
    // printf("getReadyProcess: after getting ready_mutex\n");


    if (alg == FIFO || alg == RoundRobin) {
        // if list is empty, unlock and return null
        if (head == NULL) {
      	  pthread_mutex_unlock(&ready_mutex);
        //   printf("getReadyProcess: released ready_mutex\n");
      	  return NULL;
        }

        // get first process to return and update head to point to next process
        pcb_t* first = head;
        head = first->next;

        // if there was no next process, list is now empty, set tail to NULL
        if (head == NULL) tail = NULL;
        pthread_mutex_unlock(&ready_mutex);
        // printf("getReadyProcess: released ready_mutex second time\n");

        return first;
    } else if (alg == StaticPriority) {
        if (head == NULL) {
            // printf("getReadyProcess: head is null at the top of StaticPriority\n");
            tail = NULL;
            pthread_mutex_unlock(&ready_mutex);
            // printf("getReadyProcess: released ready_mutex third time\n");

            return NULL;
        } else {
            // search for the highest priority, store its index
            int max_prio_idx = 0;
            int max_prio_val = 0;
            int cur_idx = 0;
            pcb_t* cur_pcb = head;
            // printf("\n");

            while(cur_pcb != NULL) {
                // printf("searchWhile\n");
                if ((cur_pcb->static_priority) > max_prio_val) {
                    max_prio_val = cur_pcb->static_priority;
                    max_prio_idx = cur_idx;
                }
                cur_pcb = cur_pcb->next;
                cur_idx ++;
            }
            // printf("\n");
            // printf("max_prio_val up top is %i\n", max_prio_val);
            // printf("max_prio_idx up top is %i\n", max_prio_idx);

            cur_idx = 0;
            cur_pcb = head;
            pcb_t* prev = head;

            while(1) {
                if (cur_pcb->static_priority != max_prio_val) {
                    cur_pcb = cur_pcb->next;
                    if (prev->next == cur_pcb) {
                        continue;
                    } else {
                        prev = prev->next;
                    }
                } else {
                    break;
                }
            }

            // if after searching, the found node is the head, then re-set the head
            if (cur_pcb == head) {
                // printf("highest prio is the head\n");
                head = cur_pcb->next;
                if (head == NULL) {
                    tail = NULL;
                }
            } else if (cur_pcb == tail) {
                // printf("highest prio is at the end of the list\n");
                prev->next = NULL;
                tail = prev;
            } else {
                // printf("highest prio is in the middle of the list\n"    );
                // printf("prev->next == cur_pcb?: %i\n", prev->next == cur_pcb);
                prev->next = cur_pcb->next;
            }

            pthread_mutex_unlock(&ready_mutex);
            // printf("getReadyProcess: released ready_mutex fourth time\n");

            return cur_pcb;
        }
    } else if (alg = MLF) {
        // print_ready_queue_size();
        // iterate through the ready queue, looking for the first highest prio
        int i;
        pcb_t* prev_pcb = head;
        pcb_t* cur_pcb = head;
        if (head == tail && head != NULL) {
            cur_pcb = head;
            head = NULL;
            tail = NULL;
        } else {
            int done = 0;
            for  (i = 4; i >= 1; i --) {
                if (!done) {
                    prev_pcb = head;
                    cur_pcb = head;
                    while (cur_pcb != NULL) {
                        // printf("in the while\n");
                        if (cur_pcb->temp_priority == i)  {
                            // if the highest-prio node is found, pop it.
                            if  (cur_pcb == head) {
                                // printf("1\n");
                                head = cur_pcb->next;
                                if (head == NULL) tail = NULL;
                            } else if (cur_pcb == tail) {
                                // printf("2\n");
                                tail = prev_pcb;
                                tail->next = NULL;
                            } else {
                                // printf("3\n");
                                prev_pcb->next = cur_pcb->next;
                            }
                            done = 1;
                            break;
                        } else {
                            // if not, iterate.
                            cur_pcb = cur_pcb->next;
                            if (prev_pcb->next == cur_pcb) {
                                // printf("6\n");
                                continue;
                                // break;
                            } else {
                                // printf("7\n");
                                prev_pcb = prev_pcb->next;
                            }
                        }
                    }
                } else {
                    break;
                }
            }
        }

        pthread_mutex_unlock(&ready_mutex);
        // if (cur_pcb == NULL) printf("returning nulll\n");
        return cur_pcb;
    }
    printf("ERROR: getReadyProcess didn't have anything to return!\n");
}
