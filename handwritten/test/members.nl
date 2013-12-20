module test_members

struct setmeta {
    int nmods
}

struct intset {
    setmeta meta
    [int] numbers
    int size
    func intset union(intset s)
}

func int main()
{
    intset s, z
    print(s.size)
    print(s.meta.nmods)
    print(s.union(z).size)
    print(s.union(z).meta.nmods)
}
