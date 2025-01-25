/*********************************************************************************************/
/*
 *	File name:		Logger.h
 *
 *	Synopsis:		Thread safe logging class for outputting debugging information to a
 *                  text file. A singleton implementation is used to prevent the creation
 *                  of multiple log outputs.
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/

#ifndef LOGGER_H_
#define LOGGER_H_

#include <cstdarg>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include <time.h>

class Logger
{
    public:

        /*	Class destructor.
        */
        ~Logger(void);

        /*	Get the singleton instance of the Logger class. If the instance does not exist,
         *	it is created.
         *
         *	@param filename		Name of the log file to be used by the Logger instance.
         *	@return				Pointer to the singleton Logger instance.
         */
        static Logger* GetInstance(const std::string filename = "");

        /*	Log an entry to the log file with the current execution time.
         *
         *	@param entry	    The log entry to be written to the log file.
         */
        void Log(const std::string entry);

    private:

        /*	Private constructor to prevent direct instantiation. Opens the log file for writing.
        * 
        *	@param filename		Name of the log file to open.
        */
        Logger(const std::string filename);

        /*	Prevent copy assignment.
        */
        Logger& operator=(const Logger&) { return *this; };

        /*	Calculates the time since the Logger instance was created.
        *
        *	@return			    Time in the format [minutes:seconds:milliseconds]
        */
        std::string CurrentExecutionTime(void);

    private:

        static Logger* mpInstance;
        static std::mutex mMutex;
        static std::ofstream mLogfile;

        const std::string mFileName;
        const clock_t mStartTime;
};
#endif  /* LOGGER_H_ */