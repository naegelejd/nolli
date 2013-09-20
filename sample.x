/* A working sample of nolli's syntax */
module std;     // all nolli files are members of a module

char c = 'h';
c = 'z';

int x = 42;
;;
   ;

// typedefs can be used to create aliases
typedef int hash;
hash id = 12345;

real y = 3.14;

list<int> five = [1, 2, 3, 4, 5];

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

func mean (list<real> rs) : real = {
    return sum(rs) / len(rs);
}

class Vehicle = {
    func honk = { print("honk"); }
    func drive (int miles) = { print("drove %i\n", miles); }
}

class Truck (Vehicle) = {
    func honk = { print("HONK!!"); }
}

Vehicle v = Vehicle();
assert(v is v);
