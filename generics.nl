package generics

class set<T> {
    list<T> data

    func (T elem) add {
        if !data.contains(elem) {
            data.append(elem)
        }
    }

    func (T elem) add {
        data.remove(elem)
    }
}

func E (list<E> numbers) sum {
    var E n
    for x in numbers {
        n += x
    }
    return n
}

func () main {
    s := new set<int>{[0, 1, 2, 3, 4, 5]}

    print(sum(s.data))
}
