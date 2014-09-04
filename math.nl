package math {

alias real float

const real pi = 3.141592653589793
const real e = 2.718281828459045

func int (list<int> a) sum {
    var int x
    for y in a {
        x += y
    }
    return x
}

func int (list<int> a) mean {
    s := sum(a)
    return s / a.length()
}


func int (int x, int y) pow {
    return x ^ y
}

func real (real deg) radians {
    return deg * pi / 180
}

func real (real rad) degrees {
    return rad * 180 / pi
}

}
