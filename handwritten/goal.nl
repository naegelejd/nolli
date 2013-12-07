module intro

import math

struct point {
    int x,y
}

iface shape {

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

    return 0
}
