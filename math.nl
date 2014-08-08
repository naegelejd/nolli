package math {

const real pi = 3.141592653589793
const real e = 2.718281828459045

func T (list<T> a) sum {
    var T x
    for y in a {
        x += y
    }
    return x
}

func T (list<T> a) mean {
    s := sum(a)
    return s / a.length()
}


func T (T x, T y) pow {
    return x ^ y
}

func real (real deg) radians {
    return deg * pi / 180
}

func real (real rad) degrees {
    return rad * 180 / pi
}

}
