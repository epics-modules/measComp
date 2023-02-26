measComp: EPICS Drivers for Measurement Computing Devices
=========================================================

:author: Mark Rivers, University of Chicago

This package provides EPICS drivers for the some of the USB and Ethernet
I/O modules from Measurement Computing. 

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
| mca             | mca record support                                |
+-----------------+---------------------------------------------------+
| scaler          | Scaler record support.                            |
+-----------------+---------------------------------------------------+
| seq             | State notation language sequencer. Used in MCS    |
|                 | mode with USB-CTR08 and for std.                  |
+-----------------+---------------------------------------------------+

The required versions of each of the above modules for a specific
release of measComp can be determined from the
measComp/configure/RELEASE file.

Table of Contents
-----------------

.. toctree::
  :maxdepth: 3
  
  overview
  measCompMultiFunctionDoc
  measCompUSBCTRDoc
