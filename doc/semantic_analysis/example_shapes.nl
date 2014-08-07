package shape

const real pi = 3.141592653589793

const Circle UnitCircle = new Circle {1}

class Circle {
    int radius

    func int () area {
        return pi * radius ^ 2
    }
}

func bool (Circle c) isUnitCircle {
    if c.radius == 1 {
        return true
    } else {
        return false
    }
}
