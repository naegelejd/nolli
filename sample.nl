package experimental

import math
from hell import fire, lightning

interface Shape {
    func int () area
    func (int) scale
}

data Rect {
    int width, height
    str name
    func int () id
}

methods Rect {
    func int () area {
        return width * height
    }

    func (int x) scale {
        $.width *= x
        $.height *= x
    }

    func () private {
        name.reverse()
    }
}

func () rectDemo {
    sh := &Rect{width:14, height:12, "square", func int () { return 42; }}
}


var int a = func int () { return 42; }()

var fn f = func int () { return 42; }

func int ([str] args) main {
    var bool b, f = false
    const char c, d = 'x'
    var int i, x = 3, y=4
    const real r = 3.14
    var str s = "hello",w

    var [int] l = [0, 1, 2, 3, 4]
    l[0] = l[4]

    var {str,int} m = {"hello":5, "all":3}
    m["hello"] = 42

    print(a,b,c)

    var real a = (4.0 + .4) / 3.14
    const int b = (sum(5, 6) - 3) * 2

    var int len = range(l)

    if 1 + 1 < 3 {
        print("true")
    } else if true {
        dowork()
    } else {
        print("false")
    }

    var int i = 3
    while (i > 0) {
        print(i)
        i -= 1
        break
    }

    say := func () { print("temp"); }
    say()

    alias int id
    alias {str,real} str_real_map
    alias func int (str) main_t
    var [main_t] mains = [main, main, main]
    for m in mains {
        main("hello world")
        continue
    }

    var func int ([str]) main2 = main

    dumb := func func int (str) () {
        hi := "hello, world"
        return func int (str s) {
            return len(s) + len(hi)
        }
    }

    sum := func int ([int] l) {
        var int sum
        for x in l {
            sum += x
        }
        return sum
    }

    main2 := cool()
    stupid := dumb()
    stupid("hello, world");
    sum([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);

    # called function literal
    func () {
        var std.file f = open("helloworld.txt")
        print(f.Read(128))
    }()

    return 0
}
