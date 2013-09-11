module std

char c = 'h';
c = 'z';

int x = 42;

typedef int hash;
hash id = 12345;

real y = 3.14;

list<int> five = [1, 2, 3, 4, 5];

map<str, int> string_lengths;

string_lengths["hello"] = 6;

file out = fopen("output.txt", "w");
out.write("hello world");
out.close();

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
