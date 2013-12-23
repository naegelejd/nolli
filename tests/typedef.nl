module test_typedef

struct circle {
    int radius
}

struct square {
    int width, height
}

iface shape {
    func int area()
    func int perimeter()
}

func int main ()
{
    circle c
    square s

    print(c.area())
    print(s.perimeter())

    typedef square rectangle
    typedef func int () shapeProperty

    rectangle r
    shapeProperty p = c.area
}
