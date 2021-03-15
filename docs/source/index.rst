=========================================================
measComp: EPICS Drivers for Measurement Computing Devices
=========================================================

:author: Mark Rivers, University of Chicago

.. contents:: Contents

Introduction
------------

This package provides EPICS drivers for the some of the USB and Ethernet
I/O modules from Measurement Computing. Currently the USB-CTR04/08 and
USB-4303 counter/timer, and multi-function modules (E-1608, USB-1208LS,
USB-1208FS, USB-1608G, USB-1608GX-2A0, USB-231, USB-2408-2A0, E-TC,
TC-32, USB-TEMP, USB-TEMP-AI, and E-DIO24) are supported. 
The multi-function modules support analog
input and/or output, temperature input (USB-2408-2AO, USB-TEMP, USB-TEMP-AI,
E-TC, TC-32), digital input/output, pulse counters (all but TC-32), and pulse
generators (USB-1608G and USB-1608GX-2A0).

This module is supported on both Windows and Linux, 64-bit and 32-bit.
On Windows it uses the Measurement Computing "Universal Library" (UL),
which is only available on Windows. 

On Linux it uses the [low-level drivers from Warren Jasper]
(https://github.com/wjasper/Linux_Drivers).
On top of these drivers the module provides a layer that emulates the
Windows UL library from Measurement Computing. The EPICS drivers thus
always use the UL API and are identical on Linux and Windows. Currently
only the E-1608, USB-CTR04/08, USB-TEMP, USB-TEMP-AI, E-TC, E-TC32,
and E-DIO24 models are supported on Linux. 
Support for other modules is straightforward to add and can be done as the demand arises.

Driver documentation
--------------------

Each driver is documented separately.

- `Multi-function module driver <measCompMultiFunctionDoc.html>`__
- `USB-CTR04/08 driver <measCompUSBCTRDoc.html>`__
- `USB-4303 driver <measComp4303Doc.html>`__

Where to find it
----------------

The software is located in the 
`measComp github repository <https://github.com/epics-modules/measComp>`__.

Required Modules
----------------

+-----------------+---------------------------------------------------+
| Required module | Required for                                      |
+=================+===================================================+
| EPICS base      | Base support                                      |
+-----------------+---------------------------------------------------+
| asyn            | Driver and device support                         |
+-----------------+---------------------------------------------------+
| autosave        | Save/restore support                              |
+-----------------+---------------------------------------------------+
| busy            | Busy record support                               |
+-----------------+---------------------------------------------------+
| mca             | mca record support.                               |
+-----------------+---------------------------------------------------+
| scaler          | Scaler record support.                            |
+-----------------+---------------------------------------------------+
| seq             | State notation language sequencer. Used in MCS    |
|                 | mode with USB-CTR08 and for std.                  |
+-----------------+---------------------------------------------------+

The required versions of each of the above modules for a specific
release of measComp can be determined from the
measComp/configure/RELEASE file.

--------------

| Suggestions and Comments to:
| `Mark Rivers <mailto:rivers@cars.uchicago.edu>`__ :
  (rivers@cars.uchicago.edu)
