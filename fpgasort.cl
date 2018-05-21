__kernel void fpgasort(__global float *restrict data, 
                        __global int *restrict strideLength)
{
    // get index of the work item
    int index = get_global_id(0);
    int blockStartAddress = get_group_id(0) * get_local_size(0);

    //computation is performed on only selected indices
    //as according to bitonic sort algorithm
    if( ((index-blockStartAddress)/(*strideLength)) % 2 == 0 )
    {
        //even numbered block will be sorted in increasing order
        //while odd numbered block is sorted in decreasing order
        if(get_group_id(0) % 2 == 0)
        {
            if(data[index] > data[index + *strideLength])
            {
                float temp = data[index];
                data[index] = data[index + *strideLength];
                data[index + *strideLength] = temp;
            }
        }
        else
        {
            if(data[index] < data[index + *strideLength])
            {
                float temp = data[index];
                data[index] = data[index + *strideLength];
                data[index + *strideLength] = temp;
            }
        }
    }
}

