var str hello = "hello"
const str world = "this is a relatively long string, much longer than the original size of the buffer the lexer uses to tokenize this string"

world := "this string has a\
   \\     newline in it"

a := 123
b := 456789
print("a + b")

# ! can wr!t3 $tuff h3r3
print("hello$")
print(hello)

c := 123.456
d := 0.1

e := 3.14e4
f := 42.25E5
g := 6.789e+11
h := 1.78E-2
i := 42e2
j := 1E14

k := 0xa4
j := 0X56CF
l := 0x4ED

if a == b {
    print("a == b")
} else {
    print("a != b")
}

while i > 0 {
    i -= 1
    print(i)
}

a = (b * c) / (d - e) + f
