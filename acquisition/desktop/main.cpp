/*********************************************************************************************/
/*
 *	File name:		Main.cpp
 *
 *	Synopsis:		Main executable for the multi-contrast laser illumination source.
 *					A command line interface (CLI) menu is launched in the terminal window
 *					for controlling the illumination source. Frames are continuously grabbed
 *					by the Matrox Orion HD video capture card and written to storage. The 
 *					frame intensity values are used to compute updated illumination pulse
 *					width lengths that are sent to the light modulation controller over
 *					Serial USB. The program assumes that that the Desktop is capable of
 *					grabbing and storing frames at the camera's frame acquisition rate.
 *
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 * 
 */
/*********************************************************************************************/


/*********************************************/
// HEADERS
/*********************************************/
#include <iostream>
#include <string>

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <mil.h>

#include "DeviceFunctions.cuh"
#include "LightController.h"
#include "Logger.h"
#include "MatroxCaptureCard.h"
#include "Types.h"


/*********************************************/
// PARAMETERS
/*********************************************/
const bool			MASK_TOGGLE				= 1;				// circular binary mask to exclude pixels from autoexposure
const unsigned int	MASK_CENTER_X			= 660;				// circle center x-coordinate (in pixels)
const unsigned int	MASK_CENTER_Y			= 610;				// circle center y-coordinate (in pixels)
const unsigned int	MASK_RADIUS				= 550;				// circle radius (in pixels)
const unsigned int	ROI_SIZE_X				= 1350;				// image width excluding black borders (in pixels)
const unsigned int	ROI_SIZE_Y				= 1080;				// image height excluding black borders (in pixels)
const unsigned int	ROI_OFFSET_X			= 550;				// black border width (in pixels)
const unsigned int	ROI_OFFSET_Y			= 0;				// black border height (in pixels)
const unsigned int	FRAMES_PER_VIDEO		= 200;				// number of frames stored in each video
const int			KBD101_SERIAL_NO		= 28252094;			// serial number for the direct-driver rotation mount
const std::string	OUTPUT_DIR				= "D:\\";			// output directory for videos and log file
const std::string	TEENSY_PORT				= "\\\\.\\COM6";	// USB serial port for light modulation controller Teensy 4.0 


/*********************************************/
// FUNCTION PROTOTYPES 
/*********************************************/

/*	Primary processing function called everytime a frame is grabbed from the capture buffer.
*/
MIL_INT MFTYPE PerFrameHookFunction(MIL_INT hookType, MIL_ID hookId, void* pHookData);

/*	Creates a circular mask in the provided host memory.
*/
void FillCircularMask(unsigned char* mask_host,
					  const unsigned int width,
					  const unsigned int height,
					  const unsigned int nchnls,
					  const unsigned int cx,
					  const unsigned int cy,
					  const unsigned int r);

/*	Prints user options to the CLI/terminal window.
*/
void PrintCliOptions(void);

/*	Updates the CLI title with relevant statistics.
*/
void UpdateCliTitle(MODE currMode,
					unsigned int noFramesGrbd,
					unsigned int noFramesMisd,
					bool isSynced);


/*********************************************/
// TYPE DEFINITIONS 
/*********************************************/
typedef struct {
	LightController*	mpLightCntrl;
	MatroxCaptureCard*	mpFrameGrbbr;
	MODE				mCurrMode;
	unsigned int		mNoGrabbedFrames;
	/* Host buffers. */
	unsigned char*		mpFb_host;
	unsigned char*		mpFbMainDisp_host;
	unsigned char*		mpFbExtDisp_host;
	/* Device buffers. */
	unsigned char*		mpFb_dev;
	unsigned char*		mpFbMainDisp_dev;
	unsigned char*		mpFbExtDisp_dev;
	unsigned char*		mpFbPrev_dev;
	unsigned char*		mpFbOddField_dev;
	unsigned char*		mpFbEvenField_dev;
	unsigned char*		mpFbMask_dev;
} HookDataStruct;


/*********************************************/
// MAIN
/*********************************************/
int main(void)
{
	// input pid - a unique id appended to every output file name
	std::string pid;
	std::cout << "Enter the patient ID: ";
	std::cin >> pid;
	system("cls");

	// initialize logging class and log output location
	Logger* pLogger = Logger::GetInstance(OUTPUT_DIR + pid + ".txt");

	// initialize light modulation controller
	LightController* pLightCntrl = new LightController(TEENSY_PORT, KBD101_SERIAL_NO);

	// initialize frame grabber
	MatroxCaptureCard* pFrameGrbbr = new MatroxCaptureCard(ROI{ROI_SIZE_X, ROI_SIZE_Y, ROI_OFFSET_X, ROI_OFFSET_Y},
														   OUTPUT_DIR, pid, FRAMES_PER_VIDEO);

	// allocate host memory for frame data
	unsigned char* pFb_host			= (unsigned char*)malloc(pFrameGrbbr->GetSize());
	unsigned char* pFbMainDisp_host	= (unsigned char*)malloc(pFrameGrbbr->GetSize());
	unsigned char* pFbExtDisp_host	= (unsigned char*)malloc(pFrameGrbbr->GetSize());

	// allocate device memory for frame data
	unsigned char* pFb_dev;				cudaMalloc((void**)&pFb_dev,			pFrameGrbbr->GetSize());
	unsigned char* pFbMainDisp_dev;		cudaMalloc((void**)&pFbMainDisp_dev,	pFrameGrbbr->GetSize());
	unsigned char* pFbExtDisp_dev;		cudaMalloc((void**)&pFbExtDisp_dev,		pFrameGrbbr->GetSize());
	unsigned char* pFbPrev_dev;			cudaMalloc((void**)&pFbPrev_dev,		pFrameGrbbr->GetSize());
	unsigned char* pFbOddField_dev;		cudaMalloc((void**)&pFbOddField_dev,	pFrameGrbbr->GetSize());
	unsigned char* pFbEvenField_dev;	cudaMalloc((void**)&pFbEvenField_dev,	pFrameGrbbr->GetSize());
	unsigned char* pFbMask_dev;			cudaMalloc((void**)&pFbMask_dev,		pFrameGrbbr->GetSize());

	// if enable, create a circular binary mask that excludes pixels outside of the mask from
	// the computed image statstics. This is useful when a straight cap is attatched
	// at the tip of the scope, causing saturated pixels.
	if (MASK_TOGGLE)
	{
		unsigned char* pFbMask_host = (unsigned char*)malloc(pFrameGrbbr->GetSize());

		FillCircularMask(pFbMask_host, pFrameGrbbr->GetWidth(), pFrameGrbbr->GetHeight(),
						 pFrameGrbbr->GetNoBands(), MASK_CENTER_X, MASK_CENTER_Y, MASK_RADIUS);

		cudaMemcpy(pFbMask_dev, pFbMask_host, pFrameGrbbr->GetSize(), cudaMemcpyHostToDevice);

		free(pFbMask_host);

		pLogger->Log("MASK\t"
					 + std::to_string(MASK_CENTER_X) + ","
					 + std::to_string(MASK_CENTER_Y) + ","
					 + std::to_string(MASK_RADIUS));
	}
	// otherwise, use all pixels
	else
		cudaMemset(pFbMask_dev, 255, pFrameGrbbr->GetSize());

	// initialize hook data structure
	HookDataStruct hookData;
	hookData.mCurrMode			= MODE::OFF;
	hookData.mNoGrabbedFrames	= 0;
	hookData.mpFrameGrbbr		= pFrameGrbbr;
	hookData.mpLightCntrl		= pLightCntrl;
	hookData.mpFb_host			= pFb_host;
	hookData.mpFbMainDisp_host	= pFbMainDisp_host;
	hookData.mpFbExtDisp_host	= pFbExtDisp_host;
	hookData.mpFb_dev			= pFb_dev;
	hookData.mpFbMainDisp_dev	= pFbMainDisp_dev;
	hookData.mpFbExtDisp_dev	= pFbExtDisp_dev;
	hookData.mpFbPrev_dev		= pFbPrev_dev;
	hookData.mpFbOddField_dev	= pFbOddField_dev;
	hookData.mpFbEvenField_dev	= pFbEvenField_dev;
	hookData.mpFbMask_dev		= pFbMask_dev;

	// initialize the illumination mode
	pLightCntrl->SetMode(hookData.mCurrMode);

	// start video grabbing w/ the hook function (PerFrameHookFunction)
	pFrameGrbbr->StartGrabFrames(PerFrameHookFunction, &hookData);

	// loop until exit
	char cmd = ' ';
	while (cmd != 'x')
	{
		// update CLI display
		system("cls");
		PrintCliOptions();

		// queue the user input to take effect on the next frame grab
		std::cin >> cmd;
		switch (cmd)
		{
			case '0': hookData.mCurrMode = MODE::OFF;		break;
			case '1': hookData.mCurrMode = MODE::WLI;		break;
			case '2': hookData.mCurrMode = MODE::PSE;		break;
			case '3': hookData.mCurrMode = MODE::LSCI;		break;
			case '4': hookData.mCurrMode = MODE::MULTI;		break;
			case '5': hookData.mCurrMode = MODE::SSFDI;		break;
			case '6': hookData.mCurrMode = MODE::WARMUP;	break;
			case '7': hookData.mCurrMode = MODE::SYNC;		break;
			case 'x': std::cout << "Exiting!" << std::endl;	break;
			default: break;
		}
	}

	// stop video grabbing
	pFrameGrbbr->StopGrabFrames(PerFrameHookFunction, &hookData);

	// cleanup
	delete pFrameGrbbr;
	delete pLightCntrl;
	free(pFb_host);
	free(pFbMainDisp_host);
	free(pFbExtDisp_host);
	cudaFree(pFb_dev);
	cudaFree(pFbMainDisp_dev);
	cudaFree(pFbExtDisp_dev);
	cudaFree(pFbPrev_dev);
	cudaFree(pFbOddField_dev);
	cudaFree(pFbEvenField_dev);
	cudaFree(pFbMask_dev);

	return 0;
}


/*********************************************/
// FUNCTION DEFINITIONS 
/*********************************************/

MIL_INT MFTYPE PerFrameHookFunction(MIL_INT hookType, MIL_ID hookId, void* cbData)
{
	HookDataStruct* pData = (HookDataStruct*)cbData;

	// log the frame grab
	Logger* pLogger = Logger::GetInstance();
	pLogger->Log("GRAB\t" + std::to_string(pData->mNoGrabbedFrames));

	// if the light mode was changed in the CLI
	if (pData->mpLightCntrl->GetMode() != pData->mCurrMode)
	{
		// update the illumination mode
		pData->mpLightCntrl->SetMode(pData->mCurrMode);

		// clear the image display and temporal processing buffers
		cudaMemset(pData->mpFbMainDisp_dev,	0, pData->mpFrameGrbbr->GetSize());
		cudaMemset(pData->mpFbExtDisp_dev,	0, pData->mpFrameGrbbr->GetSize());
		cudaMemset(pData->mpFbPrev_dev,		0, pData->mpFrameGrbbr->GetSize());
	}

	// copy the grabbed frame to host memory
	pData->mpFrameGrbbr->CopyMilToHostBuff(hookId, pData->mpFb_host);

	// upload the frame to device memory
	cudaMemcpy(pData->mpFb_dev, pData->mpFb_host, pData->mpFrameGrbbr->GetSize(), cudaMemcpyHostToDevice);

	// seperate the frame into odd and even image fields
	Deinterlace(pData->mpFb_dev, pData->mpFbOddField_dev, pData->mpFbEvenField_dev,
				pData->mpFrameGrbbr->GetWidth(), pData->mpFrameGrbbr->GetHeight(),
				pData->mpFrameGrbbr->GetNoBands());

	// compute the mean of each color channel for odd and even image fields
	float oddChnlMeans[3] = { 0 };
	MaskedChannelMean(pData->mpFbOddField_dev, pData->mpFbMask_dev, &oddChnlMeans[0],
					  pData->mpFrameGrbbr->GetWidth(), pData->mpFrameGrbbr->GetHeight(),
					  pData->mpFrameGrbbr->GetNoBands());
	float evenChnlMeans[3] = { 0 };
	MaskedChannelMean(pData->mpFbEvenField_dev, pData->mpFbMask_dev, &evenChnlMeans[0],
					  pData->mpFrameGrbbr->GetWidth(), pData->mpFrameGrbbr->GetHeight(),
					  pData->mpFrameGrbbr->GetNoBands());

	// compute updated pulse widths for autoexposure using the image intensity values
	// send updated pulse widths to the light modulation controller
	// recieve power monitoring unit values from the light modulation controller
	pData->mpLightCntrl->IncrementPrgrm(oddChnlMeans, evenChnlMeans);

	// update the desktop display with subplotted images of each program step
	SubplotImg( pData->mpFbOddField_dev, pData->mpFbMainDisp_dev,
				pData->mpFrameGrbbr->GetWidth(), pData->mpFrameGrbbr->GetHeight(),
				pData->mpFrameGrbbr->GetNoBands(),
				ceil(sqrt(pData->mpLightCntrl->GetPrgrmLength())),
				ceil(sqrt(pData->mpLightCntrl->GetPrgrmLength())),
				((2 * pData->mNoGrabbedFrames)) % pData->mpLightCntrl->GetPrgrmLength());
	SubplotImg( pData->mpFbEvenField_dev, pData->mpFbMainDisp_dev,
				pData->mpFrameGrbbr->GetWidth(), pData->mpFrameGrbbr->GetHeight(),
				pData->mpFrameGrbbr->GetNoBands(),
				ceil(sqrt(pData->mpLightCntrl->GetPrgrmLength())),
				ceil(sqrt(pData->mpLightCntrl->GetPrgrmLength())),
				((2 * pData->mNoGrabbedFrames) + 1) % pData->mpLightCntrl->GetPrgrmLength());
	cudaMemcpy(pData->mpFbMainDisp_host, pData->mpFbMainDisp_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToHost);
	pData->mpFrameGrbbr->CopyHostBuffToMilDisp(pData->mpFbMainDisp_host, DISPLAY::MAIN);

	// update the external display
	switch (pData->mpLightCntrl->GetMode())
	{
		// off: copy raw image
		case(MODE::OFF):
			cudaMemcpy(pData->mpFbExtDisp_dev, pData->mpFb_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToDevice);
			break;
		// wli: copy raw image
		case(MODE::WLI):
			cudaMemcpy(pData->mpFbExtDisp_dev, pData->mpFb_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToDevice);
			break;
		// pse: blend directional images
		case(MODE::PSE):
			Average(pData->mpFbOddField_dev, pData->mpFbEvenField_dev, pData->mpFbPrev_dev,
					pData->mpFbExtDisp_dev,  pData->mpFrameGrbbr->GetWidth(), pData->mpFrameGrbbr->GetHeight(),
					pData->mpFrameGrbbr->GetNoBands());
			break;
		// lsci: display only odd field (WLI)
		case(MODE::LSCI):
			cudaMemcpy(pData->mpFbExtDisp_dev, pData->mpFbOddField_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToDevice);
			break;
		// multi: blend directional images
		case(MODE::MULTI):
			Average(pData->mpFbOddField_dev, pData->mpFbEvenField_dev, pData->mpFbPrev_dev,
					pData->mpFbExtDisp_dev, pData->mpFrameGrbbr->GetWidth(), pData->mpFrameGrbbr->GetHeight(),
					pData->mpFrameGrbbr->GetNoBands());
			break;
		// ssfdi: copy raw frame
		case(MODE::SSFDI):
			cudaMemcpy(pData->mpFbExtDisp_dev, pData->mpFbOddField_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToDevice);
			break;
		// warmup: copy raw frame
		case(MODE::WARMUP):
			cudaMemcpy(pData->mpFbExtDisp_dev, pData->mpFb_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToDevice);
			break;
		// sync: copy raw frame
		case(MODE::SYNC):
			cudaMemcpy(pData->mpFbExtDisp_dev, pData->mpFb_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToDevice);
			break;
	}
	cudaMemcpy(pData->mpFbExtDisp_host, pData->mpFbExtDisp_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToHost);
	pData->mpFrameGrbbr->CopyHostBuffToMilDisp(pData->mpFbExtDisp_host, DISPLAY::EXTERNAL);

	// retain even field for frame blending at next frame grab
	cudaMemcpy(pData->mpFbPrev_dev, pData->mpFbEvenField_dev, pData->mpFrameGrbbr->GetSize(), cudaMemcpyDeviceToDevice);

	// save the frame to the hard disk
	pData->mpFrameGrbbr->ArchiveFrame(hookId);

	// increment the frame count
	pData->mNoGrabbedFrames++;

	// update the CLI title with current stats
	UpdateCliTitle(pData->mpLightCntrl->GetMode(),
				   pData->mNoGrabbedFrames,
				   pData->mpFrameGrbbr->GetNoMissedFrames(),
				   pData->mpLightCntrl->GetSyncStatus());
	
	return 0;
}

void FillCircularMask(unsigned char* mask_host,
					  const unsigned int width,
					  const unsigned int height,
					  const unsigned int nchnls,
					  const unsigned int cx,
					  const unsigned int cy,
					  const unsigned int r)
{
	for (int c = 0; c < nchnls; c++)
		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
				mask_host[y * width * nchnls + x * nchnls + c] =
					(pow(x - (int)cx, 2) + pow(y - (int)cy, 2)) <= pow(r, 2) ? 255 : 0;
}


void PrintCliOptions(void)
{
	std::cout << "-----Multimodal Laser Endoscopy-----" << std::endl;
	std::cout << "(0):  OFF"	<< std::endl;
	std::cout << "(1):  WLI"	<< std::endl;
	std::cout << "(2):  PSE"	<< std::endl;
	std::cout << "(3):  LSCI"	<< std::endl;
	std::cout << "(4):  MULTI"	<< std::endl;
	std::cout << "(5):  SSFDI" << std::endl;
	std::cout << "(6):  WARMUP" << std::endl;
	std::cout << "(7):  SYNC"	<< std::endl;
	std::cout << "(x):  QUIT"	<< std::endl;
}


void UpdateCliTitle(MODE currMode,
					unsigned int noFramesGrbd,
					unsigned int noFramesMisd,
					bool isSynced)
{
	std::wstring title = L"Multimodal Laser Endoscopy";
	title += L" | ";

	// current illumination mode
	switch (currMode)
	{
		case(MODE::OFF):	title += L"OFF";		break;
		case(MODE::WLI):	title += L"WLI";		break;
		case(MODE::PSE):	title += L"PSE";		break;
		case(MODE::LSCI):	title += L"LSCI";		break;
		case(MODE::MULTI):	title += L"MULTI";		break;
		case(MODE::WARMUP):	title += L"WARMUP";		break;
		case(MODE::SYNC):	title += L"SYNC";		break;
	}
	title += L" | ";

	// number of frames grabbed
	title += L"Grabbed: ";
	title += std::to_wstring(noFramesGrbd);
	title += L" | ";

	// number of frames missed
	title += L"Missed: ";
	title += std::to_wstring(noFramesMisd);
	title += L" | ";

	if(isSynced)title += L"SYNCED";
	else		title += L"NOT SYNCED";

	SetConsoleTitle(title.c_str());
}