#pragma warning(disable: 4503)
#include "peg.h"
#include "ParseState.h"
#include "Ast.h"
#include "Action.h"

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

extern bool showFails, showRemainder;

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

				return make_shared<ParseResult>(start->from(match.length()), std::make_shared<IdentifierAST>(match));
		}
		return ParseResultPtr();
	}
	string match;
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
		return make_shared<ParseResult>(start->from(1), std::make_shared<StringAST>(string(1, ch)));
	return ParseResultPtr();
  }
  int rangeBegin;
  int rangeEnd;
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
		return make_shared<ParseResult>(start->from(1), std::make_shared<StringAST>(string(1, ch)));
	return ParseResultPtr();
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
			return ParseResultPtr();
		if (ch != c)
			return make_shared<ParseResult>(start->from(1), std::make_shared<StringAST>(string(1, ch)));
		return ParseResultPtr();
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
			return ParseResultPtr();
		if (ch != c)
			return make_shared<ParseResult>(start->from(1), std::make_shared<StringAST>(string(1, ch)));
		return ParseResultPtr();
	}
};

ParseResultPtr WSequenceN::parse(const ParseStatePtr& start)
{
	ParseStatePtr t1 = start;
	ParseResultPtr r;
	shared_ptr<SequenceAST> ast;
	Parsers::const_iterator i = elements.begin();
	static int sequence_id = 0;
	int sequence = ++sequence_id;
	while (i != elements.end())
	{
		ParserPtr parser = *i;
		r = skipwhite(t1,parser);
		if (!r)
		{
			if (showFails) std::cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << std::endl;
			if (showFails) std::cout << " because [" << parser->name << "] failed." << std::endl;
			return r;
		}
		if (!ast)
			ast = make_shared<SequenceAST>();
		ast->append(r->getAST());
		t1 = r->getState();
		++i;
	}
	r = r->getNewResult(ast);
	if (showRemainder) std::cout << name <<' ' << sequence << " [" << r->getState()->substr(0) << "]" << std::endl;
	return r;
}

ParseResultPtr Choices::parse(const ParseStatePtr& start)
{
	ParseStatePtr t1 = start;
	ParseResultPtr r;
	Parsers::const_iterator i = choices.begin();
	string names;
	while ((!r) && i != choices.end())
	{
		ParserPtr parser = *i;
		names += " [" + parser->name + "]";
		r = (*parser)(t1);
		++i;
	}
	if (!r)
	{
		if (showFails) std::cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << std::endl;
		if (showFails) std::cout << " because " << names << " all failed " << std::endl;
		return r;
	}
	if (showRemainder) std::cout << name << " ["<< r->getState()->substr(0) <<"]" << std::endl;
	return r;
}

ParseResultPtr repeat1::parse(const ParseStatePtr& start)
{
	std::shared_ptr<SequenceAST> ast;
	std::shared_ptr<AST> firstAST;
	ParseResultPtr rep = (*next)(start);
	if (!rep)
	{
		if (showFails) std::cout << name<< " failed [" << start->substr(0) << "] at " << __LINE__ << std::endl;
		return rep;
	}
	//ast->append(rep->getAST());
	firstAST = rep->getAST();
	ParseResultPtr rep2 = (*next)(rep->getState());
	while (rep2)
	{
		if (!ast)
		{
			ast = std::make_shared<SequenceAST>();
			ast->append(firstAST);
		}
		ast->append(rep2->getAST());
		rep = rep2;
		rep2 = (*next)(rep2->getState());
	}
	if (showRemainder) std::cout << name<< " [" << rep->getState()->substr(0) << "]"<< std::endl;
	if (!ast)
		return rep->getNewResult(firstAST);
	return rep->getNewResult(ast);
}

ParseResultPtr repeat0::parse(const ParseStatePtr& start)
{
	shared_ptr<SequenceAST> ast = make_shared<SequenceAST>();
	shared_ptr<AST> firstAST;
	ParseResultPtr rep = (*next)(start);
	if (!rep)
	{
		return std::make_shared<ParseResult>(start, ast);
	}
	firstAST = rep->getAST();
	ParseResultPtr rep2 = (*next)(rep->getState());
	while (rep2)
	{
		if (!ast->size())
			ast->append(firstAST);
		ast->append(rep2->getAST());
		rep = rep2;
		rep2 = (*next)(rep2->getState());
	}
	if (showRemainder) std::cout <<  __FUNCTION__<< ' ' << rep->getState()->substr(0) << std::endl;
	if (ast->size())
		return std::make_shared<ParseResult>(rep->getState(), ast);
	return std::make_shared<ParseResult>(rep->getState(), firstAST);
}

bool showFails = false;
bool showRemainder = false;


void foo()
{
  // Leave this in peg.cpp. It's for compile-time.
  auto kw = make_shared<token>("kw");
  auto kws = Repeat1("kws",kw);
  ParseResultPtr pr = (*kws)(ParseStatePtr()); // Not valid input.
  auto kws0 = Repeat0(kw);
  pr = (*kws0)(ParseStatePtr()); // Not valid input.
  typedef Skipwhite<token> SWToken;
  SWToken swToken(kw);
  pr = swToken(ParseStatePtr()); // Not valid input.
  auto tokens2 = kw && kw;
  pr = (*tokens2)(ParseStatePtr()); // Not valid input.
}

class TestEquationAST : public AST
{
public:
	TestEquationAST(const ASTPtr& lhs, const ASTPtr& rhs)
	: lhs(lhs), rhs(rhs) {}
	std::string to_string() const
	{
		return "Equation: " + lhs->to_string() + " = " + rhs->to_string();
	}
private:
	ASTPtr lhs;
	ASTPtr rhs;
};

ASTPtr TestEquation(const ASTPtr& ast)
{
	SequenceAST* sequence = dynamic_cast<SequenceAST*>(ast.get());
	return make_shared<TestEquationAST>(sequence->at(0), sequence->at(2));
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
	auto equation = make_shared<ActionCaller>(TestEquation, make_shared<WSequenceN>("equation") && lhs && equals && addition);

			
void tests()
{
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
		cout << "*e0 [" << *e0->getState() << ']' << endl;
		cout << "AST " << *e0->getAST();
		cout << endl;
	}
	else
	{
		cout << "e0==null" << endl;
	}
}
