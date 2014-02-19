import math
from hell import fire, lightning

struct Shape {
    int area, perimeter
    str name# = "circle"
    func int () id
}

iface Buffer {
    func () show
    func int (file) read
}

iface Reader {
    func str (int bytes) Read
}

cool := func func int (str) () {
    return main
}

dumb := func func int (str) () {
    hi := "hello, world"
    return func int (str s) {
        return len(s) + len(hi)
    }
}

sum := func int ([int] l) {
    sum = 0
    for x in l {
        sum += x
    }
    return sum
}

var func int ([str]) floater

main := func int ([str] args) {
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

    return 0
}

floater = main
floater(["arg0", "arg1", "arg2"])

var int a = func int () { return 42; }()

var fn f = func int () { return 42; }

var std.file f = open("helloworld.txt")
print(f.Read(128))

# called function literal
func int (str s) {
    print(hello)
}()
