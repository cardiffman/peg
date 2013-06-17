#include "peg.h"
#include <memory>
#include <string>
#include <iostream>

using std::shared_ptr;
using std::make_shared;
using std::string;
using std::cout;
using std::endl;

shared_ptr<ParserBase> alphanum()
{
	auto digits = make_shared<trange<'0','9'>>();// digits;
	auto lc = make_shared<trange<'a','z'>>();// lc;
	auto uc = make_shared<trange<'A','Z'>>();// uc;
	auto letters = lc || uc;
	auto p = Repeat0(digits || letters);
	return p;
}

int main()
{
	shared_ptr<ParserBase> palphanum = alphanum();
	string input1("12345ABCDabcd");
	ParseState::reset();
	ParseStatePtr st = make_shared<ParseState>(input1);
	ParseResultPtr r = (*palphanum)(st);
	if (r)
	{
		cout << "r [" << *r->getState() << ']' << endl;
		cout << "AST " << *r->getAST();
		cout << endl;
	}
	else
	{
		cout << "r==null" << endl;
	}

}
