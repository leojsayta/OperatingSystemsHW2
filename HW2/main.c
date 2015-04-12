//
//  main.c
//  HW2
//
//  Created by Joel Atyas on 4/10/15.
//  Copyright (c) 2015 Joel Atyas. All rights reserved.
//

#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>
#include<stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/resource.h>

#define NUM_OF_READERS 5
#define NUM_READS 200
#define NUM_WRITES 200


/******************************* -- IMPORTANT!!!!!!!! -- ********************************************
 Unamed semaphores are not implemented in OSX.  Thus sem_init, sem_destroy, do not work in OSX.
 In addition, sem_getvalue is also not implemented in OSX.
 In order to use semaphores in OSX, they must be named semaphores.
 i.e. sem_open, sem_close, and sem_unlink.
 ******************************* -- IMPORTANT!!!!!!!! -- ********************************************/


#define SEMTYPE             // comment out to use all pthread_mutex_t and no sem_t
                            // leave uncommented to use combination of pthread_mutex_t and sem_t

int readerCount;                        // number of threads reading or wanting to read
pthread_mutex_t readerCountMutex;      // the mutual exclusion semaphore for the readerCount

int isWriterWaiting;
pthread_cond_t readerWait;

#ifdef SEMTYPE
sem_t* bufferSem;                     // mutual exclusion semaphore for the "shared buffer"
#else
pthread_mutex_t bufferMutex;          // mutual exclusion semaphore for the "shared buffer"
#endif

int sharedBuffer;           // the "shared buffer" is an int starting at 0.
                            // readers read its value and print out.
                            // writer increments the value by 1.

void* writerCode(void* a)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 25000;
//    int semVal = 0;
    
    int count;
    for (count = 0; count < NUM_WRITES; count++)
    {
#ifdef SEMTYPE
        isWriterWaiting = 1;
        sem_wait(bufferSem);
//        printf("The writer (pre sharedBuffer incr) bufferSem value is %d.\n", sem_getvalue(bufferSem, &semVal));
#else
        if (pthread_mutex_trylock(&bufferMutex))
        {
            isWriterWaiting = 1;
            pthread_mutex_lock(&bufferMutex);
        }
#endif
        sharedBuffer = sharedBuffer + 1;
        
        printf("Writer is on iteration %d. The new buffer value is %d. The readerCount is %d\n\n", count + 1, sharedBuffer, readerCount);
        
        isWriterWaiting = 0;
        pthread_cond_broadcast(&readerWait);
        
#ifdef SEMTYPE
        sem_post(bufferSem);
//        printf("The writer (post sharedBuffer incr) bufferSem value is %d.\n", sem_getvalue(bufferSem, &semVal));
#else
        pthread_mutex_unlock(&bufferMutex);
#endif
        nanosleep(&ts, NULL);
    }
    
    pthread_exit(0);
}

void* readerCode(void* a)
{
    int argumentValue = *((int *) a) + 1; // dereference to obtain argument value
//    printf("Greetings from Reader: %d.\n", argumentValue);
    
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 25000;
//    int semVal = 0;

    int count;
    for (count = 0; count < NUM_READS; count++)
    {
        pthread_mutex_lock(&readerCountMutex);
        
        while (isWriterWaiting)
            pthread_cond_wait(&readerWait, &readerCountMutex);
        
        if (readerCount == 0)
        {
#ifdef SEMTYPE
            sem_wait(bufferSem);
//            printf("The reader bufferSem value (pre-read) is %d.\n", sem_getvalue(bufferSem, &semVal));
#else
            pthread_mutex_lock(&bufferMutex);            
#endif
        }
        
        readerCount = readerCount + 1;
//        printf("The ++readerCount is %d.\n", readerCount);
        
        pthread_mutex_unlock(&readerCountMutex);
        
        printf("Reader %d is on iteration %d. The current buffer value is %d. The readerCount is %d.\n\n",
               argumentValue, count + 1, sharedBuffer, readerCount);
        
        pthread_mutex_lock(&readerCountMutex);
        
        readerCount = readerCount - 1;
//        printf("The --readerCount is %d.\n\n", readerCount);
        
        if (readerCount == 0)
        {
#ifdef SEMTYPE
            sem_post(bufferSem);
//            printf("The reader bufferSem value (post-read) is %d.\n", sem_getvalue(bufferSem, &semVal));
#else
            pthread_mutex_unlock(&bufferMutex);            
#endif
        }

        pthread_mutex_unlock(&readerCountMutex);
//        nanosleep(&ts, NULL);
    }
    
    pthread_exit(0);
 }

int main()
{
    sharedBuffer = 0;
    readerCount = 0;
    
    pthread_mutex_init(&readerCountMutex, 0);
    pthread_cond_init(&readerWait, 0);

#ifdef SEMTYPE
    if ((bufferSem = sem_open("/semBuffer", O_CREAT, 0644, 1)) == SEM_FAILED )
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
//    bufferSem = sem_open("semBuffer", O_CREAT, S_IRUSR|S_IWUSR, 0);
//    if(bufferSem == SEM_FAILED) {
//        perror("child sem_open");
//        return -1;
//    }
#else
    pthread_mutex_init(&bufferMutex, 0);    
#endif
    
    pthread_t writerThread;
    pthread_create(&writerThread, 0, writerCode, 0);
    
    pthread_t readerThread[NUM_OF_READERS];
    int count;
    for (count = 0; count < NUM_OF_READERS; count++)
    {
        // we have the use dumanic memory allocation to declare an argument var
        int *reader = (int *) malloc(sizeof(int *));
        *reader = count; // we wish to pass int value 5 as the argument to the thread
        pthread_create(&readerThread[count], 0, readerCode, (void *) reader);
    }
    
    pthread_join(writerThread, 0);
    for (count = 0; count < NUM_OF_READERS; count++)
        pthread_join(readerThread[count], 0);
    
    pthread_mutex_destroy(&readerCountMutex);
    pthread_cond_destroy(&readerWait);
    
#ifdef SEMTYPE
    if (sem_close(bufferSem) == -1) {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }
    
    if (sem_unlink("/semBuffer") == -1) {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }
#else
    pthread_mutex_destroy(&bufferMutex);    
#endif
    
    
    
    
}