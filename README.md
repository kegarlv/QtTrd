# QtTrd

Trading app written with Qt5

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

You'll need ZLib, OpenSSL and Qt5 installed in your system.

### Building

The build system is CMake, qmake is unsupported.
Example of the 
```sh
$ git clone https://github.com/kegarlv/QtTrd
$ cd QtTrd
$ mkdir build && cd build
$ cmake ../ -DCMAKE_BUILD_TYPE=Release
$ make
```
