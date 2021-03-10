.. container::

   .. rubric:: measComp: EPICS Drivers for Measurement Computing Devices
      :name: meascomp-epics-drivers-for-measurement-computing-devices

   .. rubric:: December 11, 2020
      :name: december-11-2020

   .. rubric:: Mark Rivers
      :name: mark-rivers

   .. rubric:: University of Chicago
      :name: university-of-chicago

.. _Introduction:

Introduction
------------

This package provides EPICS drivers for the some of the USB and Ethernet
I/O modules from Measurement Computing. Currently the USB-CTR04/08 and
USB-4303 counter/timer, and multi-function modules (E-1608, USB-1208LS,
USB-1208FS, USB-1608G, USB-1608GX-2A0, USB-231, USB-2408-2A0, E-TC,
TC-32, E-DIO24) are supported. The multi-function modules support analog
input and/or output, temperature input (USB-2408-2AO, E-TC, and TC-32),
digital input/output, pulse counters (all but TC-32), and pulse
generators (USB-1608G and USB-1608GX-2A0) These drivers are documented
separately.

-  `USB-CTR04/08 driver <measCompUSBCTRDoc.html>`__
-  `Multi-function module driver <measCompMultiFunctionDoc.html>`__
-  `USB-4303 driver <measComp4303Doc.html>`__

This module is supported on both Windows and Linux, 64-bit and 32-bit.
On Windows it uses the Measurement Computing "Universal Library" (UL),
which is only available on Windows. On Linux it uses the [low-level
drivers from Warren Jasper](https://github.com/wjasper/Linux_Drivers).
On top of these drivers the module provides a layer that emulates the
Windows UL library from Measurement Computing. The EPICS drivers thus
always use the UL API and are identical on Linux and Windows. Currently
only the E-1608, USB-CTR04/08, E-TC, E-TC32, and E-DIO24 models are
supported on Linux. Support for other modules is straightforward to add
and can be done as the demand arises.

--------------

| Suggestions and Comments to:
| `Mark Rivers <mailto:rivers@cars.uchicago.edu>`__ :
  (rivers@cars.uchicago.edu)
