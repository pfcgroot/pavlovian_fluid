# pavlovian_fluid
Interface module for connecting BBraun DianetStar infusion pumps in a psychological experiment setup using E-Prime.

DianetStar contains the interface module (DLL) for communication with the infusion pumps. It's a rather complex setup with a separate thread to keep the communication alive to prevent automatic disconnection after a few seconds. The DLL also requires the smaller IO_RS232 DLL for setting up a serial RS232 connection with the pumps. A compiled and working version is available in the check_pump folder. The C++ code was developed and compiled with Microsoft Visual Studio 2008 for Windows.

There are two E-Prime (version 2) examples to demonstrate usage of the module: mixed_fluids and pavlovian_fluid. A simple test to test the setup with E-Prime is available in DianetStar-EPrime-test. The E-Prime configuration runs by using a sepearate E-Prime package, which is available in DianetStarEPrimePackage.

