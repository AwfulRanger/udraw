# udraw
Use a Bluetooth linked Wiimote's uDraw tablet extension as a mouse or pen.

Supports Linux and Windows 10, but mouse mode may work on older Windows versions as well.

May require a certain kind of Bluetooth adapter, older ones may not work.

# Windows usage
Install CMake and MSBuild and build it manually, or use a Visual Studio that supports loading folders as CMake projects and build it from there.

Connect the Wiimote through Bluetooth as a Windows device (the methods for this may vary) and run the udraw program.

# Linux usage
Install cmake, make (via a build-essential package or otherwise), and the bluez development libraries.

To build it, open a terminal in this folder (containing this README.md and CMakeLists.txt) and run the following commands:

```
mkdir out
cd out
cmake ../
make
```

Once the program is built, ensure Bluetooth is enabled, press both 1 and 2 buttons on the Wiimote at the same time to enter pairing mode, and run the udraw program.

