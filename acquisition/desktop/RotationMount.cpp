/*********************************************************************************************/
/*
 *	File name:		RotationMount.cpp
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/


#include "RotationMount.h"

#include <conio.h>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include "Thorlabs.MotionControl.KCube.BrushlessMotor.h"

RotationMount::RotationMount(const int serialNo)
:   mSerialNo(serialNo),
    mInitialized(0),
    mPosition(0.f)
{
    sprintf_s(mSerialNoStr, "%d", mSerialNo);
}

RotationMount::~RotationMount(void)
{
    if (mInitialized)
    {
        BMC_StopPolling(mSerialNoStr);
        BMC_Close(mSerialNoStr);
    }
}

bool RotationMount::Initialize(void)
{
    // build list of connected devices
    short nConnected = TLI_BuildDeviceList();

    // get KBD serial numbers
    char serialNos[100];
    TLI_GetDeviceListByTypeExt(serialNos, 100, 28); // Third input is device prefix

    // search serial numbers for desired serial number
    if (!strstr(serialNos, mSerialNoStr))
    {
        printf("Rotation mount device %s not found\n", mSerialNoStr);
        return 0;
    }

    // if found, open device
    if (!(BMC_Open(mSerialNoStr) == 0))
        return 0;

    printf("Connected to rotation mount %s\n", mSerialNoStr);

    // start the device polling at 3 ms intervals
    BMC_StartPolling(mSerialNoStr, 3);

    // enable device so that it can rotate
    BMC_EnableChannel(mSerialNoStr);

    Sleep(1000);

    // set velocity and acceleration parameters
    BMC_SetVelParams(mSerialNoStr, ACCELERATION * POS_SF, VELOCITY * POS_SF);

    // home the device
    BMC_ClearMessageQueue(mSerialNoStr);
    BMC_Home(mSerialNoStr);
    printf("Homing rotation mount...\n");

    // wait to receive message from device that it is home
    WORD messageType;
    WORD messageId;
    DWORD messageData;
    BMC_WaitForMessage(mSerialNoStr, &messageType, &messageId, &messageData);
    while (messageType != 2 || messageId != 0)
    {
        BMC_WaitForMessage(mSerialNoStr, &messageType, &messageId, &messageData);
    }

    mInitialized = 1;

    return 1;
}

void RotationMount::SetPosition(float position)
{
    mPosition = position;

    BMC_MoveToPosition(mSerialNoStr, int(mPosition * POS_SF));
}