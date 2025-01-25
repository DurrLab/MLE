# Multi-Contrast Laser Endoscopy (MLE) Desktop Software

## Prerequisites
### Software
* Windows 10
* Microsoft Visual Studio
* Nvidia Device Drivers
* Matrox Imaging Library (MIL)
* Thorlabs Kinesis Software
### Hardware
* Matrox Orion HD Frame Grabber
* NVIDIA GPU
* [Light Modulation Controller](../lmc/)
* Compact Direct Drive Rotation Mount, DDR25, Thorlabs
* K-Cube Brushless DC Servo Driver, KBD101, Thorlabs

The project should be configured to include the MIL static library header and the Kinesis library header directories. The compiler should also be linked to the MIL and Kinesis library directories.

## Notes on configuring the Matrox Orion HD Frame Grabber
### Physical installation of Matrox Orion HD card
After installing the Orion HD frame grabber in the desktop, the system booted to a black screen. This occurred because the desktop tried to output the display through the Matrox card instead of the GPU connected to the monitor. To resolve this, power on the computer and enter the BIOS during boot. Navigate to the video settings and check which PCI slot the system is using for video output. In my case, the setting was set to 'Automatic,' causing the system to prioritize the Matrox PCI slot over the GPU PCI slot. To fix this, disable automatic video card detection and manually select the PCI slot for the preferred GPU (PCI Slot 2, in my case). This ensures the system uses the primary graphics card for video output. Afterward, power down the computer, unplug it, and install the Orion HD video capture card into a compatible PCI slot on the motherboard. Finally, reboot the computer.

### Matrox Imaging Library (MIL) API Installation
The Matrox Imaging Library drivers and API are required to utilize the Orion HD frame grabber. Follow the steps below to complete the installation.
1.) Run the MIL64Setup executable to begin the installation. Note: When the installer asks which boards you would like to install, do not select OrionHD or any other boards. There is a bug in the installer that will cause the device driver installation to fail.
2.) Once the installation finishes, reboot the computer.
3.) Open the MILConfig program. Go to Updates->Download Manager. Select the MIL 10 Update 57 item, then select “Open download folder”. Find the m10U057B20X64 executable and run it to install the latest OrionHD device drivers.
4.) Reboot the computer.
5.) Open MILConfig. Go to Boards->OrionHD->Device Power Management and disable the ASPM.
6.) Reboot your computer.

Prior to running the software in this repo, use MILConfig to set the default Device number, Default Digitizer Format (DCF), and Digitizer number. These settings are found under General->Default Values.



## License
This work is licensed under CC BY-NC-SA 4.0
