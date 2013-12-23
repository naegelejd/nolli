nolli
=====

Nolli is a procedural, statically-typed, lightweight programming language, implemented in C.

More information coming soon...

### Build 

You need [cmake](http://www.cmake.org/) to generate the build system for your OS (Makefiles, VS Studio project, etc.)

Unix/MinGW build example:

    mkdir build
    cd build
    cmake ..
    make

This generates the binary `nolli`.

To generate the included source documentation, obtain [doxygen 1.8.3](http://www.doxygen.org), then run `make doc`.
