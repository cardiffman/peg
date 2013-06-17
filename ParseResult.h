#ifndef INCLUDED_PARSERESULT_H
#define INCLUDED_PARSERESULT_H

#include "Ast.h"
#include <memory>

class ParseState;
class ParseResult;
typedef std::shared_ptr<ParseState> ParseStatePtr;
typedef std::shared_ptr<ParseResult> ParseResultPtr;
typedef std::shared_ptr<AST> ASTPtr;

class ParseResult
{
public:
	ParseResult(const ParseStatePtr& remaining, const ASTPtr& ast)
	: remaining(remaining), ast(ast)
	{

	}
	ParseResultPtr getNewResult(const ASTPtr& newAST)
	{
		return std::make_shared<ParseResult>(remaining, newAST);
	}
	ParseStatePtr getState() const { return remaining; }
	ASTPtr getAST() const { return ast; }
private:
	ParseStatePtr remaining;
	ASTPtr ast;
};

#endif // INCLUDED_PARSERESULT_H
