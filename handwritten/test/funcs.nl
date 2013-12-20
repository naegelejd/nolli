module test_funcs

func noop () { }

func real pie () { return 3.141592 }

func inout (str sentence)
{
    print(sentence)
}

func int sum([int] nums)
{
    int total = 0
    for n in nums {
        total += n
    }
    return total
}

func real mean ([int] nums)
{
    return sum(nums) / len(nums)
}
