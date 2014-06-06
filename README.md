nolli
=====

Nolli is:

    - procedural
    - statically-typed
    - lightweight
    - garbage collected
    - compiled
    - fast

Nolli's syntax resembles that of [Go](http://golang.org), which happens to have an *excellent* [language spec](http://golang.org/ref/spec).

Nolli is a hobby language, meaning it has a minimal set of features. This is partly intentional and partly
because the core language is under heavy development.

### Build

You need [cmake](http://www.cmake.org/) to generate the build system for your OS (Makefiles, VS Studio project, etc.)

Unix/MinGW build example:

    mkdir build
    cd build
    cmake ..
    make

This generates the binary `nolli`.

To generate the included source documentation, obtain [doxygen 1.8.3](http://www.doxygen.org), then run `make doc`.
