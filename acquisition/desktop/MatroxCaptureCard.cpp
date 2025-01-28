/*********************************************************************************************/
/*
 *  File name:        MatroxCaptureCard.cpp
 * 
 *  Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
/*********************************************************************************************/

#include "MatroxCaptureCard.h"

#include <iostream>

#include "Logger.h"

MatroxCaptureCard::MatroxCaptureCard(ROI roi, std::string outputDir, std::string pid, unsigned int fpv)
    : mRoi(roi),
      mFramesPerVideo(fpv),
      mBaseVideoFilename(outputDir + pid + "-"),
      mMilApp(0),
      mMilSys(0),
      mMilDig(0),
      mMilDispMain(0),
      mMilDispExt(0),
      mNoArchivedFrames(0),
      mFramesPerSecond(0)
{
    // allocate the default MIL system set in MIL Config
    MappAllocDefault(M_DEFAULT, &mMilApp, &mMilSys, M_NULL, &mMilDig, M_NULL);

    // inquire the raw frame size and fps
    mMilSizeX           = MdigInquire(mMilDig, M_SIZE_X, M_NULL);
    mMilSizeY           = MdigInquire(mMilDig, M_SIZE_Y, M_NULL);
    mMilNumBands        = MdigInquire(mMilDig, M_SIZE_BAND, M_NULL);
    mFramesPerSecond    = MdigInquire(mMilDig, M_SELECTED_FRAME_RATE, M_NULL);

    // configure the digitizer and display for the desired ROI
    MdigControl(mMilDig, M_SOURCE_SIZE_X,   mRoi.width);
    MdigControl(mMilDig, M_SOURCE_SIZE_Y,   mRoi.height);
    MdigControl(mMilDig, M_SOURCE_OFFSET_X, mRoi.xOffset);
    MdigControl(mMilDig, M_SOURCE_OFFSET_Y, mRoi.yOffset);

    // allocate two display windows: a main display on the desktop monitor for the research team (mMilDispMain)
    // and an exclusive display output through the Matrox card for the clinical monitor
    MdispAlloc(mMilSys, M_NULL,    MIL_TEXT("M_CENTER"),  M_WINDOWED,  &mMilDispMain);
    MdispAlloc(mMilSys, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_EXCLUSIVE, &mMilDispExt);

    // set the title bar text for the windowed display
    MdispControl(mMilDispMain, M_TITLE, 
                 M_PTR_TO_DOUBLE(MIL_TEXT("Multimodal Laser Endoscopy | Main Display")));
   
    // scale the displays to match the monitor size; retain aspect ratio
    MdispControl(mMilDispMain, M_SCALE_DISPLAY, M_ENABLE);
    MdispControl(mMilDispExt, M_SCALE_DISPLAY, M_ENABLE);

    // set the pixels outside of the display buffer to black
    MdispControl(mMilDispMain, M_BACKGROUND_COLOR, M_COLOR_BLACK);
    MdispControl(mMilDispExt, M_BACKGROUND_COLOR, M_COLOR_BLACK);

    // allocate MIL frame buffers
    for (int n = 0; n < BUFFER_SIZE; n++)
    {
        MbufAllocColor(mMilSys, mMilNumBands, mRoi.width, mRoi.height,
                       8L + M_UNSIGNED, M_IMAGE + M_GRAB + M_PROC, &mMilFbCapBuff[n]);
        MbufClear(mMilFbCapBuff[n], 0x00);
    }

    MbufAllocColor(mMilSys, mMilNumBands, mMilSizeX, mMilSizeY,
                   8L + M_UNSIGNED, M_IMAGE + M_DISP, &mMilFbMainDispFull);
    MbufClear(mMilFbMainDispFull, 0x00);
 
    MbufAllocColor(mMilSys, mMilNumBands, mMilSizeX, mMilSizeY,
                   8L + M_UNSIGNED, M_IMAGE + M_DISP, &mMilFbExtDispFull);
    MbufClear(mMilFbExtDispFull, 0x00);

    MbufAllocColor(mMilSys, mMilNumBands, mRoi.width, mRoi.height,
                   8L + M_UNSIGNED, M_IMAGE + M_DISP, &mMilFbMainDispRoi);
    MbufClear(mMilFbMainDispRoi, 0x00);

    MbufAllocColor(mMilSys, mMilNumBands, mRoi.width, mRoi.height,
                   8L + M_UNSIGNED, M_IMAGE + M_DISP, &mMilFbExtDispRoi);
    MbufClear(mMilFbExtDispRoi, 0x00);

    // assign frame buffers to the displays
    MdispSelect(mMilDispMain, mMilFbMainDispRoi);
    MdispSelect(mMilDispExt,  mMilFbExtDispRoi);

    // open the first video
    OpenVideo();
}

MatroxCaptureCard::~MatroxCaptureCard(void)
{
    // close the current video
    CloseVideo();

    // free MIL resources
    for (int n = 0; n < BUFFER_SIZE; n++)
        MbufFree(mMilFbCapBuff[n]);

    MbufFree(mMilFbMainDispFull);
    MbufFree(mMilFbExtDispFull);
    MbufFree(mMilFbMainDispRoi);
    MbufFree(mMilFbExtDispRoi);

    MdispFree(mMilDispMain);
    MdispFree(mMilDispExt);

    MappFreeDefault(mMilApp, mMilSys, M_NULL, mMilDig, M_NULL);
}

void MatroxCaptureCard::StartGrabFrames(MIL_DIG_HOOK_FUNCTION_PTR pHookFunction, void* pHookData)
{
    MdigProcess(mMilDig, mMilFbCapBuff, BUFFER_SIZE, M_START, M_DEFAULT, pHookFunction, pHookData);
}

void MatroxCaptureCard::StopGrabFrames(MIL_DIG_HOOK_FUNCTION_PTR pHookFunction, void* pHookData)
{
    MdigProcess(mMilDig, mMilFbCapBuff, BUFFER_SIZE, M_STOP, M_DEFAULT, pHookFunction, pHookData);
}

void MatroxCaptureCard::CopyMilToHostBuff(MIL_ID hookId, unsigned char* hostBuff)
{
    // get the MIL ID for the current frame in the hook function
    MIL_ID milFb = 0;
    MdigGetHookInfo(hookId, M_MODIFIED_BUFFER + M_BUFFER_ID, &milFb);

    // copy the the MIL-managed buffer to the host buffer
    MbufGetColor2d(milFb, M_PACKED + M_BGR24, M_ALL_BANDS,
                   0, 0, mRoi.width, mRoi.height, hostBuff);
}

void MatroxCaptureCard::CopyHostBuffToMilDisp(unsigned char* hostBuff, DISPLAY disp)
{
    // copy the host memory to the MIL-managed buffer for the selected display
    if (disp == DISPLAY::MAIN)
        MbufPutColor2d(mMilFbMainDispRoi, M_PACKED + M_BGR24, M_ALL_BANDS,
                       0, 0, mRoi.width, mRoi.height, hostBuff);
    if (disp == DISPLAY::EXTERNAL)
        MbufPutColor2d(mMilFbExtDispRoi, M_PACKED + M_BGR24, M_ALL_BANDS,
                       0, 0, mRoi.width, mRoi.height, hostBuff);
}

void MatroxCaptureCard::ArchiveFrame(MIL_ID hookId)
{
    MIL_ID milFb = 0;
    MdigGetHookInfo(hookId, M_MODIFIED_BUFFER + M_BUFFER_ID, &milFb);

    MbufExportSequence(std::wstring(mCurrentVideoFilename.begin(), mCurrentVideoFilename.end()).c_str(), M_DEFAULT, &milFb, 1, M_DEFAULT, M_WRITE);
    Logger* pLogger = Logger::GetInstance();
    pLogger->Log("FRAME\t"+std::to_string(mNoArchivedFrames));
    mNoArchivedFrames++;

    // if the video is filled, start a new one
    if (mNoArchivedFrames % mFramesPerVideo == 0)
    {
        CloseVideo();
        OpenVideo();
    }
}

unsigned int MatroxCaptureCard::GetWidth(void)
{
    return (unsigned int)mRoi.width;
}

unsigned int MatroxCaptureCard::GetHeight(void)
{
    return (unsigned int)mRoi.height;
}

unsigned int MatroxCaptureCard::GetNoBands(void)
{
    return (unsigned int)mMilNumBands;
}

unsigned int MatroxCaptureCard::GetSize(void)
{
    return (unsigned int)(mRoi.width * mRoi.height * mMilNumBands * sizeof(unsigned char));
}

unsigned int MatroxCaptureCard::GetNoArchivedFrames(void)
{
    return mNoArchivedFrames;
}

unsigned int MatroxCaptureCard::GetNoMissedFrames(void)
{
    MIL_INT missedFrameCount = 0;
    MdigInquire(mMilDig, M_PROCESS_FRAME_MISSED, &missedFrameCount);

    return (unsigned int)missedFrameCount;
}

void MatroxCaptureCard::OpenVideo(void)
{
    mCurrentVideoFilename = mBaseVideoFilename + std::to_string(mNoArchivedFrames / mFramesPerVideo) + ".avi";

    MbufExportSequence(std::wstring(mCurrentVideoFilename.begin(), mCurrentVideoFilename.end()).c_str(),
                        M_AVI_DIB, M_NULL, M_NULL, M_DEFAULT, M_OPEN);

    Logger* pLogger = Logger::GetInstance();
    pLogger->Log("VIDEO\tOpened " + mCurrentVideoFilename);
}

void MatroxCaptureCard::CloseVideo(void)
{
    MbufExportSequence(std::wstring(mCurrentVideoFilename.begin(), mCurrentVideoFilename.end()).c_str(),
                        M_DEFAULT, M_NULL, M_NULL, mFramesPerSecond, M_CLOSE);

    Logger* pLogger = Logger::GetInstance();
    pLogger->Log("VIDEO\tClosed " + mCurrentVideoFilename);
}