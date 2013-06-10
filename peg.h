#ifndef INCLUDED_PEG_H
#define INCLUDED_PEG_H

#include "ParserBase.h"
#include "ParseState.h"
#include "ParseResult.h"

#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdlib>

// This is the closest you can get to parameterizing on
// a compile-time string constant.
template <const char match[]> struct ttoken : public ParserBase
{
	ttoken() {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t ml = strlen(match);
		if (start->length() >= ml)
		{
			if (start->substr(0, ml) == match)
			{
				std::cout << "ttoken "<< match << " matched " << start->substr(0) << std::endl;
				return std::make_shared<ParseResult>(start->from(ml), new StringAST(match));
			}
		}
		return ParseResultPtr();
	}
};

// When a character has to be a given character, this object represents
// testing for a character code established at compile time.
template <int c> struct tch : public ParserBase
{
	tch() {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		int ch = start->at(0);
		if (ch == c)
			return std::make_shared<ParseResult>(start->from(1), new StringAST(std::string(1, ch)));
		return ParseResultPtr();
	}
};

// When a character can be from a large set such as a-z this object
// represents testing for a range established at compile time. It is 
// interesting how much this bloats the names of templates that combine
// with this template.
template<int rangeBegin, int rangeEnd> struct trange : public ParserBase
{
	trange() {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		int ch = start->at(0);
		if (ch >= rangeBegin && ch <= rangeEnd)
			return std::make_shared<ParseResult>(start->from(1), new StringAST(std::string(1, ch)));
		return ParseResultPtr();
	}
};

template <typename Parser> ParseResultPtr skipwhite(const ParseStatePtr& start, const std::shared_ptr<Parser>& next)
{
	size_t i;
	for (i=0; i<start->length() && isspace(start->at(i)); ++i)
		;
	ParseStatePtr nonblank = start->from(i);
	return (*next)(nonblank);
}

template <typename Parser> struct Skipwhite : public ParserBase
{
  Skipwhite(const std::shared_ptr<Parser>& next) : next(next) {}
  std::shared_ptr<Parser> next;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	size_t i;
	for (i=0; i<start->length() && isspace(start->at(i)); ++i)
		;
	ParseStatePtr nonblank = start->from(i);
	return (*next)(nonblank);
  }
};
template <typename Parser> std::shared_ptr<Skipwhite<Parser>> SkipWhite(const std::shared_ptr<Parser>& parser)
{
	return std::make_shared<Skipwhite<Parser>>(parser);
}

// This NUM is just the typical integer for now.
struct NUM : public ParserBase
{
  NUM() {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	size_t ic = 0;
	if (start->length() > 0 && isdigit(start->at(0)))
	{
	  ++ic;
	  while (start->length() > ic && isdigit(start->at(ic)))
		++ic;
	  int value = atoi(start->substr(0, ic).c_str());
	  return std::make_shared<ParseResult>(start->from(ic), new IntegerAST(value));
	}
	return ParseResultPtr();
  }
};

// When the current text has to derive a given parser at least once,
// this parser handles it.
struct repeat1 : public ParserBase
{
	repeat1(const std::string& name, const std::shared_ptr<ParserBase>& next) : name(name), next(next) {}
	std::string name;
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		extern bool showFails;
		SequenceAST* ast = new SequenceAST;
		ParseResultPtr rep = (*next)(start);
		if (!rep)
		{
			if (showFails) std::cout << name<< " failed [" << start->substr(0) << "] " << __LINE__ << std::endl;
			return rep;
		}
		ast->append(rep->ast);
		ParseResultPtr rep2 = (*next)(rep->remaining);
		while (rep2)
		{
			ast->append(rep2->ast);
			rep = rep2;
			rep2 = (*next)(rep2->remaining);
		}
		std::cout << name<< ' ' << rep->remaining->substr(0) << std::endl;
		return std::make_shared<ParseResult>(rep->remaining, ast);
	}
	std::shared_ptr<ParserBase> next;
};
inline std::shared_ptr<ParserBase> Repeat1(const std::string& name, const std::shared_ptr<ParserBase>& parser)
{
	return std::shared_ptr<ParserBase>(new repeat1(name, parser));
}

struct repeat0 : public ParserBase
{
  repeat0(const std::shared_ptr<ParserBase>& next) : next(next) {}
  std::shared_ptr<ParserBase> next;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	SequenceAST* ast = new SequenceAST;
	ParseResultPtr rep = (*next)(start);
	if (!rep)
	{
		return std::make_shared<ParseResult>(start, ast);
	}
	ast->append(rep->ast);
	ParseResultPtr rep2 = (*next)(rep->remaining);
	while (rep2)
	{
		ast->append(rep2->ast);
		rep = rep2;
		rep2 = (*next)(rep2->remaining);
	}
	std::cout <<  __FUNCTION__<< ' ' << rep->remaining->substr(0) << std::endl;
	return std::make_shared<ParseResult>(rep->remaining, ast);
  }
};
inline ParserPtr Repeat0(const ParserPtr& parser)
{
	return std::shared_ptr<ParserBase>(new repeat0(parser));
}

/**
* ParserReference allows us to resolve a circular grammer such as the typical
* parenthesized expression grammer using an after-the-fact resolution.
* This method of handling a loop in the grammar is also used by JParsec.
*/
struct ParserReference : public ParserBase
{
	ParserReference(const std::shared_ptr<ParserBase>& res): resolution(res) {}
	ParserReference() : resolution() {}
	void resolve(const std::shared_ptr<ParserBase>& res) { resolution = res; }
	ParseResultPtr parse(const ParseStatePtr& start) {
		if (resolution)
			return resolution->parse(start);
		if (!resolution)
			throw "ParserReference";
		return ParseResultPtr();
	}
	std::shared_ptr<ParserBase> resolution;
};

template <typename Parser> struct optional : public ParserBase
{
	optional(const std::shared_ptr<Parser>& next) : next(next) {}
	std::shared_ptr<Parser> next;
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		extern bool showFails;
		ParseResultPtr rep = (*next)(start);
		if (!rep)
		{
			if (showFails) std::cout << "optional failed [" << start->substr(0) << "] at " << __LINE__ << std::endl;
			SequenceAST* ast = new SequenceAST;
			return std::make_shared<ParseResult>(start, ast);
		}
		return rep;
	}
};
template <typename Parser> std::shared_ptr<optional<Parser>> Optional(const std::shared_ptr<Parser>& parser)
{
	return std::make_shared<optional<Parser>>(parser);
}

struct Choices : public ParserBase
{
	//typedef shared_ptr<ParserBase> ParserPtr;
	Choices(const std::string& name): name(name) {}
	std::string name;
	typedef std::vector<ParserPtr> Parsers;
	Parsers choices;
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		extern bool showFails;
		ParseStatePtr t1 = start;
		ParseResultPtr r;
		Parsers::const_iterator i = choices.begin();
		while ((!r) && i != choices.end())
		{
			ParserPtr parser = *i;
			r = (*parser)(t1);
			++i;
		}
		if (!r)
		{
			if (showFails) std::cout << name << " fail " << t1->substr(0) << ' ' << __LINE__ << std::endl;
			return r;
		}
		std::cout << name <<' '<< r->remaining->substr(0) << std::endl;
		return r;
	}
};

template <typename Parser1, typename Parser2> std::shared_ptr<Choices> operator||(const std::shared_ptr<Parser1>& parser1, const std::shared_ptr<Parser2>& parser2) {
	auto r = std::make_shared<Choices>("||");
	r->choices.push_back(parser1);
	r->choices.push_back(parser2);
	return r;
}
template <typename Parser1> std::shared_ptr<Choices> operator||(const std::shared_ptr<Choices>& c, const std::shared_ptr<Parser1>& parser1) {
	auto r = c;
	r->choices.push_back(parser1);
	return r;
}

inline std::shared_ptr<Choices> operator||(const std::shared_ptr<Choices>& c, const std::string& name) {
	auto r = c;
	r->name = name;
	return r;
}
inline std::shared_ptr<Choices> operator||(const std::string& name, const std::shared_ptr<Choices>& c) {
	auto r = c;
	r->name = name;
	return r;
}
#if 0
template <typename Parser1> std::shared_ptr<Choices> operator||(const string& name, const std::shared_ptr<Parser1>& parser1) {
	auto r = make_shared<Choices>(name);
	r->name = name;
	r->choices.push_back(parser1);
	return r;
}
#endif

class WSequenceN : public ParserBase
{
public:
	WSequenceN(const std::string& name): name(name) {}
	std::string name;
	typedef std::vector<ParserPtr> Parsers;
	Parsers elements;
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		ParseStatePtr t1 = start;
		ParseResultPtr r;
		SequenceAST* ast = 0;
		extern bool showFails;
		Parsers::const_iterator i = elements.begin();
		while (i != elements.end())
		{
			ParserPtr parser = *i;
			r = skipwhite(t1,parser);
			if (!r)
			{
				delete ast;
				if (showFails) std::cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << std::endl;
				return r;
			}
			if (!ast)
				ast = new SequenceAST;
			ast->append(r->ast);
			t1 = r->remaining;
			++i;
		}
		r->ast = ast;
		std::cout << name <<' '<< r->remaining->substr(0) << std::endl;
		return r;
	}
};

template <typename Parser1, typename Parser2> std::shared_ptr<WSequenceN> operator&&(const std::shared_ptr<Parser1>& parser1, const std::shared_ptr<Parser2>& parser2) {
	auto r = std::make_shared<WSequenceN>("&&");
	r->elements.push_back(parser1);
	r->elements.push_back(parser2);
	return r;
}
template <typename Parser1> std::shared_ptr<WSequenceN> operator&&(const std::shared_ptr<WSequenceN> s, const std::shared_ptr<Parser1>& parser1) {
	auto r = s;
	r->elements.push_back(parser1);
	return r;
}


template <typename Item, typename Join, bool endsWithJoin>
struct rLoop : public ParserBase 
{
	rLoop(const std::string& name, const std::shared_ptr<Item>& item, const std::shared_ptr<Join>& join) : name(name), item(item), join(join) {}
	std::string name;
	std::shared_ptr<Item> item;
	std::shared_ptr<Join> join;
	ParseResultPtr parse(const ParseStatePtr& start) {
		extern bool showFails;
		ParseStatePtr t1 = start;
		ParseResultPtr r = skipwhite(t1, item);
		if (!r)
		{
			if (showFails) std::cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << std::endl;
			return r;
		}
		SequenceAST* ast = new SequenceAST;
		while (r)
		{
			ast->append(r->ast);
			t1 = r->remaining;
			ParseResultPtr joinResult = skipwhite(t1, join);
			if (!joinResult && !endsWithJoin)
			{
				// This is fine, the sequence has ended.
				r->ast = ast;
				std::cout << name << ' ' << r->remaining->substr(0) << std::endl;
				return r;
			}
			if (!joinResult && endsWithJoin)
			{
				if (showFails) std::cout << name << ' ' << '[' << t1->substr(0) << "] fail at " << __LINE__ << std::endl;
				return r; // This means something like "a, a, a " happened which is not okay.
			}
			r = joinResult;
			ast->append(r->ast);
			t1 = r->remaining;
			r = skipwhite(t1, item);
			if (!r && !endsWithJoin)
			{
				if (showFails) std::cout << name << ' ' << '[' << t1->substr(0) << "] fail at " << __LINE__ << std::endl;
				return r; // This means something like "a, a, a, " happened which is not okay.
			}
			if (!r && endsWithJoin)
			{
				// Item parse failed, so last success was join.
				// This means something like "a, a, a, " happened which is okay.
				// The ast is complete and already stored in joinResult which is what we return.
				std::cout << name << ' ' << '[' << joinResult->remaining->substr(0) << "] " << __LINE__ << std::endl;
				return joinResult;
			}
		}
		if (showFails) std::cout << name << " failed "<<  __LINE__ <<  std::endl;
		return r;
	}
};
template <typename P1, typename P2>
	std::shared_ptr<rLoop<P1,P2,false>>
		RLoop(const std::string& name, const std::shared_ptr<P1>& p1, const std::shared_ptr<P2>& p2)
	{
		return std::make_shared<rLoop<P1,P2,false>>(name, p1, p2);
	}
template <typename P1, typename P2>
	std::shared_ptr<rLoop<P1,P2,true>>
		RLoop2(const std::string& name, const std::shared_ptr<P1>& p1, const std::shared_ptr<P2>& p2)
	{
		return std::make_shared<rLoop<P1,P2,true>>(name, p1, p2);
	}

// This ID is like many identifiers in many languages but it is supposed
// to represent most closely the HTML or XML id concept.
struct ID : public ParserBase
{
  ID() {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	size_t ic = 0;
	if (start->length() > 0 && isalpha(start->at(0)))
	{
	  ++ic;
	  while (start->length() > ic && isalnum(start->at(ic)))
		++ic;
	  return std::make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
	}
	return ParseResultPtr();
  }
};

void tests();

#endif // INCLUDED_PEG_H
