/*!
 \file    serialib.cpp
 \brief   Source file of the class serialib. This class is used for communication over a serial device.
 \author  Philippe Lucidarme (University of Angers)
 \version 2.0
 \date    december the 27th of 2019

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


This is a licence-free software, it can be used by anyone who try to build a better world.
 */

#include "serialib.h"



 //_____________________________________
 // ::: Constructors and destructors :::


 /*!
     \brief      Constructor of the class serialib.
 */
serialib::serialib()
{
#if defined (_WIN32) || defined( _WIN64)
    // Set default value for RTS and DTR (Windows only)
    currentStateRTS = true;
    currentStateDTR = true;
    hSerial = INVALID_HANDLE_VALUE;
#endif
#if defined (__linux__) || defined(__APPLE__)
    fd = -1;
#endif
}


/*!
    \brief      Destructor of the class serialib. It close the connection
*/
// Class desctructor
serialib::~serialib()
{
    closeDevice();
}



//_________________________________________
// ::: Configuration and initialization :::



/*!
     \brief Open the serial port
     \param Device : Port name (COM1, COM2, ... for Windows ) or (/dev/ttyS0, /dev/ttyACM0, /dev/ttyUSB0 ... for linux)
     \param Bauds : Baud rate of the serial port.

                \n Supported baud rate for Windows :
                        - 110
                        - 300
                        - 600
                        - 1200
                        - 2400
                        - 4800
                        - 9600
                        - 14400
                        - 19200
                        - 38400
                        - 56000
                        - 57600
                        - 115200
                        - 128000
                        - 256000

               \n Supported baud rate for Linux :\n
                        - 110
                        - 300
                        - 600
                        - 1200
                        - 2400
                        - 4800
                        - 9600
                        - 19200
                        - 38400
                        - 57600
                        - 115200

               \n Optionally supported baud rates, depending on Linux kernel:\n
                        - 230400
                        - 460800
                        - 500000
                        - 576000
                        - 921600
                        - 1000000
                        - 1152000
                        - 1500000
                        - 2000000
                        - 2500000
                        - 3000000
                        - 3500000
                        - 4000000

     \param Databits : Number of data bits in one UART transmission.

            \n Supported values: \n
                - SERIAL_DATABITS_5 (5)
                - SERIAL_DATABITS_6 (6)
                - SERIAL_DATABITS_7 (7)
                - SERIAL_DATABITS_8 (8)
                - SERIAL_DATABITS_16 (16) (not supported on Unix)

     \param Parity: Parity type

            \n Supported values: \n
                - SERIAL_PARITY_NONE (N)
                - SERIAL_PARITY_EVEN (E)
                - SERIAL_PARITY_ODD (O)
                - SERIAL_PARITY_MARK (MARK) (not supported on Unix)
                - SERIAL_PARITY_SPACE (SPACE) (not supported on Unix)
    \param Stopbit: Number of stop bits

            \n Supported values:
                - SERIAL_STOPBITS_1 (1)
                - SERIAL_STOPBITS_1_5 (1.5) (not supported on Unix)
                - SERIAL_STOPBITS_2 (2)

     \return 1 success
     \return -1 device not found
     \return -2 error while opening the device
     \return -3 error while getting port parameters
     \return -4 Speed (Bauds) not recognized
     \return -5 error while writing port parameters
     \return -6 error while writing timeout parameters
     \return -7 Databits not recognized
     \return -8 Stopbits not recognized
     \return -9 Parity not recognized
  */
char serialib::openDevice(const char* Device, const unsigned int Bauds,
    SerialDataBits Databits,
    SerialParity Parity,
    SerialStopBits Stopbits) {
#if defined (_WIN32) || defined( _WIN64)
    // Open serial port
    hSerial = CreateFileA(Device, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,/*FILE_ATTRIBUTE_NORMAL*/0, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return -1; // Device not found

        // Error while opening the device
        return -2;
    }

    // Set parameters

    // Structure for the port parameters
    DCB dcbSerialParams;
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    // Get the port parameters
    if (!GetCommState(hSerial, &dcbSerialParams)) return -3;

    // Set the speed (Bauds)
    switch (Bauds)
    {
    case 110:     dcbSerialParams.BaudRate = CBR_110; break;
    case 300:     dcbSerialParams.BaudRate = CBR_300; break;
    case 600:     dcbSerialParams.BaudRate = CBR_600; break;
    case 1200:     dcbSerialParams.BaudRate = CBR_1200; break;
    case 2400:     dcbSerialParams.BaudRate = CBR_2400; break;
    case 4800:     dcbSerialParams.BaudRate = CBR_4800; break;
    case 9600:     dcbSerialParams.BaudRate = CBR_9600; break;
    case 14400:    dcbSerialParams.BaudRate = CBR_14400; break;
    case 19200:    dcbSerialParams.BaudRate = CBR_19200; break;
    case 38400:    dcbSerialParams.BaudRate = CBR_38400; break;
    case 56000:    dcbSerialParams.BaudRate = CBR_56000; break;
    case 57600:    dcbSerialParams.BaudRate = CBR_57600; break;
    case 115200:   dcbSerialParams.BaudRate = CBR_115200; break;
    case 128000:   dcbSerialParams.BaudRate = CBR_128000; break;
    case 256000:   dcbSerialParams.BaudRate = CBR_256000; break;
    default: return -4;
    }
    //select data size
    BYTE bytesize = 0;
    switch (Databits) {
    case SERIAL_DATABITS_5: bytesize = 5; break;
    case SERIAL_DATABITS_6: bytesize = 6; break;
    case SERIAL_DATABITS_7: bytesize = 7; break;
    case SERIAL_DATABITS_8: bytesize = 8; break;
    case SERIAL_DATABITS_16: bytesize = 16; break;
    default: return -7;
    }
    BYTE stopBits = 0;
    switch (Stopbits) {
    case SERIAL_STOPBITS_1: stopBits = ONESTOPBIT; break;
    case SERIAL_STOPBITS_1_5: stopBits = ONE5STOPBITS; break;
    case SERIAL_STOPBITS_2: stopBits = TWOSTOPBITS; break;
    default: return -8;
    }
    BYTE parity = 0;
    switch (Parity) {
    case SERIAL_PARITY_NONE: parity = NOPARITY; break;
    case SERIAL_PARITY_EVEN: parity = EVENPARITY; break;
    case SERIAL_PARITY_ODD: parity = ODDPARITY; break;
    case SERIAL_PARITY_MARK: parity = MARKPARITY; break;
    case SERIAL_PARITY_SPACE: parity = SPACEPARITY; break;
    default: return -9;
    }
    // configure byte size
    dcbSerialParams.ByteSize = bytesize;
    // configure stop bits
    dcbSerialParams.StopBits = stopBits;
    // configure parity
    dcbSerialParams.Parity = parity;

    // Write the parameters
    if (!SetCommState(hSerial, &dcbSerialParams)) return -5;

    // Set TimeOut

    // Set the Timeout parameters
    timeouts.ReadIntervalTimeout = 0;
    // No TimeOut
    timeouts.ReadTotalTimeoutConstant = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = MAXDWORD;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    // Write the parameters
    if (!SetCommTimeouts(hSerial, &timeouts)) return -6;

    // Opening successfull
    return 1;
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Structure with the device's options
    struct termios options;


    // Open device
    fd = open(Device, O_RDWR | O_NOCTTY | O_NDELAY);
    // If the device is not open, return -1
    if (fd == -1) return -2;
    // Open the device in nonblocking mode
    fcntl(fd, F_SETFL, FNDELAY);


    // Get the current options of the port
    tcgetattr(fd, &options);
    // Clear all the options
    bzero(&options, sizeof(options));

    // Prepare speed (Bauds)
    speed_t         Speed;
    switch (Bauds)
    {
    case 110:     Speed = B110; break;
    case 300:     Speed = B300; break;
    case 600:     Speed = B600; break;
    case 1200:     Speed = B1200; break;
    case 2400:     Speed = B2400; break;
    case 4800:     Speed = B4800; break;
    case 9600:     Speed = B9600; break;
    case 19200:    Speed = B19200; break;
    case 38400:    Speed = B38400; break;
    case 57600:    Speed = B57600; break;
    case 115200:   Speed = B115200; break;
#if defined (B230400)
    case 230400:   Speed = B230400; break;
#endif
#if defined (B460800)
    case 460800:   Speed = B460800; break;
#endif
#if defined (B500000)
    case 500000:   Speed = B500000; break;
#endif
#if defined (B576000)
    case 576000:   Speed = B576000; break;
#endif
#if defined (B921600)
    case 921600:   Speed = B921600; break;
#endif
#if defined (B1000000)
    case 1000000:   Speed = B1000000; break;
#endif
#if defined (B1152000)
    case 1152000:   Speed = B1152000; break;
#endif
#if defined (B1500000)
    case 1500000:   Speed = B1500000; break;
#endif
#if defined (B2000000)
    case 2000000:   Speed = B2000000; break;
#endif
#if defined (B2500000)
    case 2500000:   Speed = B2500000; break;
#endif
#if defined (B3000000)
    case 3000000:   Speed = B3000000; break;
#endif
#if defined (B3500000)
    case 3500000:   Speed = B3500000; break;
#endif
#if defined (B4000000)
    case 4000000:   Speed = B4000000; break;
#endif
    default: return -4;
    }
    int databits_flag = 0;
    switch (Databits) {
    case SERIAL_DATABITS_5: databits_flag = CS5; break;
    case SERIAL_DATABITS_6: databits_flag = CS6; break;
    case SERIAL_DATABITS_7: databits_flag = CS7; break;
    case SERIAL_DATABITS_8: databits_flag = CS8; break;
        //16 bits and everything else not supported
    default: return -7;
    }
    int stopbits_flag = 0;
    switch (Stopbits) {
    case SERIAL_STOPBITS_1: stopbits_flag = 0; break;
    case SERIAL_STOPBITS_2: stopbits_flag = CSTOPB; break;
        //1.5 stopbits and everything else not supported
    default: return -8;
    }
    int parity_flag = 0;
    switch (Parity) {
    case SERIAL_PARITY_NONE: parity_flag = 0; break;
    case SERIAL_PARITY_EVEN: parity_flag = PARENB; break;
    case SERIAL_PARITY_ODD: parity_flag = (PARENB | PARODD); break;
        //mark and space parity not supported
    default: return -9;
    }

    // Set the baud rate
    cfsetispeed(&options, Speed);
    cfsetospeed(&options, Speed);
    // Configure the device : data bits, stop bits, parity, no control flow
    // Ignore modem control lines (CLOCAL) and Enable receiver (CREAD)
    options.c_cflag |= (CLOCAL | CREAD | databits_flag | parity_flag | stopbits_flag);
    options.c_iflag |= (IGNPAR | IGNBRK);
    // Timer unused
    options.c_cc[VTIME] = 0;
    // At least on character before satisfy reading
    options.c_cc[VMIN] = 0;
    // Activate the settings
    tcsetattr(fd, TCSANOW, &options);
    // Success
    return (1);
#endif

}

bool serialib::isDeviceOpen()
{
#if defined (_WIN32) || defined( _WIN64)
    return hSerial != INVALID_HANDLE_VALUE;
#endif
#if defined (__linux__) || defined(__APPLE__)
    return fd >= 0;
#endif
}

/*!
     \brief Close the connection with the current device
*/
void serialib::closeDevice()
{
#if defined (_WIN32) || defined( _WIN64)
    CloseHandle(hSerial);
    hSerial = INVALID_HANDLE_VALUE;
#endif
#if defined (__linux__) || defined(__APPLE__)
    close(fd);
    fd = -1;
#endif
}




//___________________________________________
// ::: Read/Write operation on characters :::



/*!
     \brief Write a char on the current serial port
     \param Byte : char to send on the port (must be terminated by '\0')
     \return 1 success
     \return -1 error while writting data
  */
int serialib::writeChar(const char Byte)
{
#if defined (_WIN32) || defined( _WIN64)
    // Number of bytes written
    DWORD dwBytesWritten;
    // Write the char to the serial device
    // Return -1 if an error occured
    if (!WriteFile(hSerial, &Byte, 1, &dwBytesWritten, NULL)) return -1;
    // Write operation successfull
    return 1;
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Write the char
    if (write(fd, &Byte, 1) != 1) return -1;

    // Write operation successfull
    return 1;
#endif
}



//________________________________________
// ::: Read/Write operation on strings :::


/*!
     \brief     Write a string on the current serial port
     \param     receivedString : string to send on the port (must be terminated by '\0')
     \return     1 success
     \return    -1 error while writting data
  */
int serialib::writeString(const char* receivedString)
{
#if defined (_WIN32) || defined( _WIN64)
    // Number of bytes written
    DWORD dwBytesWritten;
    // Write the string
    if (!WriteFile(hSerial, receivedString, strlen(receivedString), &dwBytesWritten, NULL))
        // Error while writing, return -1
        return -1;
    // Write operation successfull
    return 1;
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Lenght of the string
    int Lenght = strlen(receivedString);
    // Write the string
    if (write(fd, receivedString, Lenght) != Lenght) return -1;
    // Write operation successfull
    return 1;
#endif
}

// _____________________________________
// ::: Read/Write operation on bytes :::



/*!
     \brief Write an array of data on the current serial port
     \param Buffer : array of bytes to send on the port
     \param NbBytes : number of byte to send
     \return 1 success
     \return -1 error while writting data
  */
int serialib::writeBytes(const void* Buffer, const unsigned int NbBytes)
{
#if defined (_WIN32) || defined( _WIN64)
    // Number of bytes written
    DWORD dwBytesWritten;
    // Write data
    if (!WriteFile(hSerial, Buffer, NbBytes, &dwBytesWritten, NULL))
        // Error while writing, return -1
        return -1;
    // Write operation successfull
    return 1;
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Write data
    if (write(fd, Buffer, NbBytes) != (ssize_t)NbBytes) return -1;
    // Write operation successfull
    return 1;
#endif
}



/*!
     \brief Wait for a byte from the serial device and return the data read
     \param pByte : data read on the serial device
     \param timeOut_ms : delay of timeout before giving up the reading
            If set to zero, timeout is disable (Optional)
     \return 1 success
     \return 0 Timeout reached
     \return -1 error while setting the Timeout
     \return -2 error while reading the byte
  */
int serialib::readChar(char* pByte, unsigned int timeOut_ms)
{
#if defined (_WIN32) || defined(_WIN64)
    // Number of bytes read
    DWORD dwBytesRead = 0;

    // Set the TimeOut
    timeouts.ReadTotalTimeoutConstant = timeOut_ms;

    // Write the parameters, return -1 if an error occured
    if (!SetCommTimeouts(hSerial, &timeouts)) return -1;

    // Read the byte, return -2 if an error occured
    if (!ReadFile(hSerial, pByte, 1, &dwBytesRead, NULL)) return -2;

    // Return 0 if the timeout is reached
    if (dwBytesRead == 0) return 0;

    // The byte is read
    return 1;
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Timer used for timeout
    timeOut         timer;
    // Initialise the timer
    timer.initTimer();
    // While Timeout is not reached
    while (timer.elapsedTime_ms() < timeOut_ms || timeOut_ms == 0)
    {
        // Try to read a byte on the device
        switch (read(fd, pByte, 1)) {
        case 1: return 1; // Read successfull
        case -1: return -2; // Error while reading
        }
    }
    return 0;
#endif
}



/*!
     \brief Read a string from the serial device (without TimeOut)
     \param receivedString : string read on the serial device
     \param FinalChar : final char of the string
     \param MaxNbBytes : maximum allowed number of bytes read
     \return >0 success, return the number of bytes read
     \return -1 error while setting the Timeout
     \return -2 error while reading the byte
     \return -3 MaxNbBytes is reached
  */
int serialib::readStringNoTimeOut(char* receivedString, char finalChar, unsigned int maxNbBytes)
{
    // Number of characters read
    unsigned int    NbBytes = 0;
    // Returned value from Read
    char            charRead;

    // While the buffer is not full
    while (NbBytes < maxNbBytes)
    {
        // Read a character with the restant time
        charRead = readChar(&receivedString[NbBytes]);

        // Check a character has been read
        if (charRead == 1)
        {
            // Check if this is the final char
            if (receivedString[NbBytes] == finalChar)
            {
                // This is the final char, add zero (end of string)
                receivedString[++NbBytes] = 0;
                // Return the number of bytes read
                return NbBytes;
            }

            // The character is not the final char, increase the number of bytes read
            NbBytes++;
        }

        // An error occured while reading, return the error number
        if (charRead < 0) return charRead;
    }
    // Buffer is full : return -3
    return -3;
}


/*!
     \brief Read a string from the serial device (with timeout)
     \param receivedString : string read on the serial device
     \param finalChar : final char of the string
     \param maxNbBytes : maximum allowed number of characters read
     \param timeOut_ms : delay of timeout before giving up the reading (optional)
     \return  >0 success, return the number of bytes read (including the null character)
     \return  0 timeout is reached
     \return -1 error while setting the Timeout
     \return -2 error while reading the character
     \return -3 MaxNbBytes is reached
  */
int serialib::readString(char* receivedString, char finalChar, unsigned int maxNbBytes, unsigned int timeOut_ms)
{
    // Check if timeout is requested
    if (timeOut_ms == 0) return readStringNoTimeOut(receivedString, finalChar, maxNbBytes);

    // Number of bytes read
    unsigned int    nbBytes = 0;
    // Character read on serial device
    char            charRead;
    // Timer used for timeout
    timeOut         timer;
    long int        timeOutParam;

    // Initialize the timer (for timeout)
    timer.initTimer();

    // While the buffer is not full
    while (nbBytes < maxNbBytes)
    {
        // Compute the TimeOut for the next call of ReadChar
        timeOutParam = timeOut_ms - timer.elapsedTime_ms();

        // If there is time remaining
        if (timeOutParam > 0)
        {
            // Wait for a byte on the serial link with the remaining time as timeout
            charRead = readChar(&receivedString[nbBytes], timeOutParam);

            // If a byte has been received
            if (charRead == 1)
            {
                // Check if the character received is the final one
                if (receivedString[nbBytes] == finalChar)
                {
                    // Final character: add the end character 0
                    receivedString[++nbBytes] = 0;
                    // Return the number of bytes read
                    return nbBytes;
                }
                // This is not the final character, just increase the number of bytes read
                nbBytes++;
            }
            // Check if an error occured during reading char
            // If an error occurend, return the error number
            if (charRead < 0) return charRead;
        }
        // Check if timeout is reached
        if (timer.elapsedTime_ms() > timeOut_ms)
        {
            // Add the end caracter
            receivedString[nbBytes] = 0;
            // Return 0 (timeout reached)
            return 0;
        }
    }

    // Buffer is full : return -3
    return -3;
}


/*!
     \brief Read an array of bytes from the serial device (with timeout)
     \param buffer : array of bytes read from the serial device
     \param maxNbBytes : maximum allowed number of bytes read
     \param timeOut_ms : delay of timeout before giving up the reading
     \param sleepDuration_us : delay of CPU relaxing in microseconds (Linux only)
            In the reading loop, a sleep can be performed after each reading
            This allows CPU to perform other tasks
     \return >=0 return the number of bytes read before timeout or
                requested data is completed
     \return -1 error while setting the Timeout
     \return -2 error while reading the byte
  */
int serialib::readBytes(void* buffer, unsigned int maxNbBytes, unsigned int timeOut_ms, unsigned int sleepDuration_us)
{
#if defined (_WIN32) || defined(_WIN64)
    // Avoid warning while compiling
    UNUSED(sleepDuration_us);

    // Number of bytes read
    DWORD dwBytesRead = 0;

    // Set the TimeOut
    timeouts.ReadTotalTimeoutConstant = (DWORD)timeOut_ms;

    // Write the parameters and return -1 if an error occrured
    if (!SetCommTimeouts(hSerial, &timeouts)) return -1;


    // Read the bytes from the serial device, return -2 if an error occured
    if (!ReadFile(hSerial, buffer, (DWORD)maxNbBytes, &dwBytesRead, NULL))  return -2;

    // Return the byte read
    return dwBytesRead;
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Timer used for timeout
    timeOut          timer;
    // Initialise the timer
    timer.initTimer();
    unsigned int     NbByteRead = 0;
    // While Timeout is not reached
    while (timer.elapsedTime_ms() < timeOut_ms || timeOut_ms == 0)
    {
        // Compute the position of the current byte
        unsigned char* Ptr = (unsigned char*)buffer + NbByteRead;
        // Try to read a byte on the device
        int Ret = read(fd, (void*)Ptr, maxNbBytes - NbByteRead);
        // Error while reading
        if (Ret == -1) return -2;

        // One or several byte(s) has been read on the device
        if (Ret > 0)
        {
            // Increase the number of read bytes
            NbByteRead += Ret;
            // Success : bytes has been read
            if (NbByteRead >= maxNbBytes)
                return NbByteRead;
        }
        // Suspend the loop to avoid charging the CPU
        usleep(sleepDuration_us);
    }
    // Timeout reached, return the number of bytes read
    return NbByteRead;
#endif
}




// _________________________
// ::: Special operation :::



/*!
    \brief Empty receiver buffer
    \return If the function succeeds, the return value is nonzero.
            If the function fails, the return value is zero.
*/
char serialib::flushReceiver()
{
#if defined (_WIN32) || defined(_WIN64)
    // Purge receiver
    return PurgeComm(hSerial, PURGE_RXCLEAR);
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Purge receiver
    tcflush(fd, TCIFLUSH);
    return true;
#endif
}



/*!
    \brief  Return the number of bytes in the received buffer (UNIX only)
    \return The number of bytes received by the serial provider but not yet read.
*/
int serialib::available()
{
#if defined (_WIN32) || defined(_WIN64)
    // Device errors
    DWORD commErrors;
    // Device status
    COMSTAT commStatus;
    // Read status
    ClearCommError(hSerial, &commErrors, &commStatus);
    // Return the number of pending bytes
    return commStatus.cbInQue;
#endif
#if defined (__linux__) || defined(__APPLE__)
    int nBytes = 0;
    // Return number of pending bytes in the receiver
    ioctl(fd, FIONREAD, &nBytes);
    return nBytes;
#endif

}



// __________________
// ::: I/O Access :::

/*!
    \brief      Set or unset the bit DTR (pin 4)
                DTR stands for Data Terminal Ready
                Convenience method :This method calls setDTR and clearDTR
    \param      status = true set DTR
                status = false unset DTR
    \return     If the function fails, the return value is false
                If the function succeeds, the return value is true.
*/
bool serialib::DTR(bool status)
{
    if (status)
        // Set DTR
        return this->setDTR();
    else
        // Unset DTR
        return this->clearDTR();
}


/*!
    \brief      Set the bit DTR (pin 4)
                DTR stands for Data Terminal Ready
    \return     If the function fails, the return value is false
                If the function succeeds, the return value is true.
*/
bool serialib::setDTR()
{
#if defined (_WIN32) || defined(_WIN64)
    // Set DTR
    currentStateDTR = true;
    return EscapeCommFunction(hSerial, SETDTR);
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Set DTR
    int status_DTR = 0;
    ioctl(fd, TIOCMGET, &status_DTR);
    status_DTR |= TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status_DTR);
    return true;
#endif
}

/*!
    \brief      Clear the bit DTR (pin 4)
                DTR stands for Data Terminal Ready
    \return     If the function fails, the return value is false
                If the function succeeds, the return value is true.
*/
bool serialib::clearDTR()
{
#if defined (_WIN32) || defined(_WIN64)
    // Clear DTR
    currentStateDTR = true;
    return EscapeCommFunction(hSerial, CLRDTR);
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Clear DTR
    int status_DTR = 0;
    ioctl(fd, TIOCMGET, &status_DTR);
    status_DTR &= ~TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status_DTR);
    return true;
#endif
}



/*!
    \brief      Set or unset the bit RTS (pin 7)
                RTS stands for Data Termina Ready
                Convenience method :This method calls setDTR and clearDTR
    \param      status = true set DTR
                status = false unset DTR
    \return     false if the function fails
    \return     true if the function succeeds
*/
bool serialib::RTS(bool status)
{
    if (status)
        // Set RTS
        return this->setRTS();
    else
        // Unset RTS
        return this->clearRTS();
}


/*!
    \brief      Set the bit RTS (pin 7)
                RTS stands for Data Terminal Ready
    \return     If the function fails, the return value is false
                If the function succeeds, the return value is true.
*/
bool serialib::setRTS()
{
#if defined (_WIN32) || defined(_WIN64)
    // Set RTS
    currentStateRTS = false;
    return EscapeCommFunction(hSerial, SETRTS);
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Set RTS
    int status_RTS = 0;
    ioctl(fd, TIOCMGET, &status_RTS);
    status_RTS |= TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status_RTS);
    return true;
#endif
}



/*!
    \brief      Clear the bit RTS (pin 7)
                RTS stands for Data Terminal Ready
    \return     If the function fails, the return value is false
                If the function succeeds, the return value is true.
*/
bool serialib::clearRTS()
{
#if defined (_WIN32) || defined(_WIN64)
    // Clear RTS
    currentStateRTS = false;
    return EscapeCommFunction(hSerial, CLRRTS);
#endif
#if defined (__linux__) || defined(__APPLE__)
    // Clear RTS
    int status_RTS = 0;
    ioctl(fd, TIOCMGET, &status_RTS);
    status_RTS &= ~TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status_RTS);
    return true;
#endif
}




/*!
    \brief      Get the CTS's status (pin 8)
                CTS stands for Clear To Send
    \return     Return true if CTS is set otherwise false
  */
bool serialib::isCTS()
{
#if defined (_WIN32) || defined(_WIN64)
    DWORD modemStat;
    GetCommModemStatus(hSerial, &modemStat);
    return modemStat & MS_CTS_ON;
#endif
#if defined (__linux__) || defined(__APPLE__)
    int status = 0;
    //Get the current status of the CTS bit
    ioctl(fd, TIOCMGET, &status);
    return status & TIOCM_CTS;
#endif
}



/*!
    \brief      Get the DSR's status (pin 6)
                DSR stands for Data Set Ready
    \return     Return true if DTR is set otherwise false
  */
bool serialib::isDSR()
{
#if defined (_WIN32) || defined(_WIN64)
    DWORD modemStat;
    GetCommModemStatus(hSerial, &modemStat);
    return modemStat & MS_DSR_ON;
#endif
#if defined (__linux__) || defined(__APPLE__)
    int status = 0;
    //Get the current status of the DSR bit
    ioctl(fd, TIOCMGET, &status);
    return status & TIOCM_DSR;
#endif
}






/*!
    \brief      Get the DCD's status (pin 1)
                CDC stands for Data Carrier Detect
    \return     true if DCD is set
    \return     false otherwise
  */
bool serialib::isDCD()
{
#if defined (_WIN32) || defined(_WIN64)
    DWORD modemStat;
    GetCommModemStatus(hSerial, &modemStat);
    return modemStat & MS_RLSD_ON;
#endif
#if defined (__linux__) || defined(__APPLE__)
    int status = 0;
    //Get the current status of the DCD bit
    ioctl(fd, TIOCMGET, &status);
    return status & TIOCM_CAR;
#endif
}


/*!
    \brief      Get the RING's status (pin 9)
                Ring Indicator
    \return     Return true if RING is set otherwise false
  */
bool serialib::isRI()
{
#if defined (_WIN32) || defined(_WIN64)
    DWORD modemStat;
    GetCommModemStatus(hSerial, &modemStat);
    return modemStat & MS_RING_ON;
#endif
#if defined (__linux__) || defined(__APPLE__)
    int status = 0;
    //Get the current status of the RING bit
    ioctl(fd, TIOCMGET, &status);
    return status & TIOCM_RNG;
#endif
}


/*!
    \brief      Get the DTR's status (pin 4)
                DTR stands for Data Terminal Ready
                May behave abnormally on Windows
    \return     Return true if CTS is set otherwise false
  */
bool serialib::isDTR()
{
#if defined (_WIN32) || defined( _WIN64)
    return currentStateDTR;
#endif
#if defined (__linux__) || defined(__APPLE__)
    int status = 0;
    //Get the current status of the DTR bit
    ioctl(fd, TIOCMGET, &status);
    return status & TIOCM_DTR;
#endif
}



/*!
    \brief      Get the RTS's status (pin 7)
                RTS stands for Request To Send
                May behave abnormally on Windows
    \return     Return true if RTS is set otherwise false
  */
bool serialib::isRTS()
{
#if defined (_WIN32) || defined(_WIN64)
    return currentStateRTS;
#endif
#if defined (__linux__) || defined(__APPLE__)
    int status = 0;
    //Get the current status of the CTS bit
    ioctl(fd, TIOCMGET, &status);
    return status & TIOCM_RTS;
#endif
}






// ******************************************
//  Class timeOut
// ******************************************


/*!
    \brief      Constructor of the class timeOut.
*/
// Constructor
timeOut::timeOut()
{}


/*!
    \brief      Initialise the timer. It writes the current time of the day in the structure PreviousTime.
*/
//Initialize the timer
void timeOut::initTimer()
{
#if defined (NO_POSIX_TIME)
    LARGE_INTEGER tmp;
    QueryPerformanceFrequency(&tmp);
    counterFrequency = tmp.QuadPart;
    // Used to store the previous time (for computing timeout)
    QueryPerformanceCounter(&tmp);
    previousTime = tmp.QuadPart;
#else
    gettimeofday(&previousTime, NULL);
#endif
}

/*!
    \brief      Returns the time elapsed since initialization.  It write the current time of the day in the structure CurrentTime.
                Then it returns the difference between CurrentTime and PreviousTime.
    \return     The number of microseconds elapsed since the functions InitTimer was called.
  */
  //Return the elapsed time since initialization
unsigned long int timeOut::elapsedTime_ms()
{
#if defined (NO_POSIX_TIME)
    // Current time
    LARGE_INTEGER CurrentTime;
    // Number of ticks since last call
    int sec;

    // Get current time
    QueryPerformanceCounter(&CurrentTime);

    // Compute the number of ticks elapsed since last call
    sec = CurrentTime.QuadPart - previousTime;

    // Return the elapsed time in milliseconds
    return sec / (counterFrequency / 1000);
#else
    // Current time
    struct timeval CurrentTime;
    // Number of seconds and microseconds since last call
    int sec, usec;

    // Get current time
    gettimeofday(&CurrentTime, NULL);

    // Compute the number of seconds and microseconds elapsed since last call
    sec = CurrentTime.tv_sec - previousTime.tv_sec;
    usec = CurrentTime.tv_usec - previousTime.tv_usec;

    // If the previous usec is higher than the current one
    if (usec < 0)
    {
        // Recompute the microseonds and substract one second
        usec = 1000000 - previousTime.tv_usec + CurrentTime.tv_usec;
        sec--;
    }

    // Return the elapsed time in milliseconds
    return sec * 1000 + usec / 1000;
#endif
}


////	Serial.cpp - Implementation of the CSerial class
////
////	Copyright (C) 1999-2003 Ramon de Klein (Ramon.de.Klein@ict.nl)
////
//// This library is free software; you can redistribute it and/or
//// modify it under the terms of the GNU Lesser General Public
//// License as published by the Free Software Foundation; either
//// version 2.1 of the License, or (at your option) any later version.
//// 
//// This library is distributed in the hope that it will be useful,
//// but WITHOUT ANY WARRANTY; without even the implied warranty of
//// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//// Lesser General Public License for more details.
//// 
//// You should have received a copy of the GNU Lesser General Public
//// License along with this library; if not, write to the Free Software
//// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//
////////////////////////////////////////////////////////////////////////
//// Include the standard header files
//
////#define STRICT
//#include "Serial.h"
//#include <stdio.h>
//#include <tchar.h>
//#include <windows.h>
////#include <crtdbg.h>
////#include <tchar.h>
////#include <windows.h>
//
//
//
////////////////////////////////////////////////////////////////////////
//// Disable warning C4127: conditional expression is constant, which
//// is generated when using the _RPTF and //_ASSERTE macros.
//
//#pragma warning(disable: 4127)
//
//
////////////////////////////////////////////////////////////////////////
//// Enable debug memory manager
//
//#ifdef _DEBUG
//
//#ifdef THIS_FILE
//#undef THIS_FILE
//#endif
//
//static const char THIS_FILE[] = __FILE__;
//#define new DEBUG_NEW
//
//#endif
//
//
////////////////////////////////////////////////////////////////////////
//// Helper methods
//
//inline void CSerial::CheckRequirements (LPOVERLAPPED lpOverlapped, DWORD dwTimeout) const
//{
//#ifdef SERIAL_NO_OVERLAPPED
//
//	// Check if an overlapped structure has been specified
//	if (lpOverlapped || (dwTimeout != INFINITE))
//	{
//		// Quit application
//		::MessageBox(0,_T("Overlapped I/O and time-outs are not supported, when overlapped I/O is disabled."),_T("Serial library"), MB_ICONERROR | MB_TASKMODAL);
//		::DebugBreak();
//		::ExitProcess(0xFFFFFFF);
//	}
//
//#endif
//
//#ifdef SERIAL_NO_CANCELIO
//
//	// Check if 0 or INFINITE time-out has been specified, because
//	// the communication I/O cannot be cancelled.
//	if ((dwTimeout != 0) && (dwTimeout != INFINITE))
//	{
//		// Quit application
//		::MessageBox(0,_T("Timeouts are not supported, when SERIAL_NO_CANCELIO is defined"),_T("Serial library"), MB_ICONERROR | MB_TASKMODAL);
//		::DebugBreak();
//		::ExitProcess(0xFFFFFFF);
//	}
//
//#endif	// SERIAL_NO_CANCELIO
//
//	// Avoid warnings
//	(void) dwTimeout;
//	(void) lpOverlapped;
//}
//
//inline BOOL CSerial::CancelCommIo (void)
//{
//#ifdef SERIAL_NO_CANCELIO
//	// CancelIo shouldn't have been called at this point
//	::DebugBreak();
//	return FALSE;
//#else
//
//	// Cancel the I/O request
//	return ::CancelIo(m_hFile);
//
//#endif	// SERIAL_NO_CANCELIO
//}
//
//
////////////////////////////////////////////////////////////////////////
//// Code
//
//CSerial::CSerial ()
//	: m_lLastError(ERROR_SUCCESS)
//	, m_hFile(0)
//	, m_eEvent(EEventNone)
//	, m_dwEventMask(0)
//#ifndef SERIAL_NO_OVERLAPPED
//	, m_hevtOverlapped(0)
//#endif
//{
//}
//
//CSerial::~CSerial ()
//{
//	// If the device is already closed,
//	// then we don't need to do anything.
//	if (m_hFile)
//	{
//		// Display a warning
////		//_RPTF0(_CRT_WARN,"CSerial::~CSerial - Serial port not closed\n");
//
//		// Close implicitly
//		Close();
//	}
//}
//
//CSerial::EPort CSerial::CheckPort (LPCTSTR lpszDevice)
//{
//	// Try to open the device
//	HANDLE hFile = ::CreateFile(lpszDevice, 
//						   GENERIC_READ|GENERIC_WRITE, 
//						   0, 
//						   0, 
//						   OPEN_EXISTING, 
//						   0,
//						   0);
//
//	// Check if we could open the device
//	if (hFile == INVALID_HANDLE_VALUE)
//	{
//		// Display error
//		switch (::GetLastError())
//		{
//		case ERROR_FILE_NOT_FOUND:
//			// The specified COM-port does not exist
//			return EPortNotAvailable;
//
//		case ERROR_ACCESS_DENIED:
//			// The specified COM-port is in use
//			return EPortInUse;
//
//		default:
//			// Something else is wrong
//			return EPortUnknownError;
//		}
//	}
//
//	// Close handle
//	::CloseHandle(hFile);
//
//	// Port is available
//	return EPortAvailable;
//}
//
//LONG CSerial::Open (LPCTSTR lpszDevice, DWORD dwInQueue, DWORD dwOutQueue, bool fOverlapped)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the port isn't already opened
//	if (m_hFile)
//	{
//		m_lLastError = ERROR_ALREADY_INITIALIZED;
////		//_RPTF0(_CRT_WARN,"CSerial::Open - Port already opened\n");
//		return m_lLastError;
//	}
//
//	// Open the device
//	m_hFile = ::CreateFile(lpszDevice,
//						   GENERIC_READ|GENERIC_WRITE,
//						   0,
//						   0,
//						   OPEN_EXISTING,
//						   fOverlapped?FILE_FLAG_OVERLAPPED:0,
//						   0);
//	if (m_hFile == INVALID_HANDLE_VALUE)
//	{
//		// Reset file handle
//		m_hFile = 0;
//
//		// Display error
//		m_lLastError = ::GetLastError();
////		//_RPTF0(_CRT_WARN, "CSerial::Open - Unable to open port\n");
//		return m_lLastError;
//	}
//
//#ifndef SERIAL_NO_OVERLAPPED
//	// We cannot have an event handle yet
////	//_ASSERTE(m_hevtOverlapped == 0);
//
//	// Create the event handle for internal overlapped operations (manual reset)
//	if (fOverlapped)
//	{
//		m_hevtOverlapped = ::CreateEvent(0,true,false,0);
//		if (m_hevtOverlapped == 0)
//		{
//			// Obtain the error information
//			m_lLastError = ::GetLastError();
////			//_RPTF0(_CRT_WARN,"CSerial::Open - Unable to create event\n");
//
//			// Close the port
//			::CloseHandle(m_hFile);
//			m_hFile = 0;
//
//			// Return the error
//			return m_lLastError;
//		}
//	}
//#else
//	
//	// Overlapped flag shouldn't be specified
//	//_ASSERTE(!fOverlapped);
//
//#endif
//
//	// Setup the COM-port
//	if (dwInQueue || dwOutQueue)
//	{
//		// Make sure the queue-sizes are reasonable sized. Win9X systems crash
//		// if the input queue-size is zero. Both queues need to be at least
//		// 16 bytes large.
////		//_ASSERTE(dwInQueue >= 16);
////		//_ASSERTE(dwOutQueue >= 16);
//
//		if (!::SetupComm(m_hFile,dwInQueue,dwOutQueue))
//		{
//			// Display a warning
//			long lLastError = ::GetLastError();
////			//_RPTF0(_CRT_WARN,"CSerial::Open - Unable to setup the COM-port\n");
//
//			// Close the port
//			Close();
//
//			// Save last error from SetupComm
//			m_lLastError = lLastError;
//			return m_lLastError;	
//		}
//	}
//
//	// Setup the default communication mask
//	SetMask();
//
//	// Non-blocking reads is default
//	SetupReadTimeouts(EReadTimeoutNonblocking);
//
//	// Setup the device for default settings
// 	COMMCONFIG commConfig = {0};
//	DWORD dwSize = sizeof(commConfig);
//	commConfig.dwSize = dwSize;
//	if (::GetDefaultCommConfig(lpszDevice,&commConfig,&dwSize))
//	{
//		// Set the default communication configuration
//		if (!::SetCommConfig(m_hFile,&commConfig,dwSize))
//		{
//			// Display a warning
////			//_RPTF0(_CRT_WARN,"CSerial::Open - Unable to set default communication configuration.\n");
//		}
//	}
//	else
//	{
//		// Display a warning
////		//_RPTF0(_CRT_WARN,"CSerial::Open - Unable to obtain default communication configuration.\n");
//	}
//
//	// Return successful
//	return m_lLastError;
//}
//
////FIXME - A lot of comments
//LONG CSerial::Close (void)
//{
//	return m_lLastError;
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// If the device is already closed,
//	// then we don't need to do anything.
//	if (m_hFile == 0)
//	{
//		// Display a warning
////		////_RPTF0(_CRT_WARN,"CSerial::Close - Method called when device is not open\n");
//		return m_lLastError;
//	}
//
//#ifndef SERIAL_NO_OVERLAPPED
//	// Free event handle
//	if (m_hevtOverlapped)
//	{
//		::CloseHandle(m_hevtOverlapped);
//		m_hevtOverlapped = 0;
//	}
//#endif
//
//	// Close COM port
//	::CloseHandle(m_hFile);
//	m_hFile = 0;
//
//	// Return successful
//	return m_lLastError;
//}
//
//LONG CSerial::Setup (EBaudrate eBaudrate, EDataBits eDataBits, EParity eParity, EStopBits eStopBits)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		////_RPTF0(_CRT_WARN,"CSerial::Setup - Device is not opened\n");
//		return m_lLastError;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::	GetLastError();
//
//		// Display a warning
//		////_RPTF0(_CRT_WARN,"CSerial::Setup - Unable to obtain DCB information\n");
//		return m_lLastError;
//	}
//
//	// Set the new data
//	dcb.BaudRate = DWORD(eBaudrate);
//	dcb.ByteSize = BYTE(eDataBits);
//	dcb.Parity   = BYTE(eParity);
//	dcb.StopBits = BYTE(eStopBits);
//
//	// Determine if parity is used
//	dcb.fParity  = (eParity != EParNone);
//
//	// Set the new DCB structure
//	if (!::SetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		////_RPTF0(_CRT_WARN,"CSerial::Setup - Unable to set DCB information\n");
//		return m_lLastError;
//	}
//
//	// Return successful
//	return m_lLastError;
//}
//
//LONG CSerial::SetEventChar (BYTE bEventChar, bool fAdjustMask)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		////_RPTF0(_CRT_WARN,"CSerial::SetEventChar - Device is not opened\n");
//		return m_lLastError;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		////_RPTF0(_CRT_WARN,"CSerial::SetEventChar - Unable to obtain DCB information\n");
//		return m_lLastError;
//	}
//
//	// Set the new event character
//	dcb.EvtChar = char(bEventChar);
//
//	// Adjust the event mask, to make sure the event will be received
//	if (fAdjustMask)
//	{
//		// Enable 'receive event character' event.  Note that this
//		// will generate an EEventNone if there is an asynchronous
//		// WaitCommEvent pending.
//		SetMask(GetEventMask() | EEventRcvEv);
//	}
//
//	// Set the new DCB structure
//	if (!::SetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		////_RPTF0(_CRT_WARN,"CSerial::SetEventChar - Unable to set DCB information\n");
//		return m_lLastError;
//	}
//
//	// Return successful
//	return m_lLastError;
//}
//
//LONG CSerial::SetMask (DWORD dwEventMask)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		////_RPTF0(_CRT_WARN,"CSerial::SetMask - Device is not opened\n");
//		return m_lLastError;
//	}
//
//	// Set the new mask. Note that this will generate an EEventNone
//	// if there is an asynchronous WaitCommEvent pending.
//	if (!::SetCommMask(m_hFile,dwEventMask))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		////_RPTF0(_CRT_WARN,"CSerial::SetMask - Unable to set event mask\n");
//		return m_lLastError;
//	}
//
//	// Save event mask and return successful
//	m_dwEventMask = dwEventMask;
//	return m_lLastError;
//}
//
//LONG CSerial::WaitEvent (LPOVERLAPPED lpOverlapped, DWORD dwTimeout)
//{
//	// Check if time-outs are supported
//	CheckRequirements(lpOverlapped,dwTimeout);
//
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		////_RPTF0(_CRT_WARN,"CSerial::WaitEvent - Device is not opened\n");
//		return m_lLastError;
//	}
//
//#ifndef SERIAL_NO_OVERLAPPED
//
//	// Check if an overlapped structure has been specified
//	if (!m_hevtOverlapped && (lpOverlapped || (dwTimeout != INFINITE)))
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_FUNCTION;
//
//		// Issue an error and quit
//		////_RPTF0(_CRT_WARN,"CSerial::WaitEvent - Overlapped I/O is disabled, specified parameters are illegal.\n");
//		return m_lLastError;
//	}
//
//	// Wait for the event to happen
//	OVERLAPPED ovInternal;
//	if (!lpOverlapped && m_hevtOverlapped)
//	{
//		// Setup our own overlapped structure
//		memset(&ovInternal,0,sizeof(ovInternal));
//		ovInternal.hEvent = m_hevtOverlapped;
//
//		// Use our internal overlapped structure
//		lpOverlapped = &ovInternal;
//	}
//
//	// Make sure the overlapped structure isn't busy
//	////_ASSERTE(!m_hevtOverlapped || HasOverlappedIoCompleted(lpOverlapped));
//
//	// Wait for the COM event
//	if (!::WaitCommEvent(m_hFile,LPDWORD(&m_eEvent),lpOverlapped))
//	{
//		// Set the internal error code
//		long lLastError = ::GetLastError();
//
//		// Overlapped operation in progress is not an actual error
//		if (lLastError != ERROR_IO_PENDING)
//		{
//			// Save the error
//			m_lLastError = lLastError;
//
//			// Issue an error and quit
//			////_RPTF0(_CRT_WARN,"CSerial::WaitEvent - Unable to wait for COM event\n");
//			return m_lLastError;
//		}
//
//		// We need to block if the client didn't specify an overlapped structure
//		if (lpOverlapped == &ovInternal)
//		{
//			// Wait for the overlapped operation to complete
//			switch (::WaitForSingleObject(lpOverlapped->hEvent,dwTimeout))
//			{
//			case WAIT_OBJECT_0:
//				// The overlapped operation has completed
//				break;
//
//			case WAIT_TIMEOUT:
//				// Cancel the I/O operation
//				CancelCommIo();
//
//				// The operation timed out. Set the internal error code and quit
//				m_lLastError = ERROR_TIMEOUT;
//				return m_lLastError;
//
//			default:
//				// Set the internal error code
//				m_lLastError = ::GetLastError();
//
//				// Issue an error and quit
//				////_RPTF0(_CRT_WARN,"CSerial::WaitEvent - Unable to wait until COM event has arrived\n");
//				return m_lLastError;
//			}
//		}
//	}
//	else
//	{
//		// The operation completed immediatly. Just to be sure
//		// we'll set the overlapped structure's event handle.
//		if (lpOverlapped)
//			::SetEvent(lpOverlapped->hEvent);
//	}
//#else
//
//	// Wait for the COM event
//	if (!::WaitCommEvent(m_hFile,LPDWORD(&m_eEvent),0))
//	{
//		// Set the internal error code
//		m_lLastError = ::GetLastError();
//
//		// Issue an error and quit
//		////_RPTF0(_CRT_WARN,"CSerial::WaitEvent - Unable to wait for COM event\n");
//		return m_lLastError;
//	}
//
//#endif
//
//	// Return successfully
//	return m_lLastError;
//}
//
//
//LONG CSerial::SetupHandshaking (EHandshake eHandshake)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		////_RPTF0(_CRT_WARN,"CSerial::SetupHandshaking - Device is not opened\n");
//		return m_lLastError;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		////_RPTF0(_CRT_WARN,"CSerial::SetupHandshaking - Unable to obtain DCB information\n");
//		return m_lLastError;
//	}
//
//	// Set the handshaking flags
//	switch (eHandshake)
//	{
//	case EHandshakeOff:
//		dcb.fOutxCtsFlow = false;					// Disable CTS monitoring
//		dcb.fOutxDsrFlow = false;					// Disable DSR monitoring
//		dcb.fDtrControl = DTR_CONTROL_DISABLE;		// Disable DTR monitoring
//		dcb.fOutX = false;							// Disable XON/XOFF for transmission
//		dcb.fInX = false;							// Disable XON/XOFF for receiving
//		dcb.fRtsControl = RTS_CONTROL_DISABLE;		// Disable RTS (Ready To Send)
//		break;
//
//	case EHandshakeHardware:
//		dcb.fOutxCtsFlow = true;					// Enable CTS monitoring
//		dcb.fOutxDsrFlow = true;					// Enable DSR monitoring
//		dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;	// Enable DTR handshaking
//		dcb.fOutX = false;							// Disable XON/XOFF for transmission
//		dcb.fInX = false;							// Disable XON/XOFF for receiving
//		dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;	// Enable RTS handshaking
//		break;
//
//	case EHandshakeSoftware:
//		dcb.fOutxCtsFlow = false;					// Disable CTS (Clear To Send)
//		dcb.fOutxDsrFlow = false;					// Disable DSR (Data Set Ready)
//		dcb.fDtrControl = DTR_CONTROL_DISABLE;		// Disable DTR (Data Terminal Ready)
//		dcb.fOutX = true;							// Enable XON/XOFF for transmission
//		dcb.fInX = true;							// Enable XON/XOFF for receiving
//		dcb.fRtsControl = RTS_CONTROL_DISABLE;		// Disable RTS (Ready To Send)
//		break;
//
//	default:
//		// This shouldn't be possible
//		//_ASSERTE(false);
//		m_lLastError = E_INVALIDARG;
//		return m_lLastError;
//	}
//
//	// Set the new DCB structure
//	if (!::SetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		////_RPTF0(_CRT_WARN,"CSerial::SetupHandshaking - Unable to set DCB information\n");
//		return m_lLastError;
//	}
//
//	// Return successful
//	return m_lLastError;
//}
//
//LONG CSerial::SetupReadTimeouts (EReadTimeout eReadTimeout)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::SetupReadTimeouts - Device is not opened\n");
//		return m_lLastError;
//	}
//
//	// Determine the time-outs
//	COMMTIMEOUTS cto;
//	if (!::GetCommTimeouts(m_hFile,&cto))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::SetupReadTimeouts - Unable to obtain timeout information\n");
//		return m_lLastError;
//	}
//
//	// Set the new timeouts
//	switch (eReadTimeout)
//	{
//	case EReadTimeoutBlocking:
//		cto.ReadIntervalTimeout = 0;
//		cto.ReadTotalTimeoutConstant = 0;
//		cto.ReadTotalTimeoutMultiplier = 0;
//		break;
//	case EReadTimeoutNonblocking:
//		cto.ReadIntervalTimeout = MAXDWORD;
//		cto.ReadTotalTimeoutConstant = 0;
//		cto.ReadTotalTimeoutMultiplier = 0;
//		break;
//	default:
//		// This shouldn't be possible
//		//_ASSERTE(false);
//		m_lLastError = E_INVALIDARG;
//		return m_lLastError;
//	}
//
//	// Set the new DCB structure
//	if (!::SetCommTimeouts(m_hFile,&cto))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::SetupReadTimeouts - Unable to set timeout information\n");
//		return m_lLastError;
//	}
//
//	// Return successful
//	return m_lLastError;
//}
//
//CSerial::EBaudrate CSerial::GetBaudrate (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetBaudrate - Device is not opened\n");
//		return EBaudUnknown;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetBaudrate - Unable to obtain DCB information\n");
//		return EBaudUnknown;
//	}
//
//	// Return the appropriate baudrate
//	return EBaudrate(dcb.BaudRate);
//}
//
//CSerial::EDataBits CSerial::GetDataBits (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetDataBits - Device is not opened\n");
//		return EDataUnknown;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetDataBits - Unable to obtain DCB information\n");
//		return EDataUnknown;
//	}
//
//	// Return the appropriate bytesize
//	return EDataBits(dcb.ByteSize);
//}
//
//CSerial::EParity CSerial::GetParity (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetParity - Device is not opened\n");
//		return EParUnknown;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetParity - Unable to obtain DCB information\n");
//		return EParUnknown;
//	}
//
//	// Check if parity is used
//	if (!dcb.fParity)
//	{
//		// No parity
//		return EParNone;
//	}
//
//	// Return the appropriate parity setting
//	return EParity(dcb.Parity);
//}
//
//CSerial::EStopBits CSerial::GetStopBits (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetStopBits - Device is not opened\n");
//		return EStopUnknown;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetStopBits - Unable to obtain DCB information\n");
//		return EStopUnknown;
//	}
//
//	// Return the appropriate stopbits
//	return EStopBits(dcb.StopBits);
//}
//
//DWORD CSerial::GetEventMask (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetEventMask - Device is not opened\n");
//		return 0;
//	}
//
//	// Return the event mask
//	return m_dwEventMask;
//}
//
//BYTE CSerial::GetEventChar (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetEventChar - Device is not opened\n");
//		return 0;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetEventChar - Unable to obtain DCB information\n");
//		return 0;
//	}
//
//	// Set the new event character
//	return BYTE(dcb.EvtChar);
//}
//
//CSerial::EHandshake CSerial::GetHandshaking (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetHandshaking - Device is not opened\n");
//		return EHandshakeUnknown;
//	}
//
//	// Obtain the DCB structure for the device
//	CDCB dcb;
//	if (!::GetCommState(m_hFile,&dcb))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetHandshaking - Unable to obtain DCB information\n");
//		return EHandshakeUnknown;
//	}
//
//	// Check if hardware handshaking is being used
//	if ((dcb.fDtrControl == DTR_CONTROL_HANDSHAKE) && (dcb.fRtsControl == RTS_CONTROL_HANDSHAKE))
//		return EHandshakeHardware;
//
//	// Check if software handshaking is being used
//	if (dcb.fOutX && dcb.fInX)
//		return EHandshakeSoftware;
//
//	// No handshaking is being used
//	return EHandshakeOff;
//}
//
//LONG CSerial::Write (const void* pData, size_t iLen, DWORD* pdwWritten, LPOVERLAPPED lpOverlapped, DWORD dwTimeout)
//{
//	// Check if time-outs are supported
//	CheckRequirements(lpOverlapped,dwTimeout);
//
//	// Overlapped operation should specify the pdwWritten variable
//	//_ASSERTE(!lpOverlapped || pdwWritten);
//
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Use our own variable for read count
//	DWORD dwWritten;
//	if (pdwWritten == 0)
//	{
//		pdwWritten = &dwWritten;
//	}
//
//	// Reset the number of bytes written
//	*pdwWritten = 0;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::Write - Device is not opened\n");
//		return m_lLastError;
//	}
//
//#ifndef SERIAL_NO_OVERLAPPED
//
//	// Check if an overlapped structure has been specified
//	if (!m_hevtOverlapped && (lpOverlapped || (dwTimeout != INFINITE)))
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_FUNCTION;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::Write - Overlapped I/O is disabled, specified parameters are illegal.\n");
//		return m_lLastError;
//	}
//
//	// Wait for the event to happen
//	OVERLAPPED ovInternal;
//	if (!lpOverlapped && m_hevtOverlapped)
//	{
//		// Setup our own overlapped structure
//		memset(&ovInternal,0,sizeof(ovInternal));
//		ovInternal.hEvent = m_hevtOverlapped;
//
//		// Use our internal overlapped structure
//		lpOverlapped = &ovInternal;
//	}
//
//	// Make sure the overlapped structure isn't busy
//	//_ASSERTE(!m_hevtOverlapped || HasOverlappedIoCompleted(lpOverlapped));
//
//	// Write the data
//	if (!::WriteFile(m_hFile,pData,iLen,pdwWritten,lpOverlapped))
//	{
//		// Set the internal error code
//		long lLastError = ::GetLastError();
//
//		// Overlapped operation in progress is not an actual error
//		if (lLastError != ERROR_IO_PENDING)
//		{
//			// Save the error
//			m_lLastError = lLastError;
//
//			// Issue an error and quit
//			//_RPTF0(_CRT_WARN,"CSerial::Write - Unable to write the data\n");
//			return m_lLastError;
//		}
//
//		// We need to block if the client didn't specify an overlapped structure
//		if (lpOverlapped == &ovInternal)
//		{
//			// Wait for the overlapped operation to complete
//			switch (::WaitForSingleObject(lpOverlapped->hEvent,dwTimeout))
//			{
//			case WAIT_OBJECT_0:
//				// The overlapped operation has completed
//				if (!::GetOverlappedResult(m_hFile,lpOverlapped,pdwWritten,FALSE))
//				{
//					// Set the internal error code
//					m_lLastError = ::GetLastError();
//
//					//_RPTF0(_CRT_WARN,"CSerial::Write - Overlapped completed without result\n");
//					return m_lLastError;
//				}
//				break;
//
//			case WAIT_TIMEOUT:
//				// Cancel the I/O operation
//				CancelCommIo();
//
//				// The operation timed out. Set the internal error code and quit
//				m_lLastError = ERROR_TIMEOUT;
//				return m_lLastError;
//
//			default:
//				// Set the internal error code
//				m_lLastError = ::GetLastError();
//
//				// Issue an error and quit
//				//_RPTF0(_CRT_WARN,"CSerial::Write - Unable to wait until data has been sent\n");
//				return m_lLastError;
//			}
//		}
//	}
//	else
//	{
//		// The operation completed immediatly. Just to be sure
//		// we'll set the overlapped structure's event handle.
//		if (lpOverlapped)
//			::SetEvent(lpOverlapped->hEvent);
//	}
//
//#else
//
//	// Write the data
//	if (!::WriteFile(m_hFile,pData,iLen,pdwWritten,0))
//	{
//		// Set the internal error code
//		m_lLastError = ::GetLastError();
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::Write - Unable to write the data\n");
//		return m_lLastError;
//	}
//
//#endif
//
//	// Return successfully
//	return m_lLastError;
//}
//
//LONG CSerial::Write (LPCSTR pString, DWORD* pdwWritten, LPOVERLAPPED lpOverlapped, DWORD dwTimeout)
//{
//	// Check if time-outs are supported
//	CheckRequirements(lpOverlapped,dwTimeout);
//
//	// Determine the length of the string
//	return Write(pString,strlen(pString),pdwWritten,lpOverlapped,dwTimeout);
//}
//
//LONG CSerial::Read (void* pData, size_t iLen, DWORD* pdwRead, LPOVERLAPPED lpOverlapped, DWORD dwTimeout)
//{
//	// Check if time-outs are supported
//	CheckRequirements(lpOverlapped,dwTimeout);
//
//	// Overlapped operation should specify the pdwRead variable
//	//_ASSERTE(!lpOverlapped || pdwRead);
//
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Use our own variable for read count
//	DWORD dwRead;
//	if (pdwRead == 0)
//	{
//		pdwRead = &dwRead;
//	}
//
//	// Reset the number of bytes read
//	*pdwRead = 0;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::Read - Device is not opened\n");
//		return m_lLastError;
//	}
//
//#ifdef _DEBUG
//	// The debug version fills the entire data structure with
//	// 0xDC bytes, to catch buffer errors as soon as possible.
//	memset(pData,0xDC,iLen);
//#endif
//
//#ifndef SERIAL_NO_OVERLAPPED
//
//	// Check if an overlapped structure has been specified
//	if (!m_hevtOverlapped && (lpOverlapped || (dwTimeout != INFINITE)))
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_FUNCTION;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::Read - Overlapped I/O is disabled, specified parameters are illegal.\n");
//		return m_lLastError;
//	}
//
//	// Wait for the event to happen
//	OVERLAPPED ovInternal;
//	if (lpOverlapped == 0)
//	{
//		// Setup our own overlapped structure
//		memset(&ovInternal,0,sizeof(ovInternal));
//		ovInternal.hEvent = m_hevtOverlapped;
//
//		// Use our internal overlapped structure
//		lpOverlapped = &ovInternal;
//	}
//
//	// Make sure the overlapped structure isn't busy
//	//_ASSERTE(!m_hevtOverlapped || HasOverlappedIoCompleted(lpOverlapped));
//	
//	// Read the data
//	if (!::ReadFile(m_hFile,pData,iLen,pdwRead,lpOverlapped))
//	{
//		// Set the internal error code
//		long lLastError = ::GetLastError();
//
//		// Overlapped operation in progress is not an actual error
//		if (lLastError != ERROR_IO_PENDING)
//		{
//			// Save the error
//			m_lLastError = lLastError;
//
//			// Issue an error and quit
//			//_RPTF0(_CRT_WARN,"CSerial::Read - Unable to read the data\n");
//			return m_lLastError;
//		}
//
//		// We need to block if the client didn't specify an overlapped structure
//		if (lpOverlapped == &ovInternal)
//		{
//			// Wait for the overlapped operation to complete
//			switch (::WaitForSingleObject(lpOverlapped->hEvent,dwTimeout))
//			{
//			case WAIT_OBJECT_0:
//				// The overlapped operation has completed
//				if (!::GetOverlappedResult(m_hFile,lpOverlapped,pdwRead,FALSE))
//				{
//					// Set the internal error code
//					m_lLastError = ::GetLastError();
//
//					//_RPTF0(_CRT_WARN,"CSerial::Read - Overlapped completed without result\n");
//					return m_lLastError;
//				}
//				break;
//
//			case WAIT_TIMEOUT:
//				// Cancel the I/O operation
//				CancelCommIo();
//
//				// The operation timed out. Set the internal error code and quit
//				m_lLastError = ERROR_TIMEOUT;
//				return m_lLastError;
//
//			default:
//				// Set the internal error code
//				m_lLastError = ::GetLastError();
//
//				// Issue an error and quit
//				//_RPTF0(_CRT_WARN,"CSerial::Read - Unable to wait until data has been read\n");
//				return m_lLastError;
//			}
//		}
//	}
//	else
//	{
//		// The operation completed immediatly. Just to be sure
//		// we'll set the overlapped structure's event handle.
//		if (lpOverlapped)
//			::SetEvent(lpOverlapped->hEvent);
//	}
//
//#else
//	
//	// Read the data
//	if (!::ReadFile(m_hFile,pData,iLen,pdwRead,0))
//	{
//		// Set the internal error code
//		m_lLastError = ::GetLastError();
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::Read - Unable to read the data\n");
//		return m_lLastError;
//	}
//
//#endif
//
//	// Return successfully
//	return m_lLastError;
//}
//
//LONG CSerial::Purge()
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::Purge - Device is not opened\n");
//		return m_lLastError;
//	}
//
//	if (!::PurgeComm(m_hFile, PURGE_TXCLEAR | PURGE_RXCLEAR))
//	{
//		// Set the internal error code
//		m_lLastError = ::GetLastError();
//		//_RPTF0(_CRT_WARN,"CSerial::Purge - Overlapped completed without result\n");
//	}
//	
//	// Return successfully
//	return m_lLastError;
//}
//
//LONG CSerial::Break (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::Break - Device is not opened\n");
//		return m_lLastError;
//	}
//
//    // Set the RS-232 port in break mode for a little while
//    ::SetCommBreak(m_hFile);
//    ::Sleep(100);
//    ::ClearCommBreak(m_hFile);
//
//	// Return successfully
//	return m_lLastError;
//}
//
//CSerial::EEvent CSerial::GetEventType (void)
//{
//#ifdef _DEBUG
//	// Check if the event is within the mask
////	if ((m_eEvent & m_dwEventMask) == 0)
////		_RPTF2(_CRT_WARN,"CSerial::GetEventType - Event %08Xh not within mask %08Xh.\n", m_eEvent, m_dwEventMask);
//#endif
//
//	// Obtain the event (mask unwanted events out)
//	EEvent eEvent = EEvent(m_eEvent & m_dwEventMask);
//
//	// Reset internal event type
//	m_eEvent = EEventNone;
//
//	// Return the current cause
//	return eEvent;
//}
//
//CSerial::EError CSerial::GetError (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Check if the device is open
//	if (m_hFile == 0)
//	{
//		// Set the internal error code
//		m_lLastError = ERROR_INVALID_HANDLE;
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetError - Device is not opened\n");
//		return EErrorUnknown;
//	}
//
//	// Obtain COM status
//	DWORD dwErrors = 0;
//	if (!::ClearCommError(m_hFile,&dwErrors,0))
//	{
//		// Set the internal error code
//		m_lLastError = ::GetLastError();
//
//		// Issue an error and quit
//		//_RPTF0(_CRT_WARN,"CSerial::GetError - Unable to obtain COM status\n");
//		return EErrorUnknown;
//	}
//
//	// Return the error
//	return EError(dwErrors);
//}
//
//bool CSerial::GetCTS (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Obtain the modem status
//	DWORD dwModemStat = 0;
//	if (!::GetCommModemStatus(m_hFile,&dwModemStat))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetCTS - Unable to obtain the modem status\n");
//		return false;
//	}
//
//	// Determine if CTS is on
//	return (dwModemStat & MS_CTS_ON) != 0;
//}
//
//bool CSerial::GetDSR (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Obtain the modem status
//	DWORD dwModemStat = 0;
//	if (!::GetCommModemStatus(m_hFile,&dwModemStat))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetDSR - Unable to obtain the modem status\n");
//		return false;
//	}
//
//	// Determine if DSR is on
//	return (dwModemStat & MS_DSR_ON) != 0;
//}
//
//bool CSerial::GetRing (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Obtain the modem status
//	DWORD dwModemStat = 0;
//	if (!::GetCommModemStatus(m_hFile,&dwModemStat))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetRing - Unable to obtain the modem status");
//		return false;
//	}
//
//	// Determine if Ring is on
//	return (dwModemStat & MS_RING_ON) != 0;
//}
//
//bool CSerial::GetRLSD (void)
//{
//	// Reset error state
//	m_lLastError = ERROR_SUCCESS;
//
//	// Obtain the modem status
//	DWORD dwModemStat = 0;
//	if (!::GetCommModemStatus(m_hFile,&dwModemStat))
//	{
//		// Obtain the error code
//		m_lLastError = ::GetLastError();
//
//		// Display a warning
//		//_RPTF0(_CRT_WARN,"CSerial::GetRLSD - Unable to obtain the modem status");
//		return false;
//	}
//
//	// Determine if RLSD is on
//	return (dwModemStat & MS_RLSD_ON) != 0;
//}
