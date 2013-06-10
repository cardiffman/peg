PROGS = peghs peg_alphanum peg_arithmetic

all: $(PROGS)

CPPFLAGS += -std=c++11

libpeg.a: Ast.o ParserBase.o ParseState.o peg.o

peghs: peghs.o libpeg.a

peg_alphanum: peg_alphanum.o libpeg.a

peg_arithmetic: peg_arithmetic.o libpeg.a

