## Import

    import math
    import math.pi
    from math import pi

## Declaration

    var int x
    var int y = 42
    var int x, y, z = 1, 2, 3
    const string name = "joe"

## Assignment

    x = 1
    y[0] = 2 + 3
    x = y[0]
    y = x()
    x[0] = y[0][1]
    y = {"a":b}["a"](c)

    a.b().c[5] = 

## L-Value

lval = ident | lval [...] | lval . lval

## Expression

    1
    "hello"
    [0, 1, 2, 3]
    {"a":b, "c":d}

    !true
    1 + 1
    (5 - 3) * 7

    <Ident Expr>
    func () { return 42 }
    func (str s) { print("hello", s) }()

## Ident Expr

    hello
    fn()
    words[1]
    fns[idx()]()
    names()[2]
    first.last
    first().last[1]
    names[4].fullname().split()[4]


Callables include:

    identifiers         # print()
    container lookups   # names[1]()
    anonymous functions # func () { return 42 }()
    calls               # getCallback()(e)

Members include:

    identifiers         # X.name
    container lookups   # X.names[2]
    calls               # X.who()

Map keys:

    primitives          # {42:42}, m{"hello":5}
    identifers          # {name:"joe"}
    container lookups   # {names[1]:1}
    calls               # {myname():me}

Container indices: everything but anonymous functions

Must parse this crap:

    names[4].fullname().split()[4]

## Function declaration

    func int main ([string] args)
    func real distance (Point p1, p2)

## Anonymous functions (literals)

    return func () {
        print(err.message)
    }

    print(func int (string s) {
        print(s.len) {
    }()

## Function types

    func (Event e) handler

    struct Event {
        func string () handler
    }

    iface Handler {
        func string () handle
    }
