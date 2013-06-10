PROGS = peghs peg_alphanum peg_arithmetic

all: $(PROGS)

test: $(PROGS)
	./peg_alphanum
	./peg_arithmetic
	./peghs

# This works with gcc 4.5.3
CPPFLAGS += -std=c++0x

libpeg.a: Ast.o ParserBase.o ParseState.o peg.o
	ar rcs libpeg.a Ast.o ParserBase.o ParseState.o peg.o

peghs: peghs.o libpeg.a
	g++ -o peghs peghs.o -L. -lpeg

peg_alphanum: peg_alphanum.o libpeg.a
	g++ -o peg_alphanum peg_alphanum.o -L. -lpeg

peg_arithmetic: peg_arithmetic.o libpeg.a
	g++ -o peg_arithmetic peg_arithmetic.o -L. -lpeg

