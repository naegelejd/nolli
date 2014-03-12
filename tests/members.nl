struct setmeta {
    int nmods
}

struct intset {
    setmeta meta
    [int] numbers
    int size
    func intset (intset s) union
}

main := func int () {
    var intset s, z
    print(s.size)
    print(s.meta.nmods)
    print(s.union(z).size)
    print(s.union(z).meta.nmods)
}
