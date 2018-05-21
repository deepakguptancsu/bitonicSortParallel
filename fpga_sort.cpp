#include <stdio.h>
#include <stdlib.h>
#include "mysort.h"
#include "CL/opencl.h"
#include "AOCL_Utils.h"

using namespace aocl_utils;

// OpenCL runtime configuration
cl_platform_id platform = NULL;
unsigned num_devices = 0;
scoped_array<cl_device_id> device; // num_devices elements
cl_context context = NULL;
scoped_array<cl_command_queue> queue; // num_devices elements
cl_program program = NULL;
scoped_array<cl_kernel> kernel; // num_devices elements

bool init_opencl();

int fpga_sort(int num_of_elements, float *data)
{
  init_opencl();
  
  cl_int status;
  cl_event write_event[2];

  //creating buffer for input data, which is to be sorted
  cl_mem deviceData = clCreateBuffer(context, CL_MEM_READ_WRITE , sizeof(float) * num_of_elements, NULL, &status);
  checkError(status, "Failed to create buffer for input data");

  //creating buffer for stride length
  //stride length signifies the distance between two variables
  //which are to be compared in bitonic sort
  cl_mem strideLen_kernel = clCreateBuffer(context, CL_MEM_READ_WRITE , sizeof(int), NULL, &status);
  checkError(status, "Failed to create buffer for stride length");

  //writing data to device for sorting  
  status = clEnqueueWriteBuffer(queue[0], deviceData, CL_FALSE,
      0, num_of_elements * sizeof(float), data, 0, NULL, &write_event[0]);
  checkError(status, "Failed to transfer input data");

  // Setting data as kernel argument.
  status = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), &deviceData);
  checkError(status, "Failed to set argument 0");

  //saving global work size
  const size_t global_work_size = num_of_elements;

	//iterating through input array block by block
	for(int numBlocks = num_of_elements/2; numBlocks >= 1; numBlocks = numBlocks/2)
	{
    int blockSize = num_of_elements/numBlocks;
    const size_t local_work_size = blockSize;

    for(int strideLength = blockSize/2; strideLength >= 1; strideLength = strideLength/2)
    {
      status = clEnqueueWriteBuffer(queue[0], strideLen_kernel, CL_TRUE,
          0, sizeof(int), &strideLength, 0, NULL, &write_event[1]);
      checkError(status, "Failed to transfer input stride length");

      //setting stride length as kernel argument      
      status = clSetKernelArg(kernel[0], 1, sizeof(cl_mem), &strideLen_kernel);
      checkError(status, "Failed to set argument 1");

      //invoking kernel
      status = clEnqueueNDRangeKernel(queue[0], kernel[0], 1, NULL,
          &global_work_size, &local_work_size, 2, write_event, NULL);
      checkError(status, "Failed to launch kernel");
    }
  }

  // Release local events.
  clReleaseEvent(write_event[0]);
  clReleaseEvent(write_event[1]);
  
  //copying result from device back to data
  status = clEnqueueReadBuffer(queue[0], deviceData, CL_TRUE,
    0, num_of_elements * sizeof(float), data, 0, NULL, NULL);

  return 0;
}

// Initializes the OpenCL objects.
bool init_opencl() {
  cl_int status;

  printf("Initializing OpenCL\n");

  if(!setCwdToExeDir()) {
    return false;
  }

  // Get the OpenCL platform.
  platform = findPlatform("Altera");
  if(platform == NULL) {
    printf("ERROR: Unable to find Altera OpenCL platform.\n");
    return false;
  }

  // Query the available OpenCL device.
  device.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));
  printf("Platform: %s\n", getPlatformName(platform).c_str());
  printf("Using %d device(s)\n", num_devices);
  for(unsigned i = 0; i < num_devices; ++i) {
    printf("  %s\n", getDeviceName(device[i]).c_str());
  }

  // Create the context.
  context = clCreateContext(NULL, num_devices, device, NULL, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the program for all device. Use the first device as the
  // representative device (assuming all device are of the same type).
  std::string binary_file = getBoardBinaryFile("fpgasort", device[0]);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), device, num_devices);

  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  // Create per-device objects.
  queue.reset(num_devices);
  kernel.reset(num_devices);
  for(unsigned i = 0; i < num_devices; ++i) {
    // Command queue.
    queue[i] = clCreateCommandQueue(context, device[i], CL_QUEUE_PROFILING_ENABLE, &status);
    checkError(status, "Failed to create command queue");

    // Kernel.
    const char *kernel_name = "fpgasort";
    kernel[i] = clCreateKernel(program, kernel_name, &status);
    checkError(status, "Failed to create kernel");

  }

  return true;
}

void cleanup() {
  for(unsigned i = 0; i < num_devices; ++i) {
    if(kernel && kernel[i]) {
      clReleaseKernel(kernel[i]);
    }
    if(queue && queue[i]) {
      clReleaseCommandQueue(queue[i]);
    }
  }

  if(program) {
    clReleaseProgram(program);
  }
  if(context) {
    clReleaseContext(context);
  }
}

