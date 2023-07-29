## Overview

measComp is an [EPICS](http://www.aps.anl.gov/epics/) 
module that supports USB and Ethernet I/O modules from [Measurement Computing](http://www.mccdaq.com).

This module is supported on both Windows and Linux. 
On Windows it uses the Measurement Computing "Universal Library" (UL).

In R4-0 and later it uses the UL for Linux library from Measurement Computing for Linux drivers.
This is an [open-source library available on Github](https://github.com/mccdaq/uldaq). 
The Linux Universal Library API is similar to the Windows UL API, but the functions have different names and different syntax. 

UL for Windows and Linux support most current Measurement Computing models.

In versions prior to R4-0 the Linux support used the [low-level drivers from Warren Jasper](https://github.com/wjasper/Linux_Drivers).
On top of these drivers the module provides a layer that emulates the Windows UL library from Measurement Computing.
The EPICS drivers thus always use the Windows UL API and are identical on Linux and Windows. 
The E-1608, E-TC, E-TC32, E-DIO24, USB-1608G-2AO, USB-CTR08, USB-TEMP, USB-TEMP-AI and USB-31XX models are supported in these versions.

## Supported models
Models supported in measComp include:
* The [E-1608](https://www.mccdaq.com/ethernet-data-acquisition/E-1608-Series)
analog I/O module. This is an Ethernet device with 8 single-ended/4
differential 16-bit analog inputs, 2 16-bit analog outputs, and 8 binary input/outputs. The analog inputs
can be run at 250 kHz. The driver supports simple analog I/O, an 4 or 8 channel
waveform digitizer using the analog inputs, binary I/O, and 1 counter input.

* The [E-TC](https://www.mccdaq.com/ethernet-data-acquisition/thermocouple-input/24-bit-daq/E-TC.aspx)
thermocouple module. This is an Ethernet device with 8 24-bit thermopouple inputs, a 32-bit counter and 8 binary input/outputs. 
The driver supports temperature measurements at up to 4 Hz, binary I/O, and 1 counter input.

* The [E-DIO24 digital I/O module](https://www.mccdaq.com/ethernet-data-acquisition/24-channel-digital-io-daq/E-DIO24-Series).
This device contains 24 digital I/O lines and 1 counter.

* The [TC-32 thermocouple module](http://www.mccdaq.com/usb-ethernet-data-acquisition/temperature/usb-ethernet-24-bit-thermocouple-daq/TC-32.aspx)
This device contains 32 thermocouple inputs, 8 binary inputs and 32 binary outputs.
Thermocouple types B, E, J, K, N, R, S and T are supported.
The binary outputs can be controlled by the driver or can be configured to be an
alarm output controlled by the corresponding thermocouple input. USB and Ethernet
interfaces are both provided, and either can be used.

* The [USB-CTR08](http://www.mccdaq.com/usb-data-acquisition/USB-CTR08.aspx).
This device is an 8-channel counter/timer module. The driver supports the EPICS
scaler record with 8 32-bit counters. Counter 0 can be used as a preset counter
that controls the other 7 counters, the same as the Joerger VSC or the SIS3820.
The driver also supports using the USB-CTR08 in multi-channel scaler (MCS) mode
like the SIS3820, with internal or external channel advance and dwell times as short
as 250 ns. The device also contains 4 programmable pulse generators, and 8 digital
I/O bits, which are supported with standard EPICS records.

* The [USB-TEMP](https://www.mccdaq.com/usb-data-acquisition/USB-TEMP.aspx) and
[USB-TEMP-AI](https://www.mccdaq.com/usb-data-acquisition/USB-TEMP-Series.aspx).
These devices support RTD, thermocouple, thermistor, and semiconductor temperature
sensors.  The USB-TEMP supports up to 8 temperature inputs.  The USB-TEMP-AI supports
up to 4 temperature inputs and 4 24-bit voltage inputs. The devices also support
8 binary input/outputs, and 1 counter. The driver supports temperature input, analog input,
binary I/O, and a counter input.

* The [USB-1208LS](http://www.mccdaq.com/usb-data-acquisition/USB-1208LS.aspx).
This device has 8 single-ended/4 differential 12-bit analog
inputs, 2 10-bit analog outputs, and 16 binary input/outputs. The driver supports
simple analog I/O, binary I/O, and a counter input.

* The [USB-1208FS-Plus](http://www.mccdaq.com/usb-data-acquisition/USB-1208FS-Plus.aspx).
This device has 8 single-ended/4 differential 12-bit analog
inputs, 2 12-bit analog outputs, and 16 binary input/outputs. The driver supports
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

* The [USB-1808 and USB-1808X](https://www.mccdaq.com/data-acquisition-and-control/simultaneous-daq/USB-1808-Series.aspx).
This device contains 8 single-ended/differential 18-bit analog inputs, 
2 16-bit analog outputs, 4 binary input/outputs, 2 pulse generators, 2 counter inputs, and 
2 quadrature encoder inputs. The driver supports simple analog and binary I/O, 
2 waveform generators up to 1 MHz using the analog and/or digital outputs,
and a waveform digitizer up to 500 kHz using the analog, digital, and encoder inputs.

* The [USB-2408-2AO](http://www.mccdaq.com/usb-data-acquisition/USB-2408-2AO.aspx).
This device contains 16 single-ended/8 differential 24-bit analog inputs, 
2 16-bit analog outputs, and 8 binary input/outputs. The analog inputs can
measure either temperature or voltage. Thermocouple types B, E, J, K, N, R, S and
T are supported. The analog inputs and outputs can each be run at 1 kHz. The driver
supports simple analog I/O, 2 waveform generators using the analog outputs, an 8
or 16 channel waveform digitizer using the analog inputs, binary I/O, and 2 counter
inputs.

* The [USB-3100 series](https://www.mccdaq.com/usb-data-acquisition/USB-3100-Series.aspx)
analog output modules. These have 4, 8, or 16 analog outputs, 8 binary I/O and one counter input.

## Installing the vendor drivers on Windows
The vendor drivers can be installed on Windows by downloading and installing the InstaCal package:
https://www.mccdaq.com/daq-software/instacal.aspx.

InstaCal can be used to set the IP address of Ethernet modules, and to test basic functions.
However, it should not be used to configure the devices, because the EPICS drivers override the
InstaCal settings.

## Installing the vendor drivers on Linux
The vendor drivers can be installed on Linux by following their instructions on Github. 
The libusb 1.0 development package must be installed.
On Ubuntu 18 that package is called libusb-1.0-0-dev. On RHEL 7 it is called libusbx-devel.

The following steps can be used to install the Measurement Computing uldaq SDK from the tar file on
Github if the user has sudo privilege to write to /usr/local.

```
wget -N https://github.com/mccdaq/uldaq/releases/download/v1.2.1/libuldaq-1.2.1.tar.bz2
tar -xvjf libuldaq-1.2.1.tar.bz2
cd libuldaq-1.2.1
./configure
make -sj
sudo make install
```

If the user does not have privilege to write to /usr/local/ then do the following:
- Change the `.configure` command to `./configure --prefix=/home/epics`. `/home/epics` can be changed to 
  any directory where the user has write permission.
- Edit configure/CONFIG_SITE to uncomment and edit the lines that define ULDAQ_INCLUDE and ULDAQ_DIR.

Alternatively the following can be used to clone and build the repository.  This will allow tracking
any local changes. Again, if the user does not have privilege to write to /usr/local follow the steps above
for the `./configure` command and the CONFIG_SITE file.
```
git clone https://github.com/mccdaq/uldaq
cd uldaq
autoreconf -vif
./configure
make -sj
sudo make install
```

If installing from a non-privileged account `make install` will fail near the end when it tries to install
the udev file 50-uldaq.rules.  This must be done with root privilege in order to use USB devices.

## Additional information:
* [Documentation](https://epics-modules.github.io/measComp)
* [Release notes](RELEASE.md)
