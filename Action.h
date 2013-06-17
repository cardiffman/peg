#ifndef INCLUDED_ACTION_H
#define INCLUDED_ACTION_H

#include "ParserBase.h"
class AST;

class ActionCaller : public ParserBase
{
public:
	// This order seems natural when you write the declaration
#if 0
	ActionCaller(const ParserPtr& parser, ASTPtr (*action)(const ASTPtr&))
: parser(parser), action(action) {}
#endif
	// This order makes more sense in use.
	ActionCaller(ASTPtr (*action)(const ASTPtr&), const ParserPtr& parser)
	: parser(parser), action(action) {}

	ParseResultPtr parse(const ParseStatePtr& start)
	{
		// Parse with the underlying parser
		// and return the unset pointer if it fails
		// Otherwise let the action function transform
		// the AST.
		ParseResultPtr rep = (*parser)(start);
		if (!rep)
			return rep;
		return rep->getNewResult((*action)(rep->getAST()));
	}
private:
	ParserPtr parser;
	ASTPtr (*action)(const ASTPtr&);
};
#if 1
inline ParserPtr Action(ASTPtr (*action)(const ASTPtr&), const ParserPtr& parser)
		{
	//return make_shared<ActionCaller>(action, parser);
	// Something has gone off the rails here. The above line
	// is the obvious construction of this function.
	// The error given is that make_shared is not declared
	auto p = new ActionCaller(action, parser);
	ParserPtr pb;
	pb.reset(p);
	return pb;
		}
#else
inline ParserPtr Action(ASTPtr (*action)(const ASTPtr&), const ParserPtr& parser)
		{
	return parser;
		}
#endif

#endif
