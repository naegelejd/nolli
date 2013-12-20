module test_ifelse

func int main ()
{
    if 1 + 1 < 3 {
        print("true")
    } else {
        print("false")
    }

    if istrue() dotrue() else dofalse()

    bool isayso = 42
    if isayso == 0
        cry()
    else if isayso <= 41
        weep()
    else if isayso >= 43
        grovel()
    else
        celebrate()

    bool ugly = true
    if ugly {
        if !ugly {
            print("nested ifs and unary expression")
            return 42
        } else if (len("hello") == 5) {
            print("all is well")
            return 0
        } else {
            print("BOOM!")
            abort()
        }
    } else {
        print("not ugly")
        return 0
    }

    return 0
}
