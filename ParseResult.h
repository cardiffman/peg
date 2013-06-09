#ifndef INCLUDED_PARSERESULT_H
#define INCLUDED_PARSERESULT_H

#include "Ast.h"
#include <memory>

class ParseState;
class ParseResult;
typedef std::shared_ptr<ParseState> ParseStatePtr;
typedef std::shared_ptr<ParseResult> ParseResultPtr;

class ParseResult
{
public:
	ParseResult(const ParseStatePtr& remaining, AST* ast)
	: remaining(remaining), ast(ast)
	{

	}
	ParseStatePtr remaining;
	AST* ast;
};

#endif // INCLUDED_PARSERESULT_H
