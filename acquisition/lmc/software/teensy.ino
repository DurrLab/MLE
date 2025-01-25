/************************************************************************************************/
/*
 *  File name:    teensy.ino
 *
 *  Synopsis:     Code for the light modulation controller within the Multi-contrast 
 *                Laser Endoscopy (MLE) system. The code is designed to run on a 
 *                Teensy 4.0 mounted on the control board.
 *              
 *                Packets containing pulse width lengths are continually sent by the host
 *                desktop over Serial USB for execution by the controller, and voltage
 *                measurements from the photodiode power monitoring units are sent back to
 *                the host desktop.
 *              
 *                The program triggers a new set of pulse width lengths and takes a voltage
 *                reading when the interrupt pin is triggered by the frame-sync signal.
 *              
 *                Upon connection with the host desktop, the pulse width buffer is populated
 *                with a set of empty packets equal to the length of the image buffer list on
 *                the host desktop, effectively causing it to work several frames behind the
 *                host desktop. This ensures that the controller does not run out of pulse
 *                width lengths if the the host desktop falls behind on frame processing.
 *              
 *  Dependencies: CircularBuffer (Arduino Library Manager)
 *                TeensyTimerTool (Arduino Library Manager)
 *  
 *  
 *  Taylor Bobrow, Johns Hopkins University (2025)
 *
*/
/************************************************************************************************/

#define CIRCULAR_BUFFER_INT_SAFE // make buffers thread-safe
#include <CircularBuffer.h>
#include <TeensyTimerTool.h>

/* System Parameters. */
const int NO_LASER_DIODES                   = 15;                                       // number of laser diodes output channels
const int NO_PHOTO_DIODES                   = 3;                                        // number of photodiode monitoring input channels
const int LASER_DIODE_PINS[NO_LASER_DIODES] = {3,4,5,6,7,8,9,10,11,14,15,16,17,18,19};  // digital output pin for each laser diode
const int PHOTO_DIODE_PINS[NO_PHOTO_DIODES] = {21,22,23};                               // analog input pin for each photodiode
const int ODD_EVEN_SYNC_PIN                 = 2;                                        // digital input pin for odd-even field sync
const int PHOTO_DIODE_READ_DELAY            = 50;                                       // delay from trigger to read voltages (microseconds)
const int PHOTO_DIODE_READ_BITS             = 12;                                       // ADC read resolution
const int PHOTO_DIODE_READ_NO_SAMPLES       = 10;                                       // number of ADC samples averaged per reading
const int FRAME_BUFFER_SIZE                 = 10;                                       // length of frame buffer in desktop program
const int FID_RESET                         = -1;                                       // fid value to trigger a reset (from host)
const int FID_ERROR                         = -2;                                       // fid value to communicate an error (to host)

/* Type definitions. */
typedef struct {
    int            fid;                                                                 // frame id
    unsigned short pws[2*NO_LASER_DIODES];                                              // one pulse width for each diode for odd/even fields (in microseconds)
} ld_packet;

typedef struct {
    int            fid;                                                                 // frame id                     
    unsigned short pdvs[2*NO_PHOTO_DIODES];                                             // one voltage value for each diode for odd/even fields
} pd_packet;

/* Timers. */
TeensyTimerTool::TimerGenerator* const timerPool[] = {TeensyTimerTool::TMR1,            // override the default timer pool to use the quad timers
                                                      TeensyTimerTool::TMR2,
                                                      TeensyTimerTool::TMR3,
                                                      TeensyTimerTool::TMR4};
TeensyTimerTool::OneShotTimer laserDiodeTimers[NO_LASER_DIODES];                        // allocate one timer for each laser diode channel
TeensyTimerTool::OneShotTimer photoDiodeTimer;                                          // allocate one timer for all of the photodiode channels

/* Global Variables. */
CircularBuffer<ld_packet, 2*FRAME_BUFFER_SIZE> gLdBuffer;                               // fifo buffer for pulse widths recieved from the host
CircularBuffer<pd_packet, 2*FRAME_BUFFER_SIZE> gPdBuffer;                               // fifo buffer for photo diode measurements to be sent to the host
ld_packet gCurrentLdPacket;                                                             // current laser diode packet from host
pd_packet gCurrentPdPacket;                                                             // current photo diode packet to send to host
bool      gCurrentField;                                                                // current frame field (0 = odd, 1 = even)
bool      gInitialized;


/* Setup. */
void setup()
{
  // configure one digital output pin and pulse width timer per laser diode
  // the output pin is toggled low when the quad timer expires
  for (unsigned int n = 0; n < NO_LASER_DIODES; n++)
  {
    pinMode(LASER_DIODE_PINS[n], OUTPUT);
    laserDiodeTimers[n].begin([n] { digitalWriteFast(LASER_DIODE_PINS[n], LOW); });
  }

  // configure the photodiode analog voltage input pins
  analogReadResolution(PHOTO_DIODE_READ_BITS);
  analogReadAveraging(PHOTO_DIODE_READ_NO_SAMPLES);
  for (unsigned int n = 0; n < NO_PHOTO_DIODES; n++)
    pinMode(PHOTO_DIODE_PINS[n], INPUT_DISABLE);

  // configure one timer for triggering photodiode reading
  photoDiodeTimer.begin(READ_PHOTO_DIODE_PINS);

  // configure the odd-even input pin for interrupt triggering
  pinMode(ODD_EVEN_SYNC_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ODD_EVEN_SYNC_PIN), ISR_ODD_EVEN_SYNC, CHANGE);

  // start the controller in an uninitialized state
  gInitialized = false;
  
  // open Serial USB for communication with host desktop - baud rate ignored on Teensy
  Serial.begin(115200);
}


/* Main loop. */
void loop()
{ 
  // if we have recieved a full packet from the host desktop
  if(Serial.available() >= sizeof(ld_packet))
  {
    // read in the packet
    ld_packet ipacket;
    Serial.readBytes((byte*)&ipacket, sizeof(ld_packet));

    // if the fid is equal to FID_RESET, then reset the system
    if(ipacket.fid == FID_RESET)
    {
      gLdBuffer.clear();
      gPdBuffer.clear();
      gInitialized = false;
    }
    // otherwise, store the pulse width packet
    else
    {
      gLdBuffer.push(ipacket);
    }
    
    // if we are uninitialized and have recieved enough entries
    // from the host desktop, flag as initialized to start pulse
    // width triggering and voltage reading
    if(gLdBuffer.size()==FRAME_BUFFER_SIZE &&  !gInitialized)
    {
      gInitialized = true;
    }
  }

  // if we have photodiode entries, send to the host desktop
  if(gPdBuffer.size()>0)
  {
    pd_packet oentry = gPdBuffer.shift();
    Serial.write((byte*)&oentry, sizeof(pd_packet));
    Serial.send_now();
  }
}


/* Interrupt function called on every odd/even signal pulse at the start of a new field acquisition. */
void ISR_ODD_EVEN_SYNC(void)
{
  // if we are initialized
  if(gInitialized)
  {
    // if we have pulse width packets in the buffer
    if(gLdBuffer.size()>0)
    {
      // check whether we are in the odd or even field
      gCurrentField = !digitalRead(ODD_EVEN_SYNC_PIN);
      
      // if we have started a new frame (odd field), then store the voltage values from 
      // from the previous frame and load the new pulse widths
      if(!gCurrentField)
      {
        gPdBuffer.push(gCurrentPdPacket);
        gCurrentLdPacket = gLdBuffer.shift();
        gCurrentPdPacket.fid = gCurrentLdPacket.fid;
      }
    
      // toggle the laser diodes on and arm the timers
      for(int n=0; n<NO_LASER_DIODES; n++)
      {
        if(gCurrentLdPacket.pws[gCurrentField*NO_LASER_DIODES + n] > 0)
        {
          digitalWriteFast(LASER_DIODE_PINS[n], HIGH);
          laserDiodeTimers[n].trigger(gCurrentLdPacket.pws[gCurrentField*NO_LASER_DIODES + n]);
        }
      }
      
      // arm the timer for the analog input measurements
      photoDiodeTimer.trigger(PHOTO_DIODE_READ_DELAY);
    }
    // if we are initialized and don't have pulse width packets in the buffer,
    // then we are out of sync. Send an error to the host.
    else
    {
      gCurrentPdPacket.fid = FID_ERROR;
      gPdBuffer.push(gCurrentPdPacket);
    } 
  }
}


/* Uses the teensy on-board ADC to read the voltage values on the photodiode input pins. */
void READ_PHOTO_DIODE_PINS()
{
  for(int n=0; n<NO_PHOTO_DIODES; n++)
    gCurrentPdPacket.pdvs[(gCurrentField*NO_PHOTO_DIODES) + n] = (unsigned short)analogRead(PHOTO_DIODE_PINS[n]);
}