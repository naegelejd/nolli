# A working sample of nolli's syntax */
module std     # all nolli files are members of a module

import math
from math import PI, E

func int main (str args) {
    char c = 'h'
    c = 'z'

    int x = 42

    str s = "hello world"

    # typedefs can be used to create aliases
    typedef int hash
    hash id = 12345

    # reals are double floating-point numbers
    real y = 3.14

    # lists are dynamic arrays
    [int] five = [1, 2, 3, 4, 5]

    # maps store key value pairs
    {str, int} string_lengths = {"hello":8, "five":6}
    string_lengths["hello"] = 6

    file out = fopen("output.txt", "w")
    out.write("hello world")
    out.close()

    if (true) {
        println("true")
    } else if (not true) {
        println("false")
    } else {
        println("idk")
    }

    int i = 0
    while (i < 4) {
        i = i + 1
    }

    until (i > 8) {
        i += 1
    }

    for i in range(5) {
        print(i)
    }

    #l = [1..5]
    real m = mean(l)

    #Vehicle v = Vehicle()
    #assert(v is v)     # `is` determines if instances match

    return 1
}

func int strlen (int i, str s) { return s.len + i }
typedef func int (int i, str s) slen_func_t
slen_func_t strlen2 = strlen

func real mean ([real] rs) {
    return sum(rs) / len(rs)
}

struct Vehicle {
    func honk () { print("honk") }
    func drive (int miles) { print("drove %i\n", miles) }
}

struct Truck {
    func drive (int miles)
    func honk () { print("HONK!!") }
}
