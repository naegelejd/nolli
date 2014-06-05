refparser: grammar.y lexspec.l
	bison -d -v grammar.y
	flex lexspec.l
	cc *.c -o refparser.exe

clean:
	rm grammar.output refparser.exe *.yy.c *.tab.c *.tab.h
