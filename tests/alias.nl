struct circle {
    int radius
}

struct square {
    int width, height
}

iface shape {
    func int () area
    func int () perimeter
}

main := func int () {
    var circle c
    var square s

    print(c.area())
    print(s.perimeter())

    alias square rectangle
    alias func int () shapeProperty

    var rectangle r
    var shapeProperty p = c.area
}
