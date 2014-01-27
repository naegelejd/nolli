import math, hello, world, goodbye

# struct shape {
#     int area, perimeter
#     str name = "circle"
# }

# iface buffer {
#     func show ()
#     func int read (file)
# }

func int main(str args)
{
    var bool b, f = false
    const char c, d = 'x'
    var int i,x = 3,y=4
    const real r = 3.14
    var str s = "hello",w

    var [int] l = [0, 1, 2, 3, 4]

    var {str,int} m = {"hello":5, "all":3}

    var int len = range(l)

    if 1 + 1 < 3
        print("true")
    else
        print("false")

    var int i = 3
    while (i > 0) {
        print(i)
    }

    func say () { print("temp") }
    temp()

    typedef func int (str) main_t
    var [main_t] mains = [main, main, main]
    for m in mains {
        main("hello world")
    }

    return 0
}
