import math
from hell import fire, lightning

var int a = 4 + .4
const int b = sum(5, 6)
const int c = x
var {hello, world} m
m = {"hello":world}
print(a,b,c)

if istrue("yep") {
    print("so true")
    var int d = 42
} else if true {
    dowork()
} else {
    print("the end")
}

while (yes()) {
    hangout()
    relax()
}

for hi in hello {
    print(hi)
}

return 42 * 56
typedef int fortytwo
break
continue

print(a.b.c)

var int a = func int () { return 42; }()

var fn f = func int () { return 42; }

func int sum ([int] l) {
    sum = 0
    for x in l {
        sum += x
    }
    return sum
}

struct Point {
    int X
    int Y
}

iface Reader {
    func string Read (int bytes)
}

func int main ([str] args) {
    if len(args) > 1 {
        return 42
    }
    return
}

# Cannot parse this... looks like a function definition but it's
# actually a literal (which is then called)
#
# func int (str s) {
#     print(hello)
# }()

func func int (str) cool () {
    return hello
}
