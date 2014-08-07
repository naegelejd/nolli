package demo

const shape::Circle C = new shape::Circle {5}

alias shape::Circle CircleType
alias func () impure

var BoxedString bs

class BoxedString {
    str s
}

func int (list<str> args) main {
    if C.area() > 17 {
        return -1;
    }

    return 0;
}
