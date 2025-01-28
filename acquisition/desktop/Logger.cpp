/*********************************************************************************************/
/*
 *  File name:      Logger.cpp
 *
 *  Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/

#include "Logger.h"

// define static member variables
Logger* Logger::mpInstance = NULL;
std::ofstream Logger::mLogfile;
std::mutex Logger::mMutex;

Logger::Logger(std::string filename)
    : mFileName(filename),
      mStartTime(clock())
{
    mLogfile.open(mFileName.c_str(), std::ios::out | std::ios::app);
}

Logger::~Logger(void)
{
    if (mpInstance != NULL)
    {
        mLogfile.close();
    }
}

Logger* Logger::GetInstance(const std::string filename)
{
    // lock for thread safety
    std::lock_guard<std::mutex> lock(mMutex);

    if (mpInstance == NULL)
    {
        static Logger logger(filename);
        mpInstance = &logger;
    }

    return mpInstance;
}

void Logger::Log(const std::string entry)
{
    // lock for thread safety
    std::lock_guard<std::mutex> lock(mMutex);

    mLogfile << CurrentExecutionTime() << '\t' << entry << "\n";
}

std::string Logger::CurrentExecutionTime(void)
{
    double curr_time = ((clock() - mStartTime) / (double)CLOCKS_PER_SEC);

    int msInt = std::abs(int(std::round(curr_time * 1000.0)));

    int mins = msInt / (1000 * 60) % 60;
    int secs = msInt / (1000) % 60;
    int mils = msInt % 1000;

    std::string buffer;
    buffer.resize(13);
    snprintf(&buffer[0], 13, "[%03d:%02d:%03d]", mins, secs, mils);

    return buffer;
}