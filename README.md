nolli
=====

Nolli is:

    - statically-typed
    - lightweight
    - embeddable
    - JIT-compiled

and may soon be:

    - garbage collected

Nolli's syntax is similar to that of [Go](http://golang.org), which happens to have an *excellent* [language spec](http://golang.org/ref/spec).

Nolli is a hobby language, meaning it has a minimal set of features. This is partly intentional and partly
because the core language is under heavy development.

### Build

You need [cmake](http://www.cmake.org/) to generate the build system for your OS (Makefiles, VS Studio project, etc.) and [llvm](http://www.llvm.org/) for JIT functionality.

Unix/MinGW build example:

    mkdir build
    cd build
    cmake -D LLVM_DIR=<path to LLVMConfig.cmake> ..
    make

This generates the library `libnolli` and a sample compiler binary `nolli`.

To generate the included source documentation, obtain [doxygen 1.8.3](http://www.doxygen.org), then run `make doc`.
