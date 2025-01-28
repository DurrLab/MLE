/*********************************************************************************************/
/*
 *	File name:		ThreadedSerial.h
 *
 *	Synopsis:		Thread-safe class for asynchronous communication with
 *					the light modulation controller.
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/

#ifndef THREADEDSERIAL_H_
#define THREADEDSERIAL_H_

#include <string>
#include <utility>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

#include "concurrentqueue.h"
#include "serialib.h"
#include "Types.h"

constexpr unsigned int NO_O_DIODES      = 15;        // teensy laser diode channels
constexpr unsigned int NO_I_DIODES      = 3;         // teensy photodiode channels
constexpr unsigned int POLLING_INTERVAL = 3;         // milliseconds
constexpr unsigned int BAUD_RATE        = 115200;    // baud rate for comms with teensy

typedef struct {
    int            fid;
    unsigned short pws[2*NO_O_DIODES];
} output_entry;

typedef struct {
    int            fid;
    unsigned short pdvs[2*NO_I_DIODES];
} input_entry;

class ThreadedSerial
{
public:
    ThreadedSerial(const std::string port);

    ~ThreadedSerial(void);

    void AddToTxQueue(output_entry oentry);

    bool GetFromRxQueue(input_entry &ientry);

private:
    void Start(void);

    void Stop(void);

private:
    serialib                                    mSerialPort;
    moodycamel::ConcurrentQueue<output_entry>   mOutputQueue;
    moodycamel::ConcurrentQueue<input_entry>    mInputQueue;
    std::thread                                 mThread;
    bool                                        mThreadRunning;
};

#endif /*! THREADEDSERIAL_H_ */