#define _GNU_SOURCE
#include <sched.h>    // for processor affinity
#include <unistd.h>   // unix standard apis
#include <pthread.h>  // pthread api

#include <stdlib.h>
#include <stdio.h>

long numproc; // number of processors to use
pthread_t* tid;  // array of thread ids

// Function run by each thread
void* worker(void* arg) {
  long myid   = (long)arg;
  long mytid  = tid[myid];  // notice access to shared variable
  long mytime = 1 + rand() % 10;

  printf("Hello from thread %ld (tid %ld)\n",myid,mytid);
  printf("Thread %ld is putting itself onto procesor %ld\n", myid, myid % numproc);

  // put ourselves on the desired processor
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(myid % numproc, &set); //* round-robin; e.g. myid (thread's id) = 0, numproc = 4, run on processor 0 % 4 = 0
                                                            // * e.g. myid = 6, numproc = 4, run on processor 6 % 4 = 2
                                                            // * e.g. 4 % 4 = 0
  if (sched_setaffinity(0,sizeof(set),&set) < 0) { // do it
    perror("Can't setaffinity");  // hopefully doesn't fail
    exit(-1);
  }

  printf("Thread %ld now sleeping for %ld seconds\n", myid, mytime);

  sleep(mytime);

  printf("Thread %ld done sleeping and now exiting\n", myid);

  pthread_exit(NULL); // finish - no return value
}

int main(int argc, char* argv[]) {
  if (argc != 3 || atoi(argv[1]) < 1) {
    fprintf(stderr,"usage: pthread-ex number-of-threads number-of-procs\n");
    exit(-1);
  }

  long num = atoi(argv[1]); // number of threads
  numproc = atoi(argv[2]);  // numer of processors to use

  //* pthread_t array
  tid = (pthread_t*)malloc(sizeof(pthread_t) * num); //* tid points to array (size = num), type pthread_t

  srand(time(NULL)); // seed random number generator

  printf("Starting %ld softare threads on %ld processors (hardware threads)\n", num, numproc);

  long num_started = 0;
  for (long i = 0; i < num; i++) {
    int returncode = pthread_create(&(tid[i]), // thread id gets put here
                                    NULL, // use default attributes
                                    worker, // thread will begin in this function
                                    (void*)i // we'll give it i as the argument
                                    );
    if (returncode == 0) {
      printf("Started thread %ld, tid %lu\n", i, tid[i]);
      num_started++;
    } else {
      printf("Failed to start thread %ld\n", i);
      perror("Failed to start thread");
      tid[i] = 0xdeadbeef;
    }
  }

  printf("Finished starting threads (%ld started)\n", num_started);

  printf("Now joining\n");

  //* I assume this point is when all the child threads finished running and returned

  for (long i = 0; i < num; i++) {
    if (tid[i] != 0xdeadbeef) {
      printf("Joining with %ld, tid %lu\n", i, tid[i]);
      int returncode = pthread_join(tid[i], NULL);   //
      if (returncode != 0) {
        printf("Failed to join with %ld!\n", i);
        perror("join failed");
      } else {
        printf("Done joining with %ld\n", i);
      }
    } else {
      printf("Skipping %ld (wasn't started successfully)\n", i);
    }
  }

  printf("Done!\n");

  return 0;
}

