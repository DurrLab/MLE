/*********************************************************************************************/
/*
 *	File name:		RotationMount.h
 *
 *	Synopsis:		Class for managing communication with the direct drive rotation mount.
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/

#ifndef ROTATIONMOUNT_H_
#define ROTATIONMOUNT_H_

constexpr unsigned int      POS_SF          = 4000;     // scale factor
constexpr unsigned int      VELOCITY        = 1800;     // degrees/sec
constexpr unsigned int      ACCELERATION    = 10476;    // degrees/sec/sec

class RotationMount
{
public:

    /*	Class constructor.
    *
    *	@param rotnMntSerialNo	serial number for the direct drive rotation mount
    */
    RotationMount(const int serialNo);

    /*	Class destructor.
    */
    ~RotationMount(void);

    /*  Initializes the connection with the rotation mount.
    * 
    *	@return	true if the connection succeeds, false if it fails to connect
    */
    bool Initialize(void);

    /*	Class constructor.
    *
    *	@param position	        sets the rotation mount to this position (in degrees)
    */
    void SetPosition(float position);

private:

    const int   mSerialNo;
    bool        mInitialized;
    float       mPosition;
    char        mSerialNoStr[16];
};

#endif /*! ROTATIONMOUNT_H_ */