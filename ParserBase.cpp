#include "ParserBase.h"
#include "ParseState.h"

int ParserBase::parser_id_gen = 0;
ParseResultPtr ParserBase::operator()(const ParseStatePtr& start)
{
	std::pair<bool,ParseResultPtr> cached = start->GetCached(parser_id);
	if (cached.first)
		return cached.second;
	ParseResultPtr result = parse(start);
	start->PutCached(parser_id, result);
	return result;
}
