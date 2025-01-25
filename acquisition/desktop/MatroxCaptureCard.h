/*********************************************************************************************/
/*
 *	File name:		MatroxCaptureCard.h
 *
 *	Synopsis:		Wrapper class for acquiring frames from the Matrox Orion HD frame grabber
 *					using the Matrox Imaging Library (MIL). 
 *
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/

#ifndef MATROXCAPTURECARD_H_
#define MATROXCAPTURECARD_H_

#include <string>

#include <mil.h>
#include <milim.h>

#include "Types.h"

// defines the size of the frame buffer in non-paged memory. The likelihood of dropping frames
// decreases with increasing buffer size
#define BUFFER_SIZE 7

// ROI for discarding black border pixels around the image when processed by the digitizer
typedef struct ROI { int width, height, xOffset, yOffset; } ROI;

class MatroxCaptureCard
{
	public:
		
		/*	Class constructor.
		* 
		*	@param roi				used to discard black border pixels around the image at the digitizer
		*	@param outputDir		output directory for video files
		*	@param pid				patient id appended to video file names
		*	@param fpv				number of frames per video file
		*/
		MatroxCaptureCard(ROI roi, std::string outputDir, std::string pid, unsigned int fpv);

		/*	Class destructor.
		*/
		~MatroxCaptureCard(void);

		/*	Start aynchronous frame grabbing and processing.
		*
		*	@param pHookFunction	hook function executed at each frame grab
		*	@param pHookData		hook data used by hook function
		*/
		void StartGrabFrames(MIL_DIG_HOOK_FUNCTION_PTR pHookFunction, void* pHookData);

		/*	Stop aynchronous frame grabbing.
		*/
		void StopGrabFrames(MIL_DIG_HOOK_FUNCTION_PTR pHookFunction, void* pHookData);


		/*	Copy pixel data from the current MIL-owned frame buffer to host-owned memory
		*
		*	@param hookId			MIL hook id for the current hook iteration
		*	@param hostBuff			pointer to host-owned memory. The size of allocated memory
		*							must be equal to the value returned by GetSize().
		*/
		void CopyMilToHostBuff(MIL_ID hookId, unsigned char* hostBuff);

		/*	Copy pixel data from host-owned memory to the display window
		*
		*	@param hostBuff			pointer to host-owned memory. The size of allocated memory
		*							must be equal to the value returned by GetSize().
		*	@param disp				display type that the image should be copied to.
		*/
		void CopyHostBuffToMilDisp(unsigned char* hostBuff, DISPLAY disp);

		/*	Write the current frame to memory. When the size of the video equal fpv, close the
		*	video and open a new file.
		*
		*	@param hookId			MIL hook id for the current hook iteration
		*/
		void ArchiveFrame(MIL_ID hookId);

		/*	
		*	@return	image width in pixels
		*/
		unsigned int GetWidth(void);

		/*
		*	@return	image height in pixels
		*/
		unsigned int GetHeight(void);

		/*
		*	@return	number of image color channels
		*/
		unsigned int GetNoBands(void);

		/*
		*	@return	number of image bits (width * height * channels * 8 bits)
		*/
		unsigned int GetSize(void);

		/*
		*	@return	number of frames saved to memory
		*/
		unsigned int GetNoArchivedFrames(void);

		/*
		*	@return	number of frames dropped by the digitizer
		*/
		unsigned int GetNoMissedFrames(void);

	private:

		/*
		*	Opens a new uncompressed AVI video and stores the name in mCurrentVideoFilename.
		*/
		void OpenVideo(void);

		/*
		*	Closes the current video.
		*/
		void CloseVideo(void);

	private:

		/* MIL system parameters. */
		MIL_ID	mMilApp,
				mMilSys,
				mMilDig;

		/* MIL displays. */
		MIL_ID  mMilDispMain,
				mMilDispExt;

		/* MIL frame buffers. */
		MIL_ID  mMilFbCapBuff[BUFFER_SIZE],
				mMilFbMainDispFull,
				mMilFbMainDispRoi,
				mMilFbExtDispFull,
				mMilFbExtDispRoi;

		/* Raw video signal size and color bands. */
		MIL_INT	mMilSizeX,
				mMilSizeY,
				mMilNumBands;

		/* ROI for cropping black pixel. */
		ROI		mRoi;

		/* Video writing parameters. */
		const std::string	mBaseVideoFilename;
		const unsigned int	mFramesPerVideo;
		unsigned int		mNoArchivedFrames;
		double				mFramesPerSecond;
		std::string			mCurrentVideoFilename;

};
#endif /*! MATROXCAPTURECARD_H_ */