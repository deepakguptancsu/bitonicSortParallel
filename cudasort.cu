#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>

#define THREADS 512
#ifdef __cplusplus
extern "C"
{
#endif

__global__ void bitonicSort(float *d_inputArray, int blockSize, int strideLength, int number_of_elements)
{
  int index = blockIdx.x*blockDim.x + threadIdx.x;

  if(index >= number_of_elements)
    return;
  
  int blockNumber = index/blockSize;
  int blockStartAddress = blockNumber*blockSize;

  //computation is performed on only selected indices
  //as according to bitonic sort algorithm
  if( ((index-blockStartAddress)/strideLength) % 2 != 0 )
    return;
  
  //even numbered block will be sorted in increasing order
  //while odd numbered block is sorted in decreasing order
  if(blockNumber % 2 == 0)
  {
    if(d_inputArray[index] > d_inputArray[index + strideLength])
    {
      float temp = d_inputArray[index];
      d_inputArray[index] = d_inputArray[index + strideLength];
      d_inputArray[index + strideLength] = temp;
    }
  }
  else
  {
    if(d_inputArray[index] < d_inputArray[index + strideLength])
    {
      float temp = d_inputArray[index];
      d_inputArray[index] = d_inputArray[index + strideLength];
      d_inputArray[index + strideLength] = temp;
    }
  }
}

int cuda_sort(int number_of_elements, float *a)
{
  //allocating memory on GPU device and copying data from host to GPU device 
 	float *d_inputArray;
	if(!cudaMalloc(&d_inputArray, sizeof(float) * number_of_elements) == cudaSuccess)
		printf("error in allocating d_inputArray\n");
  
  if(!cudaMemcpy(d_inputArray, a, sizeof(float) * number_of_elements, cudaMemcpyHostToDevice) == cudaSuccess)
    printf("error in copying d_inputArray\n");

	//iterating through input array block by block
	for (int blockSize = 2; blockSize <= number_of_elements; blockSize = blockSize*2)
	{
    //iterating through each block with differnt strideLength
    for(int strideLength = blockSize/2; strideLength >= 1; strideLength = strideLength/2)
    {
      bitonicSort<<<(number_of_elements/1024 + 1),1024>>>(d_inputArray, blockSize, strideLength, number_of_elements);
    }
  }

  //copying data back from GPU device to host memory
  if(!cudaMemcpy(a, d_inputArray, sizeof(float) * number_of_elements, cudaMemcpyDeviceToHost) == cudaSuccess)
    printf("error in copying d_inputArray from device to host\n");
  
  return 0;
}

#ifdef __cplusplus
}
#endif
