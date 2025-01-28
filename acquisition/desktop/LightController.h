/*********************************************************************************************/
/*
 *	File name:		LightController.h
 *
 *	Synopsis:		Class for managing communication with the light modulation controller and
 *					the direct drive rotation mount. Mean image intensity values are repeatedly
 *					input and used to update the pulse widths for auto exposure. The updated
 *					pulse widths are sent to the light modulation controller over Serial USB,
 *					and voltage values for each of the power monitoring units are recieved from
 *					the light modulation controller over Serial USB. The class also includes
 *					a mode for synchronizing the light source with the clinical video processor.
 *					The mode sends a single bright flash, then looks for the flash in the recorded
 *					frames to compute an offset. This offset is then used to synchronize output power
 *					values with corresponding image intensity values for auto exposure.
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/

#ifndef LIGHTCONTROLLER_H_
#define LIGHTCONTROLLER_H_

#include <string>
#include <utility>
#include <vector>

#include "concurrentqueue.h"
#include "RotationMount.h"
#include "serialib.h"
#include "ThreadedSerial.h"
#include "Types.h"

// define each illumination program as a repeating sequence of steps 
// with one step per image field. Each step contains a vector of pulse
// width weightings (one per diode) and the color channel that should
// be used for auto exposure updates. For example:
// 
// {{{1.0f, 0.8f, 0.3f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}, IMG_CHNL::MONO},
//  {{1.0f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},   IMG_CHNL::RED}}};
// 
// For odd image fields, the program will assign relative pulse widths lengths of 100%,
// 80%, and 30% to laser diodes 1-3 respectively, and the mean image intensity of all of
// the color channels will be used to compute updated pulse widths. For even image fields,
// laser diodes 1-2 will use the same pulse width lengths, and the mean image intensity
// of the red color channel will be used for auto exposure updates.
//
// The length of the float vector should be equal to NO_LASER_DIODES
//
// For our experiments, the laser diodes were wired to the light modulation controller
// pins in the following order:
// 635 nm	(RGB Unit 1)
// 522 nm	(RGB Unit 1)
// 446 nm	(RGB Unit 1)
// 635 nm	(RGB Unit 2)
// 522 nm	(RGB Unit 2)
// 446 nm	(RGB Unit 2)
// 635 nm	(RGB Unit 3)
// 522 nm	(RGB Unit 3)
// 446 nm	(RGB Unit 3)
// 406 nm
// 446 nm
// 543 nm
// 562 nm
// 657 nm
// 639 nm	(high coherence)
typedef std::vector<std::pair<std::vector<float>, IMG_CHNL>> prgrm;

constexpr unsigned int 	NO_LASER_DIODES 		= 15;		// number of teensy laser diode channels
constexpr unsigned int 	NO_PHOTO_DIODES 		= 3;		// number of teensy photodiode channels
constexpr unsigned int 	ROT_ANG_MAX 			= 310;		// half wave plate rotation angle for maximum power (degs)
constexpr unsigned int 	ROT_ANG_MIN 			= 265;		// half wave plate rotation angle for minimum power (degs)
constexpr int 			FID_RESET 				= -1;		// frame id valuesent to reset the teensy
constexpr int 			FID_ERROR 				= -2;		// frame id value sent by teensy indicating a runtime error
constexpr unsigned char MAX_IMG_INTENSITY 		= 255;		// maximum intensity value of image (8 bit)
constexpr unsigned char TARGET_IMG_INTENSITY 	= 128;		// target image intensity for autoexposure
constexpr float 		PW_MAX 					= 14000;	// maximum allowed pulse width (microseconds)
constexpr float 		PW_LSCI 				= 7000;		// pulse width length for high coherence laser in LSCI mode (microseconds)
constexpr float 		PWR_START 				= 0.2;		// power laser diodes are initialized at
constexpr float 		PWR_MAX 				= 1.0;		// maximum power value 
constexpr float 		PWR_MIN 				= 0.01;		// minimum power value

class LightController
{
	public:

		/*	Class constructor.
		*
		*	@param teensyPort		serial USB port number for the Teensy within the light modulation controller
		*	@param rotnMntSerialNo	serial number for the direct drive rotation mount
		*/
		LightController(const std::string teensyPort,
						const int rotnMntSerialNo);

		/*	Class destructor.
		*/
		~LightController(void);

		/*	Updates the illumination program to the specified mode.
		*/
		void SetMode(MODE mode);

		/*	Move to the next step in the current illumination program. This function should be
		*	called once per frame grab.
		*
		*	@param oddBgrVals		pointer to mean odd field channel intensity values in bgr order
		*	@param evenBgrVals		pointer to mean even field channel intensity values in bgr order
		*/
		void IncrementPrgrm(float* oddBgrVals,
							float* evenBgrVals);

		/*
		*	@return	the current illumination mode
		*/
		MODE GetMode(void);

		/*
		*	@return	number of steps in the current illumination program
		*/
		unsigned int GetPrgrmLength(void);

		/*
		*	@return	the number of steps taken since the start of the current illumination mode
		*/
		unsigned int GetPrgrmCount(void);

		/*
		*	@return whether the illumination source is frame synchronized
		*/
		bool GetSyncStatus(void);


	private:

		/*	Clamps the input value to POWER_MAX and POWER_MIN
		* 
		* 	@param pwr		value to be clamped
		* 
		*	@return	clamped value
		*/
		float ClampPwr(float pwr);

		/*	Computes updated illumination power using a modified secant root solving algorithm.
		*	The algorithm iteratively updates the power and converges to the desired mean
		*	image intensity (TARGET_IMG_INTENSITY).
		*
		* 	@param prevIntensity		measured image intensity
		* 	@param prevPwr				illumination power that produced prevIntensity
		*
		*	@return	updated illumination power
		*/
		float UpdatePower(float prevIntensity,
						  float prevPwr);

		/*	Converts the illumination power value to a rotation angle for the direct
		*	drive rotation mount.
		*
		* 	@param power				illumination power
		*
		*	@return	direct drive rotation mount angle
		*/
		float Power2RotnAngle(float power);


		ThreadedSerial*						mThreadedSerial;
		RotationMount*						mpRotnMnt;
		MODE								mMode;
		prgrm								mPrgrm;
		unsigned int						mPrgrmCount;
		unsigned int						mBufferOffset;
		unsigned int						mFid;
		moodycamel::ConcurrentQueue<float>	mPwrBuffer;
		moodycamel::ConcurrentQueue<float>	mFrmBuffer;
		moodycamel::ConcurrentQueue<float>	mRotBuffer;
		bool								mSynced;
};
#endif /*! LIGHTCONTROLLER_H_ */