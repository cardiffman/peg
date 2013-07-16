peg
===

Parsing Experiments on github

Or

Parsing Expression Grammar

This project is mainly targetted to parsing Haskell programs. To do so it uses
a parsing expression grammar. The grammar is written using variables which 
represent either terminal or non-terminal symbols, combined into expressions
that represent the derivation of further non-terminals including the start
symbol. The operators in the expressions represent how the symbols combine to
derive the non-terminal.

In this project, rules create objects derived from `ParserBase`. These objects 
should be held in `shared_ptr<ParserBase>` variables. Having such an object, to
use it to parse, one creates a `shared_ptr<ParseState>` from a string and passes
it to the `operator()` of the `ParserBase` object.

Thus one combines the `ParserBase` objects using an expression that generates a 
more complex `ParserBase` object which represents the non-terminal.

The combinators, or means of combining the objects in this project are:

* && -- The sequence operator
* || -- The choice operator
* `Rloop` -- A loop in which a pair of symbols alternates to make a list. One symbol, such as a comma, acts as the connector.
* `Rloop2` -- A form of `Rloop` in which the connector is expected to appear at the end.
* `Repeat0` -- Zero or more repetitions of one symbol
* `Repeat` -- One or more repetitions of one symbol
* `Optional` -- Zero or one appearances of one symbol

The simplest built-in `ParserBase` object is `class tch<int>` which recognizes one 
character whose value is that of the integer. Fixed sequences of characters can
be recognized with `ttoken<const char match[]>`. So 

    auto space = make_shared<tch<' '>>(); // recognize one space
    auto tab = make_shared<tch<'\t'>>(); // recognize one tab
    auto whitespace = Repeat0(space || tab); // recognize an arbitrary sequence of spaces and tabs.

The peg.h library in this project actually has a template Skipwhite<> that skips 
whitespace and then invokes a parser of its argument's type, so a symbol can be
recognized with white space coming before it. And explicit mention of whitespace
is seldom necessary because all of the combinators except Repeat and Repeat0 skip
whitespace.

Parsers, whether by themselves or combined, return `ParseResult` objects, which 
consist of an `AST` object and a `ParseState` object. The `ParseState` is just where 
parsing should continue from now that the symbol has been recognized, and the 
`AST` is some representation of the parsed symbol. The common AST types are 
`CharAST` from the `tch` template, `StringAST` from the `ttoken` template, and 
`WSequence` from the sequence operator && or the repetition combinators. The 
`ParseState` object also implements a memoization system. The time performance of 
PEG's can be poor without memoization. The `ParserBase` `operator()` which drives 
parsing consults the `ParseResult` cache before parsing a given symbol at a given 
position in the input and returns the previously-computed result if there is 
one. 

In order to keep track of things either in a debugger or a log, it is handy to 
be able to attach names to parsers. `ParserBase`'s constructor takes a name 
parameter for this reason. Most of the built-in parsers either construct a name
from their parameters or accept a name as a parameter. Also, the `ChoicesName` 
function returns a `Choices` object with a name given to it, so that a 
multiple-choice expression is built into that `Choices` object and is given that 
name. Similarly SequenceName can be used to name sequences. This is easier to
understand given an explanation of the sequence and choice operators. The 
choice operator|| is overloaded to take two combinations of operand pairs. One
overload takes two `ParserBase` objects and makes a `Choices` object containing
references to the two `ParserBase` objects. The other takes a `Choices` object as 
the left operand and a `ParserBase` as the right operand and returns the same 
`Choices` object with another `ParserBase` appended to the `Choices`' `ParserBase`
collection. The operator&& is overloaded similarly with `ParserBase` and 
`WSequence` arguments. The W in the name `WSequence` represents that the sequence
has implicit skipping of white space. The `Choices` object created by the 
`ChoicesName` function has no parsers. If used as follows, it helps naturally
apply names within expressions:

	auto operand = ChoicesName("factor") || identifier || literal;
	auto oper = ChoicesName("oper") || make_shared<tch<'+'>> || make_shared<tch<'*'>>;
	auto expression = SequenceName("expression") && operand && Repeat0(oper && operand); 

Occasionally a non-terminal will refer to itself directly or indirectly in a
way that cannot be factored out. The 'ParserReference' object can be used to
close a loop that arises this way. One names a ParserReference variable after
the symbol it shall represent, references it in the right-hand sides of the 
expressions that need to refer to that symbol, and then writes the expression '
that defines the symbol. Then one 'resolves' the reference.

	auto expressionRef = make_shared<ParserReference>();
	auto operand = ChoicesName("factor") || identifier || literal 
					|| (make_shared<tch<'('>>() && expressionRef && make_shared<tch<')'>>())
					;
	auto oper = ChoicesName("oper") || make_shared<tch<'+'>> || make_shared<tch<'*'>>;
	auto expression = SequenceName("expression") && operand && Repeat0(oper && operand); 
	expressionRef->resolve(expression);

The AST objects generated by this generic framework can be printed out and 
understood as a representation of the syntax tree of the input text, but leave 
something to be desired in representing the potential semantics of the 
constructs recognized. This deficiency can be remedied using the `ActionCaller` 
parser and the `Action` combinator. The `Action` combinator takes a 
transformation function and a parser and returns an `ActionCaller` parser which 
will call the transformation function if the parser is successful. This stuff 
is being used in a small way, mostly to log that certain constructs have been 
recognized. Use of it can be made to generate specific AST objects from generic 
ones, and to restructure the AST of a non-terminal based on semantics. For 
example, a proper `ExpressionAST` could be generated from the `SequenceAST` 
that the `expression` parser above would generate:

	class ExpressionAST : public AST { ... };
	ASTPtr Expression(const ASTPtr& ast)
	{
		SequenceAST* sequence = dynamic_cast<SequenceAST*>(ast.get());
		auto newAST = make_shared<ExpressionAST>();
		// work on rearranging sequence into expression
		return newAST;
	}
	...
	auto expression = Action(Expression, 
		SequenceName("expression") && operand && Repeat0(oper && operand));
	...

A brief comparison with implementation possibilities in other libraries and 
languages is in order. The Boost Spirit library aims at the same goal and on
review appears to be more complete and to do more at compile time. The ParSec
library for Haskell lets the recognizers be functions instead of objects and
Haskell offers more flexibility in defining operators AND their precedence AND
their associativity, possibly giving a cleaner notation. If one understands
that the result of my operators and combinators is not the process of parsing
but the specification of how the process will be done, then one begins to see
that this is possible in languages even less advanced than C++. For example in
C the terminals could be recognized with individually-written functions, a few
struct's could be developed that specified how to use combinations of parsers
and the templates and operator overloads are not strictly necessary.

The Haskell-specific code in peghs.cpp contains objects for recognizing the
lexemes of Haskell, including its very specific versions of literals and 
comments. These are used in a first layout pass to determine where to insert 
semicolons and curly braces, and then in a second pass to recognize the 
program. The reference syntax of Chapter 10 of the Haskell 2010 report has been
more-or-less faithfully translated into the PEG framework above, in a series of 
expressions spanning roughly 400 lines, supported by Haskell-specific parsers 
for Haskell lexemes spanning an additional 1000 lines. The resulting parser has 
been tested and found to recognize a small number of Haskell programs. It may 
well recognize even more, but it is too early to make extravagant claims. 

I don't think GHC uses a PEG to parse Haskell. A [report](http://www.dmst.aueb.gr/dds/pubs/jrnl/1993-StrProg-Haskell/html/exp.html)
from the early 90's on
a Haskell implementation project done for a master's degree used a custom lexer
driving a layout layer into a yacc parser and barely managed (as I would 
describe it) to parse Haskell in that LALR(1) framework. I wrote part of a parser in C++ in recursive descent
and the syntax part was looking very tedious. It did recognize a couple of 
Haskell fragments that did not originate with me, but there were a lot of 
productions left to do and some of them didn't look amenable to classic 
recursive descent. 

Why write a Haskell implementation? Why did Frege write Begriffsschrift? I've
done two Scheme implementations, the compiler somewhat less complete than the
interpreter that the compiler ran on. I am ultimately interested in much more 
dynamic programs than compilers. The programs I am interested in are usually 
considered far too dynamic and demanding to be implemented in languages other 
than C or C++, but I want to explore the space at its frontier instead of its 
cozy interior.

This code has been compiled with VS Express 2010 and with GCC 4.5.3.
