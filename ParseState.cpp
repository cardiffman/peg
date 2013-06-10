#include "ParseState.h"
#include <string>
#include <iostream>
#include <map>
#include <utility>

using std::string;
using std::pair;
using std::cout;
using std::endl;
using std::map;
using std::make_shared;
using std::make_pair;

/*static*/
ParseState::Memos ParseState::memos;

std::ostream& operator<<(std::ostream& os, ParseState& ps)
{
  if (ps.index >= ps.input.size())
	return os << "";
	return os << ps.input.substr(ps.index);
}

ParseState::ParseState(string& input, size_t index)
	: index(index)
	, input(input)
{

}
ParseStatePtr ParseState::from(size_t advance)
{
	return make_shared<ParseState>(input, index+advance);
}
// The cache is addressed by parser_id (which should represent the 
// function/closure/combinator) and by character in the stream
// The character in the stream is the state of this object.
// 
pair<bool,ParseResultPtr> ParseState::GetCached(int id) {
	Memos::const_iterator pm = memos.find(make_pair(index, id));
	if (pm != memos.end())
		return make_pair(true,pm->second);
	return make_pair(false,ParseResultPtr());
}
