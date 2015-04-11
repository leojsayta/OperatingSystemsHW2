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

pthread_mutex_t readerCountMutex;      // the mutual exclusion semaphore for the readerCount
int readerCount;             // number of threads reading or wanting to read

int isWriterWaiting;
pthread_cond_t readerWait;
pthread_mutex_t bufferMutex;          // mutual exclusion semaphore for the "shared buffer"
int sharedBuffer;           // the "shared buffer" is an int starting at 0.
                            // readers read its value and print out.
                            // writer increments the value by 1.

void* writerCode(void* a)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 10;
    
    int count;
    for (count = 0; count < NUM_WRITES; count++)
    {
        isWriterWaiting = 1;
        pthread_mutex_lock(&bufferMutex);
        
        sharedBuffer = sharedBuffer + 1;
        
        printf("Writer is on iteration %d. The new buffer value is %d. The readerCount is %d\n\n", count + 1, sharedBuffer, readerCount);
//        printf("New buffer value is %d.\n", sharedBuffer);
        
        isWriterWaiting = 0;
        pthread_cond_signal(&readerWait);
        pthread_mutex_unlock(&bufferMutex);
//        nanosleep(&ts, NULL);
    }
    
    pthread_exit(0);
    
    //return 0;
}

void* readerCode(void* a)
{
    int argumentValue = *((int *) a); // dereference to obtain argument value
    //printf("Greetings from Reader: %d.\n", argumentValue);
    
    int count;
    for (count = 0; count < NUM_READS; count++)
    {
        pthread_mutex_lock(&readerCountMutex);
        
        if (isWriterWaiting && readerCount == 5) {
            pthread_cond_wait(&readerWait, &readerCountMutex);
        }
        
        readerCount = readerCount + 1;
        if (readerCount == 1)
            pthread_mutex_lock(&bufferMutex);
        printf("The ++readerCount is %d.\n", readerCount);
        pthread_mutex_unlock(&readerCountMutex);
        
        printf("Reader %d is on iteration %d. The current buffer value is %d\n", argumentValue, count + 1, sharedBuffer);
        
        pthread_mutex_lock(&readerCountMutex);
        readerCount = readerCount - 1;
        if (readerCount == 0)
            pthread_mutex_unlock(&bufferMutex);
        printf("The --readerCount is %d.\n\n", readerCount);
        pthread_mutex_unlock(&readerCountMutex);
    }
    
    pthread_exit(0);
    
    //return 0;
}

int main()
{
    sharedBuffer = 0;
    readerCount = 0;
    
    pthread_mutex_init(&readerCountMutex, 0);
    pthread_mutex_init(&bufferMutex, 0);
    pthread_cond_init(&readerWait, 0);
    
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
    pthread_mutex_destroy(&bufferMutex);
    pthread_cond_destroy(&readerWait);
}