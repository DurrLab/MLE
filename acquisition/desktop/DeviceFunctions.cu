/*********************************************************************************************/
/*
 *	File name:		DeviceFunctions.cu
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/

#include "DeviceFunctions.cuh"

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cooperative_groups.h>


__global__ void average_kernel(unsigned char* inImgA_dev,
							   unsigned char* inImgB_dev,
							   unsigned char* inImgC_dev,
							   unsigned char* outImg_dev,
							   const unsigned int width,
							   const unsigned int height,
							   const unsigned int noChnls)
{
	int pixelX = threadIdx.x + blockIdx.x * blockDim.x;
	int pixelY = threadIdx.y + blockIdx.y * blockDim.y;

	// bounds handling
	if (pixelX >= width)  return;
	if (pixelY >= height) return;

	// average value for each color channel
	for (int c = 0; c < noChnls; c++)
	{
		unsigned char valA = inImgA_dev[width*noChnls*pixelY + pixelX*noChnls + c];
		unsigned char valB = inImgB_dev[width*noChnls*pixelY + pixelX*noChnls + c];
		unsigned char valC = inImgC_dev[width*noChnls*pixelY + pixelX*noChnls + c];

		outImg_dev[width*noChnls*pixelY + pixelX*noChnls + c] = (valA + valB + valC) / 3;
	}
}

__host__ void Average(unsigned char* inImgA_dev,
					  unsigned char* inImgB_dev,
					  unsigned char* inImgC_dev,
					  unsigned char* outImg_dev,
					  const unsigned int width,
					  const unsigned int height,
					  const unsigned int noChnls)
{
	const dim3 blockSize(32, 32);
	const dim3 gridSize(width / blockSize.x + 1, height / blockSize.y + 1);

	average_kernel <<<gridSize, blockSize >>> (inImgA_dev,
											   inImgB_dev,
											   inImgC_dev,
											   outImg_dev,
											   width,
											   height,
											   noChnls);

	cudaDeviceSynchronize();
}


__global__ void deinterlace_kernel(unsigned char* inImg_dev,
								   unsigned char* outOddImg_dev,
								   unsigned char* outEvenImg_dev,
								   const unsigned int width,
								   const unsigned int height,
								   const unsigned int noChnls)
{
	int pixelX = threadIdx.x + blockIdx.x * blockDim.x;
	int pixelY = threadIdx.y + blockIdx.y * blockDim.y;

	// bounds handling
	if (pixelX >= width)  return;
	if (pixelY >= height) return;

	int pixelYPrev = (pixelY == 0) ? 0 : pixelY - 1;
	int pixelYNext = (pixelY == (height-1)) ? height - 1 : pixelY + 1;

	// interpolate the values for each color channel
	for (int c = 0; c < noChnls; c++)
	{
		unsigned char valCurr = inImg_dev[width*noChnls *pixelY     + pixelX*noChnls + c];
		unsigned char valPrev = inImg_dev[width*noChnls *pixelYPrev + pixelX*noChnls + c];
		unsigned char valNext = inImg_dev[width*noChnls *pixelYNext + pixelX*noChnls + c];

		// if pixel belongs to an even field (zero-indexed)
		if (pixelY % 2)
		{
			// assign even field original value
			outEvenImg_dev[width*noChnls*pixelY + pixelX*noChnls + c] = valCurr;

			// assign odd field average of previous and next row
			outOddImg_dev[width*noChnls*pixelY + pixelX*noChnls + c] = (valPrev + valNext) / 2;
		}
		// if pixel belongs to an odd field (zero-indexed)
		else
		{
			// assign odd field original value
			outOddImg_dev[width*noChnls*pixelY + pixelX*noChnls + c] = valCurr;

			// assign even field average of previous and next row
			outEvenImg_dev[width*noChnls*pixelY + pixelX*noChnls + c] = (valPrev + valNext) / 2;
		}
	}
}

__host__ void Deinterlace(unsigned char* inImg_dev,
						  unsigned char* outOddImg_dev,
						  unsigned char* outEvenImg_dev,
						  const unsigned int width,
						  const unsigned int height,
						  const unsigned int noChnls)
{
	const dim3 blockSize(32, 32);
	const dim3 gridSize(width / blockSize.x + 1, height / blockSize.y + 1);

	deinterlace_kernel<<<gridSize,blockSize>>>( inImg_dev,
												outOddImg_dev,
												outEvenImg_dev,
												width,
												height,
												noChnls);

	cudaDeviceSynchronize();
}


__global__ void maskedChnlMean_kernel(unsigned char* inImg_dev,
									  unsigned char* inMask_dev,
                                      int* chnlSums_dev,
									  int* chnlNoPxs_dev,
                                      unsigned int noChnls,
									  unsigned int N)
{
    // gather thread information
    auto g = cooperative_groups::this_thread_block();
    int bTid = g.thread_rank();                         // local thread id (0:blockSize-1)
    int gTid = blockIdx.x * blockDim.x + threadIdx.x;   // global thread id


    // shared memory within thread block. Split into two segments:
    // *threadChnlSums* for storing the cumulative sum of valid (non-masked) pixels
    // *threadChnlNoPxs* for counting the number of valid (non-masked) pixels
    extern __shared__ int sharedMem[];
    int* threadChnlSums = (int*)sharedMem;
    int* threadChnlNoPxs = (int*)(&threadChnlSums[noChnls * g.size()]);

    /// initialize shared memory to zero
    for (int c = 0; c < noChnls; c++)
    {
        threadChnlSums[c * g.size() + bTid] = 0;
        threadChnlNoPxs[c * g.size() + bTid] = 0;
    }

    // each thread moves through the input data and retains one initial sum value
    for (int i = gTid; i < N * noChnls; i += blockDim.x * gridDim.x)
    {
        threadChnlSums[(i % noChnls) * g.size() + bTid] += inMask_dev[i] > 0 ? inImg_dev[i] : 0;
        threadChnlNoPxs[(i % noChnls) * g.size() + bTid] += inMask_dev[i] > 0 ? 1 : 0;
    }

    // synchronize the threads in the block to ensure all initialization is complete
    g.sync();

    // use binary reduction to reduce the shared memory to a single sum.
    // each iteration halves the number of active threads
	// each thread adds its partial sum[i] to sum[lane+i]
    for (int i = g.size() / 2; i > 0; i /= 2)
    {
        if (bTid < i)
        {
            for (int c = 0; c < noChnls; c++)
            {
                threadChnlSums[c * g.size() + bTid] += threadChnlSums[c * g.size() + bTid + i];
                threadChnlNoPxs[c * g.size() + bTid] += threadChnlNoPxs[c * g.size() + bTid + i];
            }
        }
        g.sync();
    }

    // store the result in global memory using atomics
    if (bTid == 0)
    {
        for (int c = 0; c < noChnls; c++)
        {
            atomicAdd(&chnlSums_dev[c], threadChnlSums[c * g.size()]);
            atomicAdd(&chnlNoPxs_dev[c], threadChnlNoPxs[c * g.size()]);
        }
    }
}

__host__ void MaskedChannelMean(unsigned char* inImg_dev,
                                unsigned char* inMask_dev,
                                float* outChnlMeans_host,
                                const unsigned int width,
                                const unsigned int height,
                                const unsigned int noChnls)
{
    int blockSize = 1024;
    int gridSize  = 32;
    int sharedBytesSize = blockSize * noChnls * 2 * sizeof(int);

    int* chnlSums_dev;
	cudaMalloc((void**)&chnlSums_dev, noChnls * sizeof(int));
    cudaMemset(chnlSums_dev, 0, noChnls * sizeof(int));

    int* chnlNoPxs_dev;
	cudaMalloc((void**)&chnlNoPxs_dev, noChnls * sizeof(int));
    cudaMemset(chnlNoPxs_dev, 0, noChnls * sizeof(int));

    maskedChnlMean_kernel <<<gridSize, blockSize, sharedBytesSize >>> (inImg_dev,
																	   inMask_dev,
																	   chnlSums_dev,
																	   chnlNoPxs_dev,
																	   noChnls,
																	   width * height);
    cudaDeviceSynchronize();

    // copy sum value to host
    int* chnlSums_host = (int*)malloc(noChnls * sizeof(int));
    cudaMemcpy(chnlSums_host, chnlSums_dev, noChnls * sizeof(int), cudaMemcpyDeviceToHost);
    int* chnlNoPxs_host = (int*)malloc(noChnls * sizeof(int));
    cudaMemcpy(chnlNoPxs_host, chnlNoPxs_dev, noChnls * sizeof(int), cudaMemcpyDeviceToHost);

    // compute mean from sum and store in provided array
    for (unsigned int c = 0; c < noChnls; c++)
        outChnlMeans_host[c] = (float)chnlSums_host[c] / (float)chnlNoPxs_host[c];

    // clean up
    cudaFree(chnlSums_dev);
    cudaFree(chnlNoPxs_dev);
    free(chnlSums_host);
    free(chnlNoPxs_host);
}

__global__ void subplotimg_kernel(unsigned char* inImg_dev,
								  unsigned char* outImg_dev,
								  const unsigned int width,
								  const unsigned int height,
								  const unsigned int noChnls,
								  const unsigned int outWidth,
								  const unsigned int outHeight,
								  const unsigned int startX,
								  const unsigned int startY)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x;
	int y = threadIdx.y + blockIdx.y * blockDim.y;

	// bounds handling
	if (x >= outWidth)  return;
	if (y >= outHeight) return;

	// output pixel index
	int pixelX = startX + x;
	int pixelY = startY + y;

	// sample nearest neighbor value
	int pixelXNearest = x * (width / outWidth);
	int pixelYNearest = y * (height / outHeight);

	if (pixelXNearest > width)  pixelXNearest = width;
	if (pixelYNearest > height) pixelYNearest = height;

	for (int c = 0; c < noChnls; c++)
		outImg_dev[width*noChnls*pixelY + pixelX * noChnls + c] =
			inImg_dev[width * noChnls * pixelYNearest + pixelXNearest * noChnls + c];

}

__host__ void SubplotImg( unsigned char* inImg_dev,
						  unsigned char* outImg_dev,
						  const unsigned int width,
						  const unsigned int height,
						  const unsigned int noChnls,
						  const unsigned int cols,
						  const unsigned int rows,
						  const unsigned int idx )
{
	const unsigned int outWidth	 = width/cols;
	const unsigned int outHeight = height/rows;
	const unsigned int startX	 = outWidth*(idx%cols);
	const unsigned int startY	 = outHeight*(idx/cols);

	const dim3 blockSize(32, 32);
	const dim3 gridSize(outWidth / blockSize.x + 1, outHeight / blockSize.y + 1);

	subplotimg_kernel <<<gridSize, blockSize>>> (inImg_dev,
												 outImg_dev,
												 width,
												 height,
												 noChnls,
												 outWidth,
												 outHeight,
												 startX,
												 startY);

	cudaDeviceSynchronize();
}
