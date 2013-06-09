#ifndef INCLUDED_PARSESTATE_H
#define INCLUDED_PARSESTATE_H
#include <string>
#include <utility>
#include <memory>
#include <map>

class ParseResult;
class ParseState;
typedef std::shared_ptr<ParseState> ParseStatePtr;
typedef std::shared_ptr<ParseResult> ParseResultPtr;
class ParseState
{
public:
	ParseState(std::string& input, size_t index=0);
	ParseStatePtr from(size_t advance);
	std::string substr(int offset)
	{
	  if (offset+index < input.size())
		return input.substr(offset+index);
	  return "";
	}
	std::string substr(int offset, size_t length)
	{
	  if (offset+index < input.size())
		return input.substr(offset+index, length);
	  return "";
	}
	int at(size_t offset)
	{
	  if (offset+index < input.size())
		return input[offset+index];
	  return 0;
	}
	size_t length()
	{
		return input.size()-index;
	}
	friend std::ostream& operator<<(std::ostream& os, ParseState&);

	std::pair<bool,ParseResultPtr> GetCached(int id);
	void PutCached(int id, ParseResultPtr c) {
	  memos[std::make_pair(index, id)] = c;
	}

	static void reset() { memos = Memos(); }
private:
	size_t index;
	std::string& input;
	typedef std::map< std::pair<size_t,int>, ParseResultPtr > Memos;
	static Memos memos;
};
#endif // INCLUDED_PARSESTATE_H
