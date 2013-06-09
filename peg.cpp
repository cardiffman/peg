#pragma warning(disable: 4503)
#include "peg.h"
#include "ParseState.h"
#include "Ast.h"

#include <iostream>
#include <cstdlib>
#include <string>
#include <cctype>
//#include <tr1/functional>
#include <vector>
#include <map>
#include <sstream>


using std::cout;
using std::endl;
using std::string;
//using std::tr1::bind;
//using std::tr1::function;
using std::ostream;
using std::ostringstream;
//using namespace std::tr1::placeholders;
using std::vector;
using std::map;
using std::make_pair;
using std::pair;
using std::shared_ptr;
using std::make_shared;
int parser_id_gen = 0;
/*static*/
ParseState::Memos ParseState::memos;

ostream& operator<<(ostream& os, ParseState& ps)
{
  if (ps.index >= ps.input.size())
	return os << "";
	return os << ps.input.substr(ps.index);
}

ostream& operator<<(ostream& os, const AST& ast)
{
	os << ast.to_string();
	return os;
}

#include "ParseResult.h"

/*
 * Basic function that matches a string at the current state.
 * Not a good way to match keywords as most languages want them.
 */
struct token : public ParserBase
{
  token(const string& match) : match(match) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	if (start->length() >= match.length())
	{
	  if (start->substr(0, match.length()) == match)

		  return make_shared<ParseResult>(start->from(match.length()), new IdentifierAST(match));
	}
	return 0;
  }
  string match;
};

// This ID is like many identifiers in many languages but it is supposed
// to represent most closely the HTML or XML id concept.
struct ID : public ParserBase
{
  ID() {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	size_t ic = 0;
	if (start->length() > 0 && isalpha(start->at(0)))
	{
	  ++ic;
	  while (start->length() > ic && isalnum(start->at(ic)))
		++ic;
	  return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
	}
	return 0;
  }
};
// When a character can be from a large set such as a-z this object
// represents testing for a range established at construction time.
struct range : public ParserBase
{
  range(int rangeBegin, int rangeEnd) : rangeBegin(rangeBegin), rangeEnd(rangeEnd) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	int ch = start->at(0);
	if (ch >= rangeBegin && ch <= rangeEnd)
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
  int rangeBegin;
  int rangeEnd;
};
// When a character can be from a large set such as a-z this object
// represents testing for a range established at compile time. It is 
// interesting how much this bloats the names of templates that combine
// with this template.
template<int rangeBegin, int rangeEnd> struct trange : public ParserBase
{
  trange() {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	int ch = start->at(0);
	if (ch >= rangeBegin && ch <= rangeEnd)
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
};
// When a character has to be a given character, this object represents
// testing for a character code established at runtime.
struct ch : public ParserBase
{
  ch(int c) : c(c) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	int ch = start->at(0);
	if (ch == c)
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
  int c;
};
// When a character must not be a given character, this object represents
// testing for a character code established at runtime.
struct notch : public ParserBase
{
  notch(int c) : c(c) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	int ch = start->at(0);
	if (ch == 0)
	  return 0;
	if (ch != c)
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
  int c;
};
// When a character must not be a given character, this object represents
// testing for a character code established at compile time.
template <int c> struct tnotch : public ParserBase
{
  tnotch() {}
  ParseResult* parse(ParseState* start)
  {
	int ch = start->at(0);
	if (ch == 0)
	  return 0;
	if (ch != c)
		return make_shared<ParseResult>(start->from(1), new AST(string(1, ch)));
	return 0;
  }
};

bool showFails = false;



// When you want a parse's failure to be a good thing.
// This parser does not "consume" input.
template <typename Parser> struct isnt : public ParserBase
{
  isnt(Parser& next) : next(next) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	string ast;
	ParseResultPtr rep = next(start);
	if (!rep)
		return start; // don't advance, but do succeed
	return 0
  }
  Parser& next;
};


void foo()
{
  auto kw = make_shared<token>("kw");
  auto kws = Repeat1("kws",kw);
  ParseResultPtr pr = (*kws)(0);
  auto kws0 = Repeat0(kw);
  pr = (*kws0)(0);
  typedef Skipwhite<token> SWToken;
  SWToken swToken(kw);
  pr = swToken(0);
  auto tokens2 = kw && kw;
  pr = (*tokens2)(0);
}

shared_ptr<ParserBase> x()
{
	auto digits = make_shared<trange<'0','9'>>();// digits;
	auto lc = make_shared<trange<'a','z'>>();// lc;
	auto uc = make_shared<trange<'A','Z'>>();// uc;
	auto letters = lc || uc;
	auto p = Repeat0(digits || letters);
	return p;
}

template <typename a> 
Skipwhite<a> whiteskip(a p) 
{ 
	return Skipwhite<a>(p); 
}

shared_ptr<ParserBase> y()
{
	ID anID; NUM aNUM;
	static auto id = make_shared<ID>();// ID id;
	static auto num = make_shared<NUM>();//NUM num;
	static auto operators = make_shared<tch<'+'>>()||make_shared<tch<'*'>>()||make_shared<tch<'/'>>();
	//static Skipwhite<choice2<choice2<tch<'+'>,tch<'*'>>,tch<'/'>>> woperators = 
	static auto rhs = SkipWhite(id || num) && Repeat0(SkipWhite(operators) && (SkipWhite(id || num)));
	static auto p = SkipWhite(id) && Repeat0(SkipWhite(id)) && SkipWhite(make_shared<tch<'='>>()) && rhs;
	return p;
}



	auto idparse = make_shared<ID>();
	auto numparse = make_shared<NUM>();
	auto equals = make_shared<tch<'='>>();
	auto plus = make_shared<tch<'+'>>();
	auto times = make_shared<tch<'*'>>();
	auto leftParen = make_shared<tch<'('>>();
	auto rightParen = make_shared<tch<')'>>();
	auto rhsRef = make_shared<ParserReference>();
	auto parenthesized = (make_shared<WSequenceN>("parenthesized") && leftParen && rhsRef && rightParen);
	// Application of a function is a sequence, starting with an id, of Factor's.
	// E.g. "sqr cos x" does the square of the cosine of x.
	// E.g. hypot o a = sqrt (sqr o + sqr a)
	//      hypot 3 4 -- Computes 5
	// The arity of the functions involved determines the meaning
	// and validity.
	// An id can be treated as a zero-arity function that always returns the same thing.
	// Therefore Application takes the place of ID in some contexts.
	auto factorRef = make_shared<ParserReference>();  // factorRef->resolve(factor)
	auto anArgumentW = SkipWhite<ParserReference>(factorRef);
	auto arguments = Repeat0(anArgumentW);
	auto application = (make_shared<WSequenceN>("application") && idparse && arguments);
	auto factor = (make_shared<Choices>("factor") || application || numparse || parenthesized);
	auto multiplication = RLoop("multiplication",factor,times);
	auto addition = RLoop("addition",multiplication,plus);
	auto lhs = Repeat1("lhs",SkipWhite(idparse));
	auto equation = (make_shared<WSequenceN>("equation") && lhs && equals && addition);

			
void tests()
{
	shared_ptr<ParserBase> p = x();
	string input1("12345ABCDabcd");
	ParseState::reset();
	ParseStatePtr st = make_shared<ParseState>(input1);
	ParseResultPtr r = (*p)(st);
	if (r)
	{
		cout << "r [" << *r->remaining << ']' << endl;
		cout << "AST " << *r->ast;
		cout << endl;
	}
	else
	{
		cout << "r==null" << endl;
	}
	string input1b("x = a + b + c * 9 + 10 * d");
	ParseState::reset();
	ParseStatePtr st1b = make_shared<ParseState>(input1b);
	r = (*y())(st1b);
	if (r)
	{
		cout << "r [" << *r->remaining << ']' << endl;
		cout << "AST " << *r->ast;
		cout << endl;
	}
	else
	{
		cout << "r==null" << endl;
	}
	string input1c("hypot o a = o * o + a * a");
	ParseState::reset();
	ParseStatePtr st1c = make_shared<ParseState>(input1c);
	r = (*y())(st1c);

	if (r)
	{
		cout << "r [" << *r->remaining << ']' << endl;
		cout << "AST " << *r->ast;
		cout << endl;
	}
	else
	{
		cout << "r==null" << endl;
	}
	//typedef trange<32, 0x1FFFF> AnyChar;
	//AnyChar anyChar;
	//typedef repeat0<AnyChar> Anything;
	//Anything anything(anyChar);
	auto anything = Repeat0(make_shared<trange<32,0x1FFFF>>());


	string input2("circumference r = 2 * pi * r");

	/*
equation : lhs '=' rhs
     rhs : term addl
rhs : term
addl : '+' term
term : factor mult
term : factor
mult : '*' factor
factor : application
factor : num
factor : '(' rhs ')'
application : id factor*
lhs : id*
*/
	//return 0;
	ParseState::reset();

	ParseStatePtr doc1=make_shared<ParseState>(input2);

	rhsRef->resolve(addition);
	factorRef->resolve(factor);
	

	ParseResultPtr e0 = (*equation)(doc1);
	//ParseResult* e0 = docparse(&doc1);
	if (e0)
	{
		cout << "*e0 [" << *e0->remaining << ']' << endl;
		cout << "AST " << *e0->ast;
		cout << endl;
	}
	else
	{
		cout << "e0==null" << endl;
	}
}