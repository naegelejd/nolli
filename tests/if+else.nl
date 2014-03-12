main := func int () {
    if 1 + 1 < 3 {
        print("true")
    } else {
        print("false")
    }

    if istrue() {
        dotrue()
    } else {
        dofalse()
    }

    if true { print(42); }

    var bool isayso = 42
    if isayso == 0 {
        cry()
    } else if isayso <= 41 {
        weep()
    } else if isayso >= 43 {
        grovel()
    } else {
        celebrate()
    }

    var bool ugly = true
    if ugly {
        if !ugly {
            print("nested ifs and unary expression")
            # return 42
            #} else if (len("hello") == 5) {
            # print("all is well")
            # return 0
        } else {
            print("BOOM!")
        #     abort()
        }
        print("hello")
    } else {
        print("not ugly")
        return 0
    }

    return 0
}
