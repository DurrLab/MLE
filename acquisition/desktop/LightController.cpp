/*********************************************************************************************/
/*
 *	File name:		LightController.cpp
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/


#include "LightController.h"

#include <bitset>
#include <iostream>
#include <string>

#include <math.h>

#include "Logger.h"

/*********************************************/
// LIGHT PROGRAM DEFINITIONS 
/*********************************************/

static const prgrm wle_prgrm	= {{{1.f,0.85f,0.85f,1.f,0.85f,0.85f,1.f,0.85f,0.85f,0.f,0.f,0.f,0.f,0.f,0.f},	IMG_CHNL::MONO}};
static const prgrm pse_prgrm	= {{{0.85f,0.85f,0.85f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f},		IMG_CHNL::MONO},
								   {{0.f,0.f,0.f,0.85f,0.85f,0.85f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f},		IMG_CHNL::MONO},
								   {{0.f,0.f,0.f,0.f,0.f,0.f,0.85f,0.85f,0.85f,0.f,0.f,0.f,0.f,0.f,0.f},		IMG_CHNL::MONO}};
static const prgrm lsci_prgrm	= {{{1.f,0.85f,0.85f,1.f,0.85f,0.85f,1.f,0.85f,0.85f,0.f,0.f,0.f,0.f,0.f,0.f},	IMG_CHNL::MONO},
								   {{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,1.f},				IMG_CHNL::RED}};
static const prgrm multi_prgrm  = {{{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f,0.f},				IMG_CHNL::BLUE},
								   {{0.f,0.7f,0.f,0.f,1.0f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f},				IMG_CHNL::GREEN},
								   {{0.7f,0.f,0.f,1.0f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f},			IMG_CHNL::RED},
								   {{0.f,0.f,0.7f,0.f,0.f,1.0f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f},			IMG_CHNL::BLUE},
								   {{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f},				IMG_CHNL::GREEN},
								   {{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,1.f,0.f},				IMG_CHNL::RED},
								   {{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f},				IMG_CHNL::BLUE},
								   {{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f},				IMG_CHNL::GREEN}};
static const prgrm ssfdi_prgrm =  {{{0.f,0.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f},				IMG_CHNL::RED},
								   {{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,1.f},				IMG_CHNL::RED}};
static const prgrm warmup_prgrm = {{{1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f},				IMG_CHNL::MONO},
								   {{1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f},				IMG_CHNL::MONO}};
static const prgrm off_prgrm	= {{{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f},				IMG_CHNL::MONO}};



LightController::LightController(const std::string teensyPort,
								 const int rotnMntSerialNo)
:	mPrgrmCount(0),
	mBufferOffset(0),
	mSynced(0),
	mFid(0)
{
	// open serial USB port to light modulation controller.
	// open on a seperate thread to avoid blocking.
	mThreadedSerial = new ThreadedSerial(teensyPort);

	// send the reset signal to the light modulation controller (fid=-1)
	mThreadedSerial->AddToTxQueue(output_entry{FID_RESET,{0}});

	// initialize the illumination mode as OFF
	SetMode(MODE::OFF);

	// initialize the direct drive rotation mount
	mpRotnMnt = new RotationMount(rotnMntSerialNo);
	mpRotnMnt->Initialize();
	mpRotnMnt->SetPosition(Power2RotnAngle(0.1f));
}

LightController::~LightController(void)
{
	delete mThreadedSerial;
	delete mpRotnMnt;
}

void LightController::SetMode(MODE mode)
{
	// if we are not synced, only allow a mode change to sync/warmup/off
	if (!mSynced && !(mode==MODE::SYNC || mode==MODE::WARMUP || mode == MODE::OFF))
		return;

	// if in SSFDI mode, we will use pulse width modulation on the high coherence channel,
	// so set the rotation angle for 100% power
	if(mode==MODE::SSFDI)
		mpRotnMnt->SetPosition(Power2RotnAngle(1.0f));

	// store as current mode
	mMode = mode;

	// log the mode change
	Logger* pLogger = Logger::GetInstance();
	pLogger->Log("MODE\t" + std::to_string(mode));

	if (mode == MODE::SYNC)
	{
		// reset the buffer offset if starting synchronization
		mBufferOffset = 0;
		mPrgrm = off_prgrm;
		mSynced = 0;
	}
	else
	{
		// assign the program sequence
		switch (mMode)
		{
			case MODE::WLE:		mPrgrm = wle_prgrm;		break;
			case MODE::PSE:		mPrgrm = pse_prgrm;		break;
			case MODE::LSCI:	mPrgrm = lsci_prgrm;	break;
			case MODE::MULTI:	mPrgrm = multi_prgrm;	break;
			case MODE::SSFDI:	mPrgrm = ssfdi_prgrm;	break;
			case MODE::WARMUP:	mPrgrm = warmup_prgrm;	break;
			case MODE::OFF:		mPrgrm = off_prgrm;		break;
			default:									break;
		}

		// reset buffers for start of new program
		float temp;
		while (mPwrBuffer.try_dequeue(temp)) {};
		while (mRotBuffer.try_dequeue(temp)) {};
		while (mFrmBuffer.try_dequeue(temp)) {};
	}

	// reset program step count
	mPrgrmCount = 0;
}

void LightController::IncrementPrgrm(float* oddBgrVals, float* evenBgrVals)
{
    Logger* pLogger = Logger::GetInstance();

    // output power for odd and even fields
    float pwrs[2 * NO_LASER_DIODES] = { 0 };

    // if we are in sync mode and not yet synchronized
    if ((mMode == MODE::SYNC) && !mSynced)
    {
        // if sync mode just started, send a sync pulse w/ all diodes enabled
        if (mBufferOffset == 0)
        {
            for (int i = 0; i < NO_LASER_DIODES; i++)
                pwrs[i] = 1.0f;
            mBufferOffset += 2;
        }
        // otherwise, check if the sync pulse has arrived
        else
        {
            if (((oddBgrVals[0] + oddBgrVals[1] + oddBgrVals[2]) / 3.f) > 40.f)
            {
                mSynced = 1;
                pLogger->Log("SYNCED\t");
                pLogger->Log("BUFF\t" + std::to_string(mBufferOffset));
            }
            else
            {
                mBufferOffset += 2;
            }
        }

        // increment the count for both the odd and even image fields
        mPrgrmCount += 2;
    }
    // if we are in warmup mode
    else if (mMode == MODE::WARMUP)
    {
        // set all diodes to 50% power
        for (int i = 0; i < NO_LASER_DIODES; i++)
        {
            pwrs[i] = mPrgrm[0].first[i];
            pwrs[NO_LASER_DIODES + i] = mPrgrm[1].first[i];
        }

        // increment the count for both the odd and even image fields
        mPrgrmCount += 2;
    }
    // for all other modes
    else
    {
        // for each image field
        for (int f = 0; f < 2; f++)
        {
            unsigned int prgrmIdx = (mPrgrmCount) % mPrgrm.size();
            unsigned int frameIdx = (mPrgrmCount - mBufferOffset) % mPrgrm.size();

            // If we have exceeded the buffer offset, start saving the frame intensity values for autoexposure
            if (mPrgrmCount >= mBufferOffset)
            {
                // get intensity value to use for autoexposure
                float vals[3];
                if (f)
                    memcpy(vals, evenBgrVals, 3 * sizeof(float));
                else
                    memcpy(vals, oddBgrVals, 3 * sizeof(float));

                float prevIntensity = 0.f;
                switch (mPrgrm[frameIdx].second)
                {
                case IMG_CHNL::BLUE:
                    prevIntensity = vals[0];
                    break;
                case IMG_CHNL::GREEN:
                    prevIntensity = vals[1];
                    break;
                case IMG_CHNL::RED:
                    prevIntensity = vals[2];
                    break;
                case IMG_CHNL::MONO:
                    prevIntensity = (vals[0] + vals[1] + vals[2]) / 3.f;
                    break;
                }

                // log intensity value
                std::string valString = "VALS\t" + std::to_string(prevIntensity);
                pLogger->Log(valString);

                // Get the illumination power value that produced this image intensity
                float prevPwr = 0.f;
                mPwrBuffer.try_dequeue(prevPwr);

                // compute the power update
                float newPwr = ClampPwr(UpdatePower(prevIntensity, prevPwr));

                // store the updated power
                mFrmBuffer.enqueue(newPwr);
            }

            // if we have exceeded the buffer offset and we have made a full pass through the program, then start doing autoexposure
            float newPwr = 0.f;
            if (mPrgrmCount >= mPrgrm.size() * (int)ceilf((float)mBufferOffset / (float)mPrgrm.size()))
                mFrmBuffer.try_dequeue(newPwr);
            else
                newPwr = PWR_START;

            mPwrBuffer.enqueue(newPwr);

            // assign the overall power to power per channel
            for (int n = 0; n < NO_LASER_DIODES; n++)
                if (mPrgrm[prgrmIdx].first[n] > 0)
                    pwrs[(f * NO_LASER_DIODES) + n] = newPwr * mPrgrm[prgrmIdx].first[n];

            // increment the program step count
            mPrgrmCount++;
        }
    }

    // convert the laser output powers to pulse width lengths
    unsigned short pws[2 * NO_LASER_DIODES] = { 0 };
    for (int n = 0; n < 2 * NO_LASER_DIODES; n++)
        pws[n] = (unsigned short)(PW_MAX * pwrs[n]);

    // if in LSCI mode, force the LSCI channel to a constant pulse width
    if (mMode == MODE::LSCI)
    {
        pws[29] = PW_LSCI;

        // update rotation mount power
        float newPwr = 0.f;
        if (mPrgrmCount >= mBufferOffset - (10 * 2))
        {
            float intensity = evenBgrVals[2];
            float prevPwr = 0.f;
            mRotBuffer.try_dequeue(prevPwr);
            newPwr = ClampPwr(UpdatePower(intensity, prevPwr));
        }
        else
        {
            newPwr = 0.2f;
        }

        // store rotation mount power
        mRotBuffer.enqueue(newPwr);
        mpRotnMnt->SetPosition(Power2RotnAngle(newPwr));
        pLogger->Log("ROTN\t" + std::to_string(newPwr));
    }

    // send pulse width length to teensy
    output_entry oentry{ mFid, {0} };
    memcpy(oentry.pws, pws, sizeof(unsigned short) * 2 * NO_LASER_DIODES);
    mThreadedSerial->AddToTxQueue(oentry);

    // log pulse widths
    std::string oLogEntry = "PWS\t" + std::to_string(oentry.fid);
    for (int n = 0; n < 2 * NO_LASER_DIODES; n++)
        oLogEntry = oLogEntry + "," + std::to_string(oentry.pws[n]);
    pLogger->Log(oLogEntry);

    // receive photodiode voltages from teensy if available
    input_entry ientry;
    if (mThreadedSerial->GetFromRxQueue(ientry))
    {
        // if we receive an error from the teensy, log an error
        if (ientry.fid == FID_ERROR)
        {
            pLogger->Log("ERR\t");
        }
        // log photodiode voltages
        else
        {
            std::string iLogEntry = "PDV\t" + std::to_string(ientry.fid);
            for (int n = 0; n < 2 * NO_PHOTO_DIODES; n++)
                iLogEntry = iLogEntry + "," + std::to_string(ientry.pdvs[n]);
            pLogger->Log(iLogEntry);
        }
    }

    // increment frame id
    mFid++;
}

MODE LightController::GetMode(void)
{
    return mMode;
}

unsigned int LightController::GetPrgrmLength(void)
{
    return mPrgrm.size();
}

unsigned int LightController::GetPrgrmCount(void)
{
    return mPrgrmCount;
}

bool LightController::GetSyncStatus(void)
{
    return mSynced;
}

float LightController::ClampPwr(float pwr)
{
    if (pwr < PWR_MIN)       pwr = PWR_MIN;
    else if (pwr > PWR_MAX)  pwr = PWR_MAX;
    return pwr;
}

float LightController::UpdatePower(float prevIntensity, float prevPwr)
{
    float yFixed = (float)MAX_IMG_INTENSITY + 1.0f;

    float alpha = (yFixed - (float)TARGET_IMG_INTENSITY) * PWR_MAX;

    float newPwr = ((yFixed - prevIntensity) * prevPwr * (float)PWR_MAX) /
                   (((float)TARGET_IMG_INTENSITY - prevIntensity) * prevPwr + alpha);

    if (newPwr > 0.999f) newPwr = 0.999f;

    return newPwr;
}

float LightController::Power2RotnAngle(float power)
{
    return (ROT_ANG_MAX - ROT_ANG_MIN) * power + ROT_ANG_MIN;
}