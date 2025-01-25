/*********************************************************************************************/
/*
 *	File name:		DeviceFunctions.cuh
 *
 *	Synopsis:		Contains function prototypes for GPU-accelerated device functions.
 *
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/


#pragma once

/*	Averages input frames and stores the result in the output frame. Pixel values should be packed.
*
*	@param inImgA_dev			pointer to first input frame in device memory
*	@param inImgB_dev			pointer to second input frame in device memory
*	@param inImgC_dev			pointer to thirs input frame in device memory
*	@param outImg_dev			pointer to output frame in device memory
*	@param width				image width in pixels
*	@param height				image height in pixels
*	@param noChnls				number of color channels
*/
void Average(unsigned char* inImgA_dev,
			 unsigned char* inImgB_dev,
			 unsigned char* inImgC_dev,
			 unsigned char* outImg_dev,
			 const unsigned int width,
			 const unsigned int height,
			 const unsigned int noChnls);

/*	Deinterlaces the input frame and resizes the odd/even fields to the original image height using
*	bilinear interpolation.
*
*	@param inFbA_dev			interlaced input frame in device memory
*	@param outFbOddField_dev	deinterlaced odd image field in device memory
*	@param outFbEvenField_dev	deinterlaced even image field in device memory
*	@param width				image width in pixels
*	@param height				image height in pixels
*	@param noChnls				number of color channels
*/
void Deinterlace(unsigned char* inImg_dev,
				 unsigned char* outOddImg_dev,
				 unsigned char* outEvenImg_dev,
				 const unsigned int width,
				 const unsigned int height,
				 const unsigned int noChnls);

/*	Computes the mean of each image channel using a sum reduction algorithm. Pixels equal to 0
*	in the image mask are excluded from the mean computation.
*
*	@param inFb_dev				input frame in device memory
*	@param inFbMask_dev			input image mask of size width x height in device memory
*	@param outChnlMeans			pointer to memory where mean value for each color channel will be stored
*	@param width				image width in pixels
*	@param height				image height in pixels
*	@param noChnls				number of color channels
*/
void MaskedChannelMean(unsigned char* inImg_dev,
					   unsigned char* inMask_dev,
					   float* outChnlMeans,
					   const unsigned int width,
					   const unsigned int height,
					   const unsigned int noChnls);

/*	Resizes the input image using nearest neighbor interpolation and stores it
*	in tile number idx in the output frame.
*	
*	@param inFb_dev				input frame in device memory
*	@param outFb_dev			output frame in device memory
*	@param width				image width in pixels
*	@param height				image height in pixels
*	@param noChnls				number of color channels
*	@param rows					number of subplot image rows
*	@param cols					number of subplot image columns
*	@param idx					tile index to store the image in (row major order)

*/
void SubplotImg(unsigned char* inImg_dev,
				unsigned char* outImg_dev,
				const unsigned int width,
				const unsigned int height,
				const unsigned int noChnls,
				const unsigned int cols,
				const unsigned int rows,
				const unsigned int idx );
