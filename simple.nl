#!/usr/bin/env nolli

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

    return i / sum(3, 4)
}
