nolli
=====

Nolli is a procedural, statically-typed, lightweight programming language, implemented in C.

More information coming soon...

### Build 

To generate the necessary scanner and parser, you'll need
[flex (lex)](http://flex.sourceforge.net/) and
[bison (yacc)](http://www.gnu.org/software/bison/)

Both `flex` and `bison` are available via:

- any Linux distribution's package manager
- [Macports](http://www.macports.org/) on OSX
- [GnuWin32](http://gnuwin32.sourceforget.net/) on Windows.

You'll also need [cmake](http://www.cmake.org/) to generate the build system for your OS (Makefiles, VS Studio project, etc.)

Unix/MinGW build example:

    mkdir build
    cd build
    cmake ..
    make

This generates the binary `nolli`.

To generate the included source documentation, obtain [doxygen 1.8.3](http://www.doxygen.org), then run `make doc`.
