/* A working sample of nolli's syntax */
module std;     // all nolli files are members of a module

func int (str) main = |args| {
    char c = 'h';
    c = 'z';

    int x = 42;
    ; ;;  // discarded

    str s = "hello world";

    // typedefs can be used to create aliases
    typedef int hash;
    hash id = 12345;

    // reals are double floating-point numbers
    real y = 3.14;

    // lists are dynamic arrays
    list<int> five = [1, 2, 3, 4, 5];

    // maps store key value pairs
    map<str, int> string_lengths = {"hello":"world", "five":6};
    string_lengths["hello"] = 6;

    file out = fopen("output.txt", "w");
    out.write("hello world");
    out.close();

    if (true) {
        println("true");
    } else if (not true) {
        println("false");
    } else {
        println("idk");
    }

    int i = 0;
    while (i < 4) {
        i = i + 1;
    }

    until (i > 8) {
        i += 1;
    }

    for i in range(5) {
        print(i);
    }

    //l = [1..5];
    real m = mean(l);

    Vehicle v = Vehicle();
    assert(v is v);     // `is` determines if instances match

    return 1;
};

func int (int, str) strlen = |i, s| { return s.len + i; };
/* typedef func int (int, str) slen_func_t; */
/* slen_func_t strlen2 = strlen; */

func real (list<real>) mean = |rs| {
    return sum(rs) / len(rs);
};

class Vehicle = {
    func () honk = { print("honk"); };
    func (int) drive = |miles| { print("drove %i\n", miles); };
};

class Truck = {
    func (int) drive = Vehicle.drive;
    func () honk = { print("HONK!!"); };
};
