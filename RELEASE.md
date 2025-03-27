# measComp Release Notes

## Release 4-4 (April XXX, 2025)
  - drvMultiFunction.cpp
    - Added support for the USB-ERB24, a 24-bit relay output module.
    - Moved the code for reading thermocouples and floating point voltages (USB-TEMP-AI) from the readFloat64
      function to the poller, and changed the device support from asynFloat64 to asynFloat64 average.
      This allows these measurements to be averaged like other analog inputs.
      It also allows them to have SCAN=I/O Intr and scan faster than 10 Hz.
    - Changed the behavior of the waveform generator Offset PV when using the internal Pulse waveform.
      Previously if the Offset was 0 the pulse was bi-polar around 0 volts, and the non-pulse part of 
      the waveform had value of Offset + Amplitude/2, which was not intuitive.
      The new behavior is that the non-pulse part of the waveform has the value of Offset.
    - Added a PulseDelay PV, which controls the time of the beginning of the pulse relative to the start of the waveform.
      This is useful when using 2 waveforms, and one wants a time offset between them.
  - drvUSBCTR
    - Changes to allow use of an external gate signal in scaler and MCS modes.
      - Changed the polarity of the gate signals for the counters (C0GT-C7GT).
        Previously counting was enabled with TTL high signal, and disabled with TTL low.
        This has been swapped to allow use of an external TTL high gate signal to disable counting.
      - Changed the polarity of the counter output signals (C0O-C7O).
        Previously these were TTL high when counting and TTL low when not counting.
        This was swapped to match the change to the gate signals and use of an external gate.
      - These changes are backwards compatible with existing hardware, where C0O is wired to C1GT-C7GT,
        and no external gate is used.
      - To use an external gate the following wiring is required.
        - Install an external chip with 4 OR gates (74HC32N or equivalent).
        - Connect the external gate signal to the first input of the OR gate.
        - Connect channel 0 output (C0O) to the second input of the OR gate.
        - Connect the output of the OR gate to all counter gate inputs (C0GT-C7GT).
      - With these changes the external gate will inhibit counting in scaler mode.
        In MCS mode external gate will inhibit counting but will not inhibit channel advance.
    - Fixed a problem with Rising Edge and Falling Edge trigger modes. The wrong enums were being used.

## Release 4-3 (February 10, 2024)
  - drvMultiFunction.cpp
    - Work around bug where the value of numAnalogIn on E-1608 is returned by the vendor UL library is 4 rather than 8.
    - Set the initial value of pollSleepMS to 50.
  - drvUSBCTR.cpp
    - Remove the pollTime argument from the constructor and USBCTRConfig iocsh command.
      This was no longer used since pollSleepMS was added in R4-2.
    - Set the initial value of pollSleepMS to 50.

## Release 4-2 (July 29, 2023)
  - Added support for the following new models:
    - USB-SSR08, which supports 8 solid-state relays.
    - USB-1808 and 1808X.  These support 8 18-bit analog inputs, 2 16-bit analog outputs,
      2 50 MHz pulse-generators, 4 digital I/O bits, 2 counter inputs, and 2 quadrature encoder inputs.
  - Added support for counter device to E-DIO24.substitutions.
  - Added OPI and iocBoot files for the USB-3104.
  - drvUSBCTR.cpp and drvMultiFunction.cpp
    - Added new parameters for modelName, modelNumber, firmwareVersion, ULVersion,
      driverVersion, pollSleepMS, pollTimeMS, lastErrorMessage
    - replacedPulseGenWidth with pulseGenDutyCycle.  
      pulseGenWidth is now calculated in the database from pulseGenFrequency and pulseGenDutyCycle.
    - Changed the poller function to sleep for pollSleepMS rather than hardcoded time of 100 ms.
      The actual poller cycle time is now reported in pollTimeMS.
  - drvMultiFunction.cpp
    - Added a new global mutex to prevent multiple devices from calling the vendor library at the same time.
      This is needed to prevent errors with multiple Ethernet devices in the same IOC on Linux.
    - Changes to support multiple pulse generators (timers) rather than only 1. Needed for USB-1808.
    - Changes to support write-only and read-only digital I/O ports.
    - The analog inputs are now read in the poller thread, rather than in the readInt32 method.
      The device support for analog inputs was changed from asynInt32 to asynInt32Average.
      This means that the device support now averages all of the values read by the poller thread
      between record processing.  For example, if the poller thread is running at 100 Hz and the
      ai record SCAN field is .2 seconds then 20 values will be averaged each time the record processes.
      If the record SCAN field is I/O Intr then every value read in the poller thread will result in
      record processing with no averaging.
  - measCompAnalogOut.template
    - Changed PINI from YES to $(PINI=YES) so the default value can be overridden.
    - Set PHAS=2 on the ao record to ensure that the Range record will process before the ao record.
  - measCompBinaryIn.template
    - Set DESC=$(DESC=) to allow optional passing a description macro value.
  - measCompBinaryOut.template
    - Set DESC=$(DESC=) to allow optional passing a description macro value on the bo and bi RBV records.
    - Set PHAS=2 on the bo record to ensure that the Direction record will process before the bo record.
  - measCompDevice.template
     - New file with records for ModelName, ModelNumber, FirmwareVersion, ULVersion,
       DriverVersion, PollSleepMS, PollTimeMS, LastErrorMessage
  - measCompEncoder.template
     - New file with records for quadrature encoder inputs.
  - measCompLongOut.template
    - Changed PINI from YES to $(PINI=NO) so the default is now NO but can be overridden.
  - measCompPulseGen.template
    - Added DutyCycle record, and calc record logic to convert from DutyCycle to PulseWidth and vice versa.
  - OPI files
    - Add ModelName, ModelNumber, FirmwareVersion, ULVersion, DriverVersion, PollSleepMS, PollTimeMS, LastErrorMessage
      PV at top of each module screen.
  - Documentation
    - Added documentation for the records in the new measCompDevice.template file.
    - Updated the screen shots to show the new measCompDevice embedded screen at the top.
    - Added documentation for the USB-1808 models.
    - Changed the analog input documentation to describe the new pollSleepMS record and asynInt32Average device support.

## Release 4-1 (February 21, 2023)
  - Fixed problems with the E-DIO24 support that caused only bits 1-8 to work correctly,
    bits 9-24 did not work.
  - Added OPI screens for the E-DIO24.
  - Added substitutions files for the USB-3104.
  - Added support for the USB-1208FS-Plus and fixed some issues with the USB-1208FS and USB-1208LS.
    Thanks to Wei Li from Duke for this.
  - Added support for the USB1608HS-2A0. Thanks to Zachary Arthur from CLS for this.
  - Changed to allow the uniqueId for Ethernet devices in MultifunctionConfig() to be an IP DNS name,
    in addition to the previously supported IP address or MAC address.
  - Moved the documentation to [Github Pages](https://epics-modules.github.io/measComp).

## Release 4-0 (June 10, 2022)
  - Changed the Linux support from using the [low-level drivers from Warren Jasper](https://github.com/wjasper/Linux_Drivers)
    to using the "UL for Linux" library (uldaq)) from Measurement Computing.
    This is an [open-source library available on Github](https://github.com/mccdaq/uldaq). 
    The Linux Universal Library API is similar to the Windows Universal Library API, 
    but the functions have different names and different syntax.
  - On Linux the Measurement Computing "UL for Linux" library must be locally installed.
    The instructions for doing this are in the README.md file.
  - The higher-level drivers, drvMultiFunction.cpp and drvUSCTR.cpp were changed to make
    different function calls for Linux and Windows.
  - For USB devices the UNIQUE_ID (serial number) in the startup script must now be
    8 characters long.  This means adding a leading 0 for seven-digit serial numbers.
  - Added open thermocouple detect (OTD) support for models that support it (e.g. USB-2408).
  - Added analog input Rate record to control the sampling rate for models that support it (e.g. USB-2408).
  - Added DRVH and DRVL limits for NumPoints in the waveform digitizer and waveform generator.
  - Added DwellActual records to report the actual dwell time for the waveform digitizer and waveform generator.

## Release 3-1 (May 2, 2022)
  - Added support for the USB-1608G on Linux. Thanks to David Dudley for this.
  - Added support for the USB-3100 series of devices on both Windows and Linux.
    These have 4, 8 or 16 channels of analog output, plus 8 digital I/O and one counter.
  - Added support for configurable analog output ranges in drvMultiFunction.cpp.
    The USB-3100 models are the first supported modules to require this.
  - Added new OPI screens for 4, 8, and 16 channels of analog output and
    analog output configuration.  Added Range to analog output configuration screens.
  - Bug fix for measCompDiscover for Ethernet devices when UNIQUE_ID was an IP address
    rather than a MAC address, and the device was on the local subnet.
    It was creating a duplicate entry in the device table.
    This was harmless, but resulted duplicate values being printed with measCompShowDevices.
  - Bug fix for the TC-32 in the digital input function.
    This could cause stack corruption and crash.
  - Fixed OPI screen errors for TC-32.
  - Added 4 missing analog input channels to E1608_settings.req for autosave.
  - Added Warren Jasper's low-level Linux drivers for the USB-3100 and the USB-1608G.

## Release 3-0 (March 18, 2021)
  - Changed the method used to specify the device in the constructors and the startup script.
    - Previously one specified what device to connect to in the constructor and startup script
      using an integer `boardNum`.
      - On Windows one needed to use the vendor's InstaCal program to assign a `boardNum` to each device,
        and then use that `boardNum` in the startup script. 
      - On Linux one needed an extra `cbAddBoard` command to the startup script which assigned a `boardNum` to a
        device, emulating what InstaCal does on Linux.
    - I recently learned that using the InstaCal board numbers on Windows has a serious limitation:
      only a single measComp IOC application can be running on the computer at once. This is a known
      limitation in the vendor's Universal Library implementation.  However, they do support multiple applications
      running at once if the device numbers are assigned dynamically at run-time using a discovery mechanism,
      rather than using the static numbers assigned with InstCal.
    - The code has therefore been changed to use this discovery mechanism for finding the devices,
      rather than the InstaCal board numbers. Emulation for this discovery mechanism has been added on Linux,
      so that the startup scripts for Windows and Linux can now be identical. Linux no longer uses
      the `cbAddBoard` command.
    - The second argument the configuration commands in the startup script (e.g. `drvMultifunctionConfigure()`)
      is no longer a `boardNum` integer , it is a `uniqueID` string.
      - For USB devices the `uniqueID` is the serial number, which is printed on the device (e.g. `"01F6335A"`).
      - For Ethernet devices the `uniqueID` can either be the MAC address (e.g. `"00:80:2F:24:53:DE"`), 
        or the IP address (e.g. `"10.54.160.216"`).
        The MAC address or IP address can be used for devices on the local subnet,
        while the IP address must be used for devices on other subnets.
    - There is a new iocsh command, `measCompShowDevices`. This will list all Measurement Computing USB devices
      attached to the computer running the IOC, as well as all Measurement Computing Ethernet devices on the subnet.
    - The discovery mechanism on Windows means that the InstaCal configuration for a board is no longer be used
      with EPICS. This required adding EPICS control for the analog input mode (Differential or Single-ended).
      That was previously only configurable with InstaCal. 
      EPICS already had control of the other configuration settings that InstaCal could control.
  - Added support for the USB-TEMP and USB-TEMP-AI models.
    - These support reading temperature with RTD, thermocouple, thermistor, and semiconductor sensors.  
    - The USB-TEMP supports up to 8 temperature inputs.
    - The USB-TEMP-AI supports up to 4 temperature inputs and 4 24-bit voltage inputs.
    - The USB-TEMP and USB-TEMP-AI behave differently from all other Measurement Computing devices.  
      On Windows InstaCal is used to select the temperature sensor type (RTD, thermocouple, etc.)
      and the RTD wiring configuration.  
      Those settings are written into non-volatile memory on the device, and cannot be changed with EPICS.
      However, they **can** be changed with EPICS on Linux, so they are exposed in the OPI screen.
  - E-1608, USB-2408, USB-1208, USB-1608G, USB1608G-2AO, USB-231, USB-2408
    - Added support for AiMode record to select Differential or Single-ended mode.
  - USB-CTR04/08
    - Fix problem with stopping MCS acquisition.  It was calling cbStopBackground() incorrectly.  This problem
      was introduced in R2-5.
  - iocBoot st.cmd and .substitutions files
    - Changed all of the .substitutions files to remove the P (prefix) macro. This makes these files site-independent.
    - The P macro must now be passed in the dbLoadTemplate command in st.cmd.
    - The .substitutions files have been moved to measCompApp/Db, from which they are installed in the measComp/db.
    - This means that most sites can now use the distributed .substitutions files directly, rather than needing to
      edit them in the local iocBoot directory.  The only time this is not true is if sites want to use different
      names for the ai, bi, etc. records rather than simply a different prefix.
  - Converted documentation from HTML to Markdown (this document) and ReST (other documentation) at
    [epics-meascomp.readthedocs.io](https://epics-meascomp.readthedocs.io/en/latest/).
  - Changed the OPI screens to be modular, using subscreens.  This works well because many devices have
    identical functional blocks.  This is much easier to maintain.
  - Removed support for the USB-4303.  This module is obsolete and has not been available for many years.
    The USB-CTR04/08 should be used instead.

## Release 2-6 (December 11, 2020)
  - mcBoard_E-1608.cpp (Linux)
      - Increased the timeout waiting for the module to respond from 1
        second to 3 seconds. This seems to have completely fixed errors
        on the units in the APS Vibration Measurement System. Previously
        there were errors when polling the digital I/O bits and other
        operations, and AInScanStart_E1608 would sometimes require more
        than 5 retries.
      - Bug fix where lock was not taken when it should be.
  - E-1608::AInScanStart_E1608 (Linux)
      - Changed the number of retries from 5 to 50. The APS Vibration
        Measurement System was occasionally failing even after 5
        retries. However, after increasing the timeout from 1 to 3
        seconds (see above) they have observed at most 1 retry, so this
        change may not be necessary.
  - iocBoot
      - Previously each iocBoot/* directory had its own copy of
        save_restore.cmd, and they were all identical. Changed to using
        a single copy in iocBoot/save_restore.cmd.
  - scaler record support
      - The support for the scaler record has been moved from the std
        module to the scaler module. This required changes to a number
        of the files in measComp.

## Release 2-5-1 (November 20, 2020)
  - drvMultiFunction.cpp
      - Fixed bug in waveform digitizer mode, current channel was being
        calculated incorrrectly.
      - Improved poller to reduce the number of error messages when the
        device is not available.
  - E-1608::AInScanStart_E1608 (Linux)
      - Added 5 retries to start waveform acquisition. It appears that
        sometimes the connect() has not actually completed before
        acquisition is started, and the device returns a "not ready"
        status.
      - Improved error messages when acquisition does not start
        correctly.
      - Bug fix for 1-byte message not being sent correctly.
  - mcBoard_E-1608.cpp (Linux)
      - Bug fix where lock was not taken when it should be.

## Release 2-5 (November 9, 2020)
  - Improvements to USB-CTR04/08 support
      - Added support for reading the 8 digital I/O bits in the MCS
        scan. This allows capturing the status of input and output bits
        during a scan. There are now 9 MCS spectra: 8 for the counters
        and 1 for the digital I/O.
      - Added ability to select counters to use in the MCS on an
        individual basis, rather than the first and last counter to use.
        This also applies to the digital I/O word.
      - Added an AbsTimeWF waveform record containing the absolute time
        of each point in waveform digitizer and MCS scans.
      - Added support for cbDaqInScan and cbDaqSetTrigger on Linux.
      - Aded TrigMode record to control the trigger mode of DaqInScan.
      - Fixed bugs on Linux when the number of counters selected was not
        a power of 2. The data could be wrong and it would stop before
        the number of channels to collect was reached.
  - Added support for E-DIO24 module on Linux and Windows .
  - Added autoconverted OPI files for edm, caQtDM, CSS-Boy and
    CSS-Phoebus.
  - Updated the version of the Measurement Computing Windows UL library
    from 6.51 to 6.72.

## Release 2-4 (September 14, 2019)
  - Improvements to USB-CTR04/08 support
      - Added support for Linux.
      - Added Dwell_RBV record to show the actual dwell time which can
        be different from requested.
      - Added Point0Action record to control how the first time point in
        an MCS scan is handled. Choices are "Clear", "No clear", and
        "Skip".
      - Fixed bugs when FirstCounter!=0 or LastCounter!=7.
      - Fixed initialization bug which was causing random crashes at
        startup.
      - Improved update rate at long dwell times.

## Release 2-3 (August 9, 2019)
  - Improved behavior for E-1608 when there are brief network outages.
    If the waveform digitizer was running when the outage occurred it
    would stop and not restart when the network recovered. This has been
    fixed, and tested with the E-1608 on both Linux and Windows.
  - Added Makefile to produce autoconverted OPI files from medm to edm,
    CSS/BOY, and caQtDM. Added/updated the autoconverted files for
    these.

## Release 2-2 (October 4, 2018)
  - Modified Ethernet support on Linux to be tolerant of brief network
    outages. Previously it was not flushing stale input before sending a
    command and reading the response. This could result in a stale
    response being read, rather than the response to the current
    command. If the network cable was briefly disconnected, for example,
    the driver would never recover. Now the driver does recover if the
    cable is briefly disconnected. If the device is not reachable for a
    longer time the OS will close the socket, and the code does not yet
    handle this.
  - Updated to latest version of Warren Jasper's Linux drivers, and
    applied patches. Pushed patches back to his repository where he has
    merged them, including the change to the Ethernet code described
    above.
  - Added support for E-TC32 to measComp library on Linux. Previously it
    was not supported due to a conflict with E-TC.h.
  - Fixed bug in Linux support for E-TC32. It was not passing the
    channel number correctly when reading the temperature.
  - Fixed bug in Linux support for E-1608. It was not releasing mutex
    when it should, error introduced in R2-1.

## Release 2-1-1 (August 29, 2018)
  - Fixed bug in E-1608 driver, it was not creating epicsEvent in
    constructor.

## Release 2-1 (June 28, 2018)
  - Changed from using std::thread and std::mutex to using epicsThread,
    epicsMutex, and epicsEvent from the EPICS libCom library. This
    allows it to build on older compilers, and is easier to understand.
  - Rearranged code directories.
      - Split linuxSrc into cbw_linuxSrc (which contains the cbw
        library emulation for Linux) and LinuxDrivers (which contains
        Warren Jasper's Linux drivers).
      - Moved cbw.h, cbw32.lib, and cbw64.lib from src/ into new
        directory measCompSupport.
  - Fixed problem with uninitialized variables in E-TC and E-TC32
    drivers on Linux. This could lead to failures to connect to the
    devices at iocInit.

## Release 2-0 (March 22, 2018)

  - Added support for Linux. Previously measComp was limited to running
    on Windows because it uses the Measurement Computing Universal
    Library (UL), which is available only on Windows. The Linux support
    is designed as follows:
      - It uses the [low-level Linux drivers from Warren
        Jasper](https://github.com/wjasper/Linux_Drivers).
      - On top of these drivers the module provides a layer that
        emulates the Measurement Computing Windows UL library.
      - The EPICS drivers thus always use the UL API and are identical
        on Linux and Windows.
      - The Linux UL layer is independent of EPICS, and uses std::thread
        and std::mutex to provide the required threading and mutex
        capabilities. These methods require C++11, and so will not build
        with very old compilers. They do build with gcc 4.8.5 (e.g. RHEL
        7/Centos 7), and gcc 4.4.7 (e.g. RHEL 6/Centos 6).
      - Currently only the E-1608, E-TC, and E-TC32 models are supported
        on Linux. Support for other modules is straightforward to add
        and can be done as the need arises.
    R2-0 is backwards compatible with previous releases but was given a
    new major release number because of all the new code that was added
    to support Linux.
  - Added support to drvMultiFunction for the E-TC. This is an Ethernet
    module with 8 thermocouple inputs (type B, E, J, K, N, R, S, T),
    digital I/O, and a counter. Added new medm screen ETC_module.adl
    and new iocBoot directory, iocETC. This device is useful in
    applications where the length limitation of USB and/or the ability
    to move around without an attached computer are important. This
    module has Linux support.
  - Improved the support for the E-1608 to include fast analog input,
    i.e. waveform digitizer support. Also added Linux support for this
    module.
  - Added Linux support for the TC-32 module using Ethernet. This is not
    yet tested.

## Release 1-5 (March 2, 2018)
  - Added support to drvMultiFunction for the E-1608. This is an
    Ethernet module with analog input, analog output, digital I/O, and a
    counter. Added new medm screen E1608_module.adl and new iocBoot
    directory, iocE1608. This device is useful in applications where the
    length limitation of USB and/or the ability to move around without
    an attached computer are important.
  - Renamed 1608GCounter.template to measCompCounter.template, since it
    is a generic file.
  - Changed line endings in all .substitutions files to be Linux.

## Release 1-4 (September 18, 2017)
  - Added support to drvMultiFunction for the USB-1608G. This is similar
    to the previously supported USB-1608GX-2AO except that is is 250 kHz
    vs 500 kHz, and does not have the 2 analog output ports.
  - Added support for new version of USB-1608GX-2AO which is
    functionally the same but has a different board type identifier.
  - Renamed the iocBoot/iocUSB1608G to iocUSB1608G-2AO, and added new
    iocUSB1608 directory for the USB-1608G.
  - Added support to drvMultiFunction for the USB-231, and a new
    iocUSB231 directory.
  - Removed parameter counting and NUM_PARAMS argument to
    asynPortDriver constructor in all drivers. These are not needed in
    asyn R4-32.

## Release 1-3-1 (May 12, 2016)
  - Added support to drvMultiFunction for the USB-1208FS.
  - Fixed problem with measCompTemperatureSetup.adl medm screen for
    TC-32.

## Release 1-3 (May 10, 2016)
  - Added support to drvMultiFunction for the TC-32, which is a
    32-channel thermocouple input module with both USB and Ethernet
    interfaces.
  - Added new iocBoot/iocTC32 directory.
  - Updated from release 6.50 to 6.52 of the Measurement Computing
    Universal Library.

## Release 1-2 (February 11, 2016)
  - Added support for the USB-1208LS and USB-2408-2AO.
  - Renamed the drv1608G driver to drvMultiFunction because it is now
    designed to handle any multi-function model. It currently supports
    the USB-1208LS, USB-2408-2AO, and USB-1608GX-2AO.
  - Updated from release 6.3 to 6.5 of the Measurement Computing
    Universal Library.

## Release 1-1 (June 26, 2014)
  - Added support for the new USB-CTR04/08 counter/timer modules. This
    includes support for the EPICS scaler record (similar to the Joerger
    VSC and SIS3820), and multi-channel scaler mode (similar to the
    SIS3820). This device can replace the SIS3820 in most applications
    for $429, which is less than 10% of the cost of the SIS3820.
  - Updated from release 6.1 to 6.3 of the Measurement Computing
    Universal Library.
  - Added dependency on synApps mca module for USB-CTR08 driver.
  - Added more fields to the measCompAnalogIn_settings.req and
    measCompAnalogOut_settings.req files for save/restore.
  - Added _RBV (readback) fields for pulse generator Period, Frequency,
    Width, and Delay since the requested value may not match the actual
    value due to hardware limitations.
  - Add new demoSrc directory that contains a tutorial/demo on how to
    write a driver using asynPortDriver. The example is based on the
    USB-1608GX-2AO. It includes:
      - 6 versions of the demo driver source code, starting with a very
        simple 132 line driver that just does 2 analog output records,
        and ending with the full driver that is 1255 lines and has
        waveform digitizer and waveform generator support, etc.
      - A new iocMeasCompDemo directory with the various versions of the
        startup script.
      - New medm files (measCompDemoTop.adl, 1608G_V[1,2,3,4,5].adl).
      - New files in the documentation directory, measCompDriverTalk.pdf
        and measCompTutorial.pdf that were presented at an EPICS class
        on writing drivers based on asynPortDriver.
  - Added the Run record to measCompPulseGen_settings.req, so it
    remembers the run/stop state.

## Release 1-0 (Nov. 28, 2011)
  - First release. Has support for USB-4303 and USB-1608GX-2AO.

-----

Suggestions and Comments to:  
[Mark Rivers](mailto:rivers@cars.uchicago.edu) :
(rivers@cars.uchicago.edu)
