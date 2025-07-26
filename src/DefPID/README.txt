

DefPID Library:
   The 'pidDef' class is directly derived from the Arduino-PID-Library, (vers 1.2.1) with some
   minor changes - use float instead of double, and use functions to set the
   setpoint, input and output.
   
   The license for the original library is as follows:

***************************************************************
* Arduino PID Library - Version 1.2.1
* by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
*
* This Library is licensed under the MIT License
***************************************************************

The full, original, text of the library is under the 'doc' folder for refrence.

Changes by Doug Fajardo 7/26/2025:
(1) Change name of the class from PID to DefPid.
(2) Use float instead of double
(3) Add function to determine time until next call to compute.
(4) 