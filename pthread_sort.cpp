  // CSC456: Name -- Deepak Gupta
  // CSC456: Student ID # -- 200206821 (dgupta22@ncsu.edu)
  // CSC456: I am implementing -- Bitonic Sort {What parallel sorting algorithm using pthread}
  // CSC456: Feel free to modify any of the following. You can only turnin this file.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <sys/time.h>
#include <pthread.h>
#include "mysort.h"
#include <algorithm>

using namespace std;

//defining macros
#define INC 0
#define DEC 1
#define NUM_THREAD 4

//defining argument structure for sending multiple arguments
//to a thread
struct argStruct
{
	int start;
	int end;
	int blockSize;
	float *data;
};

void* bitonicSort(void *arguments);
void orderBlock(int begin, int length, float *data, int order);


//sorting in increasing order using Bitonic sort
int pthread_sort(int num_of_elements, float *data)
{
	//iterating through input array block by block
	for (int blockSize = 2; blockSize <= num_of_elements; blockSize = blockSize*2)
	{
		pthread_t thread[NUM_THREAD];
		int numBlocks = num_of_elements/blockSize;
		int numBlocks_per_thread = max(numBlocks/NUM_THREAD, 1);
		argStruct args[NUM_THREAD];
		int numThread = min(numBlocks,NUM_THREAD);

		//creating threads and assigning blocks to each thread
		for(int i=0; i<numThread; i++)
		{
			args[i].start = i*(numBlocks_per_thread*blockSize);
			args[i].end = (i+1)*(numBlocks_per_thread*blockSize);
			args[i].blockSize = blockSize;
			args[i].data = data;
			if(pthread_create(&thread[i],NULL,&bitonicSort,(void*)&args[i]))
				fprintf(stderr,"Thread creation failed!!!!!\n");
		}			

		//waiting for each thread to finish execution
		for(int i=0; i<NUM_THREAD; i++)
			pthread_join(thread[i], NULL);
	}	
	
    return 0;
}

//this funtion is called by every thread to run on the blocks
//assiged to it
void* bitonicSort(void *arguments)
{
	argStruct *args = (argStruct *) arguments; 

	//iterating through each block one by one
	for (int i = args->start; i < args->end; i += args->blockSize)
	{
		//even numbered block will be sorted in increasing order
		//while odd numbered block is sorted in decreasing order
		if ((i / args->blockSize) % 2 == 0)
			orderBlock(i, args->blockSize, args->data, INC);
		else
			orderBlock(i, args->blockSize, args->data, DEC);
	}
}

//this function orders the block either in inc or dec order
void orderBlock(int begin, int length, float *data, int order)
{
    if (1 == length)
        return;

    int splitLen = length/2;

    for (int i = begin; i < begin + splitLen; i++)
    {
        if((INC == order) && (data[i] > data[i + splitLen]))
                swap(data[i], data[i + splitLen]);
        else if((DEC == order) && (data[i] < data[i + splitLen]))
                swap(data[i], data[i + splitLen]);
    }

    orderBlock(begin, splitLen, data, order);
    orderBlock(begin + splitLen, splitLen, data, order);
}


