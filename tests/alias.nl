package test

data circle {
    int radius
}

data square {
    int width, height
}

interface shape {
    func int () area
    func int () perimeter
}

func int () main {
    var circle c
    var square s

    print(c.area())
    print(s.perimeter())

    alias square rectangle
    alias func int () shapeProperty

    var rectangle r
    var shapeProperty p = c.area
}
