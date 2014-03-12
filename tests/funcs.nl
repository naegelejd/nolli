main := func int ([str] args) {
    xs := [1, 2, 3, 4, 5, 6, 7, 8, 9]
    print(len(xs))
    print(sum(xs))
    print(mean(xs))
}

sum := func int ([int] xs) {
    s := 0
    for x in xs {
        s += x
    }
    return s
}

length := func int ([int] xs) {
    l := 0
    for x in xs {
        l += 1
    }
    return s
}

mean := func int ([int] xs) {
    return sum(xs) / len(xs)
}
