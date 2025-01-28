/***********************************************************************************/
/*
 *	File name:	ThreadedSerial.cpp
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */

#include "ThreadedSerial.h"

ThreadedSerial::ThreadedSerial(const std::string port)
: mThreadRunning(true)
{
    // connect to serial port
    char errorOpening = mSerialPort.openDevice(port.c_str(), BAUD_RATE);

    if (!errorOpening)
        std::cout << "Device not found at " + port << std::endl;

    std::cout << "Device found at " + port << std::endl;

    // flush the input buffer
    mSerialPort.flushReceiver();

    // start the worker thread. */
    mThread = std::thread(&ThreadedSerial::Start, this);
}

ThreadedSerial::~ThreadedSerial(void)
{
    Stop();
}

void ThreadedSerial::AddToTxQueue(output_entry oentry)
{
    mOutputQueue.enqueue(oentry);
}

bool ThreadedSerial::GetFromRxQueue(input_entry &ientry)
{
    return mInputQueue.try_dequeue(ientry);
}

void ThreadedSerial::Start(void)
{
    while (mThreadRunning)
    {
        output_entry oentry;
        if (mOutputQueue.try_dequeue(oentry))
            mSerialPort.writeBytes(&oentry, sizeof(output_entry));

        if (mSerialPort.available() > sizeof(input_entry))
        {
            input_entry ientry;
            int error = mSerialPort.readBytes(&ientry, sizeof(input_entry));
            mInputQueue.enqueue(ientry);
        }

        // poll every 3 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL));
    }
}

void ThreadedSerial::Stop(void)
{
    mThreadRunning = false;

    mThread.join();

    mSerialPort.closeDevice();
}