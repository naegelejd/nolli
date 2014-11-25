package generics {

class set<T> {
    list<T> data

    func (T elem) add {
        if !data.contains(elem) {
            data.append(elem)
        }
    }

    func (T elem) remove {
        data.remove(elem)
    }
}

func<E> E (list<E> numbers) sum {
    var E n
    for x in numbers {
        n += x
    }
    return n
}

alias func<T> T (list<T>) adder

func () main {
    s := new set<int>{[0, 1, 2, 3, 4, 5]}
    const adder total = sum
    print(total(s.data))
}
}
