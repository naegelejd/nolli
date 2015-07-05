#!/usr/bin/env nolli

func str (str s) repeat {
    return s;
}

func int (int a, int b) sum {
    return a + b;
}

func real (real a, real b) prod {
    return a * b
}

func int () main {
    cond := true
    var int i
    while cond {
        i += 7
        if (i >= 42) {
            cond = false
        }
    }

    hello := "hello"
    printf("%s: %g\n", repeat(hello), prod(3.14, 4.25));

    return i / sum(3, 4)
}
