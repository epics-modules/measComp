Overview
--------

This package provides EPICS drivers for the some of the USB and Ethernet
I/O modules from Measurement Computing. Currently the USB-CTR04/08, 
and multi-function modules (E-1608, USB-1208LS, USB-1208FS, USB-1608G, 
USB-1608GX-2AO, USB-1808/1808X, USB-231, USB-2408-2AO, E-TC, TC-32, USB-TEMP, USB-TEMP-AI,
and E-DIO24) are supported. 
The multi-function modules support analog input and/or output, temperature input
(USB-2408-2AO, USB-TEMP, USB-TEMP-AI, E-TC, TC-32), digital input/output, 
pulse counters (all but TC-32), and pulse generators (USB-1608G and USB-1608GX-2AO).

Support for other modules is straightforward to add and can be done as the demand arises.

This module is supported on both Windows and Linux, 64-bit and 32-bit.

On Windows it uses the Measurement Computing "Universal Library" (UL),
which is only available on Windows. 

In R4-0 and later it uses the UL for Linux library from Measurement Computing for Linux drivers.
This is an [open-source library available on Github](https://github.com/mccdaq/uldaq). 
The Linux Universal Library API is similar to the Windows UL API, but the functions have different names and different syntax. 

UL for Windows and Linux support most current Measurement Computing models.

In versions prior to R4-0 the Linux support used the [low-level drivers from Warren Jasper](https://github.com/wjasper/Linux_Drivers).
On top of these drivers the module provides a layer that emulates the Windows UL library from Measurement Computing.
The EPICS drivers thus always use the Windows UL API and are identical on Linux and Windows. 
The E-1608, E-TC, E-TC32, E-DIO24, USB-1608G-2AO, USB-CTR08, USB-TEMP, USB-TEMP-AI and USB-31XX models are supported in these versions.
