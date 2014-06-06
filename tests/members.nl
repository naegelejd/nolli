package test

data setmeta {
    int nmods
}

data intset {
    setmeta meta
    [int] numbers
    int size
    func intset (intset s) union
}

func int () main {
    var intset s, z
    print(s.size)
    print(s.meta.nmods)
    print(s.union(z).size)
    print(s.union(z).meta.nmods)
}
