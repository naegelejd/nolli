package test

func int ([str] args) main {
    xs := [1, 2, 3, 4, 5, 6, 7, 8, 9]
    print(len(xs))
    print(sum(xs))
    print(mean(xs))
}

func int ([int] xs) sum {
    s := 0
    for x in xs {
        s += x
    }
    return s
}

func int ([int] xs) length {
    l := 0
    for x in xs {
        l += 1
    }
    return s
}

func int ([int] xs) mean {
    return sum(xs) / len(xs)
}
