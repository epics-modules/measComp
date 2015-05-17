An [EPICS](http://www.aps.anl.gov/epics/) 
module that supports USB I/O modules from [Measurement Computing](http://www.mccdaq.com).

This module is currently only supported on Windows, with the win32-x86, windows-x64,
and cygwin-x86 EPICS architectures. The driver uses the Measurement Computing "Universal
Library", which is only available on Windows. 

Models supported in measComp include:
* The [USB-CTR08](http://www.mccdaq.com/usb-data-acquisition/USB-CTR08.aspx).
This device is an 8-channel counter/timer module. The driver supports the EPICS
scaler record with 8 32-bit counters. Counter 0 can be used as a preset counter
that controls the other 7 counters, the same as the Joerger VSC or the SIS3820.
The driver also supports using the USB-CTR08 in multi-channel scaler (MCS) mode
like the SIS3820, with internal or external channel advance and dwell times as short
as 250 ns. The device also contains 4 programmable pulse generators, and 8 digital
I/O bits, which are supported with standard EPICS records.

* The [USB-1608GX-2AO](http://www.mccdaq.com/usb-data-acquisition/USB-1608GX-2AO.aspx).
analog I/O module. This device contains 16 single-ended/8 differential 16-bit analog
inputs, 2 16-bit analog outputs, and 8 binary input/outputs. The analog inputs and
outputs can each be run at 500 kHz. The driver supports simple analog I/O, 2 waveform
generators using the analog outputs, an 8 or 16 channel waveform digitizer using
the analog inputs, binary I/O, and a pulse generator.

* The [USB-4303](http://www.mccdaq.com/usb-data-acquisition/USB-4303.aspx).
This device is a counter/timer module. It contains 2 9513 counter/timer chips, each
of which contains 5 16-bit counter/timer units. The driver supports independent
control of each counter/timer unit, but it also supports the EPICS scaler record
with 4 32-bit counters, a 16-bit preset timer, and one 16-bit counter. The device
also contains 8 binary outputs, 8 binary inputs, which are supported with standard
EPICS bi, bo, longin, and longout records. NOTE: This module is obsolete and is
no longer be available. Use the USB-CTR08 instead.
 
Additional information:
* [Documentation](http://cars.uchicago.edu/software/epics/measCompDoc.html)
* [Release notes](http://cars.uchicago.edu/software/epics/measCompReleaseNotes.html)
