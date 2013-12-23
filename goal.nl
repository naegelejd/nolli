module intro

import math, hello, world, goodbye

struct shape {
    int area, perimeter
    str name = "circle"
}

iface buffer {
    show ()
    int read (file)
}

func int main(str args)
{
    bool b, f = false
    char c, d = 'x'
    int i,x = 3,y=4
    real r = 3.14
    str s = "hello",w

    [int] l = [0, 1, 2, 3, 4]

    {str,int} m = {"hello":5, "all":3}

    int len = range(l)

    if 1 + 1 < 3
        print("true")
    else
        print("false")

    int i = 3
    while (i > 0) {
        print(i)
    }

    func say () { print("temp") }
    temp()

    typedef func int (str) main_t
    [main_t] mains = [main, main, main]
    for m in mains {
        main("hello world")
    }

    return 0
}
