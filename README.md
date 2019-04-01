An [EPICS](http://www.aps.anl.gov/epics/) 
module that supports USB and Ethernet I/O modules from [Measurement Computing](http://www.mccdaq.com).

This module is supported on both Windows and Linux, 64-bit and 32-bit. 
On Windows it uses the Measurement Computing "Universal Library" (UL), which is only available on Windows.

On Linux it uses the [low-level drivers from Warren Jasper](https://github.com/wjasper/Linux_Drivers).
On top of these drivers the module provides a layer that emulates the Windows UL library from Measurement Computing.  
The EPICS drivers thus always use the UL API and are identical on Linux and Windows.
The Linux UL layer is independent of EPICS, and uses std::thread and std::mutex to provide the required
threading and mutex capabilities.  These methods require C++11, and so will not build with very old compilers.
They do build with gcc 4.8.5 on RHEL 7/Centos 7, and gcc 4.4.7 on RHEL 6/Centos 6.
Currently only the E-1608, E-TC, and E-TC32 models are supported on Linux.
Support for other modules is straightforward to add and can be done as the demand arises.

Models supported in measComp include:
* The [USB-CTR08](http://www.mccdaq.com/usb-data-acquisition/USB-CTR08.aspx).
This device is an 8-channel counter/timer module. The driver supports the EPICS
scaler record with 8 32-bit counters. Counter 0 can be used as a preset counter
that controls the other 7 counters, the same as the Joerger VSC or the SIS3820.
The driver also supports using the USB-CTR08 in multi-channel scaler (MCS) mode
like the SIS3820, with internal or external channel advance and dwell times as short
as 250 ns. The device also contains 4 programmable pulse generators, and 8 digital
I/O bits, which are supported with standard EPICS records.

* The [USB-1208LS](http://www.mccdaq.com/usb-data-acquisition/USB-1208LS.aspx).
This device has 8 single-ended/4 differential 12-bit analog
inputs, 2 10-bit analog outputs, and 16 binary input/outputs. The driver supports
simple analog I/O, binary I/O, and a counter input.

* The [USB-1208HS](http://www.mccdaq.com/usb-data-acquisition/USB-1208HS.aspx).
This device has 8 single-ended/4 differential 13-bit analog
inputs, 16 binary input/outputs, and 2 counters. The driver supports
simple analog I/O, binary I/O, and a counter input.

* The [USB-231](http://www.mccdaq.com/usb-data-acquisition/USB-231.aspx).
This device has 8 single-ended/4 differential 16-bit analog
inputs, 2 16-bit analog outputs, and 8 binary input/outputs. The analog inputs 
can be run at 50 kHz, and the analog outputs at 5 kHz. The driver supports simple analog I/O, 
2 waveform generators using the analog outputs, an 4 or 8 channel waveform digitizer using
the analog inputs, and binary I/O.

* The [USB-1608GX-2AO](http://www.mccdaq.com/usb-data-acquisition/USB-1608GX-2AO.aspx)
analog I/O module. This device contains 16 single-ended/8 differential 16-bit analog
inputs, 2 16-bit analog outputs, and 8 binary input/outputs. The analog inputs and
outputs can each be run at 500 kHz. The driver supports simple analog I/O, 2 waveform
generators using the analog outputs, an 8 or 16 channel waveform digitizer using
the analog inputs, binary I/O, and a pulse generator.

* The [USB-1608G](http://www.mccdaq.com/usb-data-acquisition/USB-1608G.aspx)
analog I/O module. This is similar to the USB-1608GX-2AO except that it runs at
250 kHz rather than 500 kHz, and does not have the 2 analog outputs.

* The [E-1608](https://www.mccdaq.com/ethernet-data-acquisition/E-1608-Series)
analog I/O module. This is an Ethernet device with 8 single-ended/4
differential 16-bit analog inputs, 2 16-bit analog outputs, and 8 binary input/outputs. The analog inputs
can be run at 250 kHz. The driver supports simple analog I/O, an 4 or 8 channel
waveform digitizer using the analog inputs, binary I/O, and 1 counter input.

* The [USB-2408-2AO](http://www.mccdaq.com/usb-data-acquisition/USB-2408-2AO.aspx).
This device contains 16 single-ended/8 differential 24-bit analog inputs, 
2 16-bit analog outputs, and 8 binary input/outputs. The analog inputs can
measure either temperature or voltage. Thermocouple types B, E, J, K, N, R, S and
T are supported. The analog inputs and outputs can each be run at 1 kHz. The driver
supports simple analog I/O, 2 waveform generators using the analog outputs, an 8
or 16 channel waveform digitizer using the analog inputs, binary I/O, and 2 counter
inputs.

* The [E-TC](https://www.mccdaq.com/ethernet-data-acquisition/thermocouple-input/24-bit-daq/E-TC.aspx)
thermocouple module. This is an Ethernet device with 8 24-bit thermopouple inputs, a 32-bit counter and 8 binary input/outputs. 
The driver supports temperature measurements at up to 4 Hz, binary I/O, and 1 counter input.

* The [TC-32 thermocouple module](http://www.mccdaq.com/usb-ethernet-data-acquisition/temperature/usb-ethernet-24-bit-thermocouple-daq/TC-32.aspx)
This device contains 32 thermocouple inputs, 8 binary inputs and 32 binary outputs.
Thermocouple types B, E, J, K, N, R, S and T are supported.
The binary outputs can be controlled by the driver or can be configured to be an
alarm output controlled by the corresponding thermocouple input. USB and Ethernet
interfaces are both provided, and either can be used.

* The [USB-4303](http://www.mccdaq.com/usb-data-acquisition/USB-4303.aspx).
This device is a counter/timer module. It contains 2 9513 counter/timer chips, each
of which contains 5 16-bit counter/timer units. The driver supports independent
control of each counter/timer unit, but it also supports the EPICS scaler record
with 4 32-bit counters, a 16-bit preset timer, and one 16-bit counter. The device
also contains 8 binary outputs, 8 binary inputs, which are supported with standard
EPICS bi, bo, longin, and longout records. NOTE: This module is obsolete and is
no longer available. Use the USB-CTR08 instead.
 
Additional information:
* [Home page](https://cars.uchicago.edu/software/epics/measComp.html)
* [Documentation](https://cars.uchicago.edu/software/epics/measCompDoc.html)
* [Release notes](https://cars.uchicago.edu/software/epics/measCompReleaseNotes.html)
