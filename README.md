# Introduction
A command line tool for starting stopping and saving captures in [QTM](https://www.qualisys.com/software/qualisys-track-manager)

# Running

`qtmc --help`
```
    QTM Control (qtmc) v0.0.2

Description:
    This program provides command line automation control of QTM

Usage:
    qtmc (start|stop [<filename>]|save <filename>) [--host=<hostname or ip>] [--port=<number>] [--password=<text>]

Arguments:
      start                  Creates a new measurement and starts capture.
       stop [<filename>]     Stops a running capture. If <filename> is provided, the capture is saved as <filename>
       save <filename>       Saves a stopped capture as <filename>
     --host=<hostname or ip> QTM RT endpoint address, default=localhost.
     --port=<number>         QTM RT endpoint port, default=22222.
 --password=<text>           Specifies the QTM "take control" password. default=****.
     --help                  Shows this message.

Notice:
    QTM must be started, and a project must be loaded for the RT protocol to be active.
```
# Build + Install
_Requires [CMake](https://cmake.org/download) v3.50 or greater._
```
  mkdir build
  cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<install-path>
  cmake --build . --config Release
  cmake --build . --target=install
  cd ..
```
