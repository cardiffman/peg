#ifndef INCLUDED_PARSERBASE_H
#define INCLUDED_PARSERBASE_H

#include <utility>
#include <memory>
class ParseState;
class ParseResult;
typedef std::shared_ptr<ParseState> ParseStatePtr;
typedef std::shared_ptr<ParseResult> ParseResultPtr;

/**
* Parsers derive from this.
*/
class ParserBase
{
public:
  ParserBase() : parser_id(parser_id_gen++) {}
  virtual ~ParserBase() {}
  ParseResultPtr operator()(const ParseStatePtr& start);
  virtual ParseResultPtr parse(const ParseStatePtr& start) = 0;
  int parser_id;
  static int parser_id_gen;
};
typedef std::shared_ptr<ParserBase> ParserPtr;

#endif // INCLUDED_PARSERBASE_H
