#include "peg.h"
#include <memory>
#include <utility>
#include <string>
#include <iostream>

using std::shared_ptr;
using std::make_shared;
using std::string;
using std::cout;
using std::endl;

shared_ptr<ParserBase> arithmetic()
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

int main()
{
	string input1b("x = a + b + c * 9 + 10 * d");
	ParseState::reset();
	ParseStatePtr st1b = make_shared<ParseState>(input1b);
	ParseResultPtr r = (*arithmetic())(st1b);
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
	r = (*arithmetic())(st1c);

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

}