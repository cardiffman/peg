peg
===

Parsing Experiments on github

So technically this thing was started when I hit a wall while writing a Haskell 
parser.

In general what we have here is 

* ParserBase -- anything that recognizes some text.
* ParseState -- A state of parsing.
* ParseResult -- A result of parsing
* AST -- Abstract Syntax Tree node

In this code I use shared_ptr for all of the above except AST currently. So
there are definitely leaks of AST's. The shared_ptr's have typedefs xxxPtr. This
code makes use of make_shared from the Standard Library.

The general pattern is that to get something parsed, there needs to be a
derivative of ParserBase that recognizes it. And some of those classes are best
generated via templates.

Normally in a framework like this one goes down to the character level, and in
this case the templates are there to enable that, such as recognizers of single 
characters, strings in general, and special forms of strings that you might 
normally have a tokenizer provide, such as the parsing done by the ID class or 
the NUM class. Since I seriously am interested in parsing Haskell programs I 
have some classes that are the initial attempts to recognize constructor names,
variable names, and symbols, along with ways of distinguishing variable names or
symbols from reserved words or symbols.

A ParserBase-derived object implements:
ParseResultPtr parse(const ParseStatePtr& start)

The ParseResult has a ParseState that represents consumption of the recognized
text and an AST that to some extent represents the consumed text. E.g. NUM's
result includes an IntegerAST containing the number the text represented.

So you could recognize the word bad by successively using parsers tch<'b'>,
tch<'a'>, tch<'d'>. You could recognize bad0 by additionally recognizing 
trange<'0','9'>. This of course suggests it would be interesting to have a
convenient way to recognize a sequence of successful parses. 

One approach to recognizing sequences in the initial code is to select the
sequence2<P1,P2> template. The parameter types are parser classes. There are
several templates that recognize sequences including sequence3, sequence4 and
sequence5, because the compiler I'm using doesn't do variadic class templates so
well. There is also a set of sequence templates for sequences with white space
which may be found between items in the sequence.

There are also functions called Repeat0, Repeat1, RLoop and RLoop2. These 
return shared_ptr<>'s to parsers that recognize repetitions. Repeat0 
corresponds to the * operator or Kleene star commonly used in parsing, Repeat1 
corresponds to the + operator, RLoop corresponds to p (q p)* for two parsers p 
and q, and RLoop2 corresponds to (p q)+. Repeat0 and Repeat1 do not account for 
white space on their own. RLoop and RLoop2 actually do skip whitespace. So RLoop 
actually does w* p (w* q w* p)* and RLoop2 does (w* p w* q)+ where w represents
some white space.

It's hard to have a grammar without having two ways to recognize one 
non-terminal. In this code, the basic means for this is ordered choice. The 
simplest embodyment of ordered choice is the choice2<P1,P2> template. A choice2
parser invokes the P1 parser and if it succeeds, the result of P1 is the result
of the choice2. Otherwise the result of the choice2 is the result, successful or
otherwise of P2.

In functional programming this idiom of functions returned by functions falls
under the notion of combinators. When a parsing library is devised from 
combinators it is referred to as a parser combinator library.

In the subject area of parsing, a grammar can be written in a language of 
terminal symbols, non-terminal symbols, and operators symbolized with *, +, /, 
?, &; and ! combined into expressions. This formalism is called Parsing 
Expression Grammars which is the real reason this experiment is called Peg. In 
the standard notation for PEG there is no operator that corresponds to 
sequencing two parsers, they are just written in the expression in the order 
they should be recognized in.

Occasionally a form of recursion crops up in a grammar that cannot be resolved
by refactoring the grammar. The parser ParserReference is used in such a case.
In use the ParserReference is constructed, and used as if it was the full 
embodiment of its production. At some point an expression is written that does
embody the production. At that time the ParserReference is resolved with the
result of that expression. For example:

    auto expr = make_shared<ParserReference>();
    auto op = Choice(plus,asterisk,slash,minus);
    auto exprDef = Choice(WSequence(lparen,expr,rparen), RLoop(literal,op));
    expr->resolve(exprDef);
    
This framework readily allows a grammar that says

    Choice(WSequence(nonterm1,nonterm2,nonterm3,nonterm4,nonterm5)
         , WSequence(nonterm1,nonterm2,nonterm4,nonterm6,nonterm7)
         )

The Choice operator is supposed to try parsing the second choice if the first
choice fails, backing up to its original starting point. The performance could
easily be poor. This implementation, like most PEG implementations, assumes that
parsers always return the same results when called from the same position in the
input. Therefore a cache of previously achieved parse results is maintained. In
lazy functional languages the built-in memoization gives this same optimization.
In this implementation there is a map keyed by the parser ID and text position
that gives the ParseResultPtr that was returned, if in fact there has been an 
attempt to use the same parser at the same spot. This would not prevent left-
recursion problems because if a nonterminal parser is left-recursing into itself
it will not have returned its result yet. 

The original committed state of peg.cpp represents a mixture of stages of 
evolution in the implementation of PEG and parser combinators in C++. The 
templates sequence2, wsequence2, and choice2 and their bretheren were my initial 
work. The next stage used the functions Sequence, WSequence, and Choice due to 
C++'s property of more readily implementing variadic templates for functions than 
for classes. The latest stage is the operators "&&" and "||" for sequence and 
choice. The implementation of these operators is based on the classes WSequenceN 
and Choices which are not generated by templates, but instead rely on the fact 
that all parsers in this work are derived from ParserBase.

This code has been compiled with VS Express 2010.
