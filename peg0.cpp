#pragma warning(disable: 4503)
#include <iostream>
#include <cstdlib>
#include <string>
#include <cctype>
//#include <tr1/functional>
#include <vector>
#include <map>
#include <sstream>


using std::cout;
using std::endl;
using std::string;
//using std::tr1::bind;
//using std::tr1::function;
using std::ostream;
using std::ostringstream;
//using namespace std::tr1::placeholders;
using std::vector;
using std::map;
using std::make_pair;
using std::pair;
using std::shared_ptr;
using std::make_shared;
int parser_id_gen = 0;
class ParseResult;
class ParseState;
typedef std::shared_ptr<ParseState> ParseStatePtr;
typedef std::shared_ptr<ParseResult> ParseResultPtr;
class ParseState
{
public:
	ParseState(string& input, size_t index=0)
	: index(index)
	, input(input)
	{

	}
	ParseStatePtr from(size_t advance)
	{
		return make_shared<ParseState>(input, index+advance);
	}
	string substr(int offset)
	{
	  if (offset+index < input.size())
		return input.substr(offset+index);
	  return "";
	}
	string substr(int offset, size_t length)
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
	friend ostream& operator<<(ostream& os, ParseState&);

	// The cache is addressed by parser_id (which should represent the 
	// function/closure/combinator) and by character in the stream
	// The character in the stream is the state of this object.
	// 
	pair<bool,ParseResultPtr> GetCached(int id) {
		Memos::const_iterator pm = memos.find(make_pair(index, id));
		if (pm != memos.end())
			return make_pair(true,pm->second);
		return make_pair(false,ParseResultPtr());
	  //return memos[make_pair(index, id)];
	}
	void PutCached(int id, ParseResultPtr c) {
	  memos[make_pair(index, id)] = c;
	}

private:
	size_t index;
	string& input;
	typedef map< pair<size_t,int>, ParseResultPtr > Memos;
	static Memos memos;
};
/*static*/
ParseState::Memos ParseState::memos;

ostream& operator<<(ostream& os, ParseState& ps)
{
  if (ps.index >= ps.input.size())
	return os << "";
	return os << ps.input.substr(ps.index);
}
class AST
{
public:
	AST() {}
	virtual ~AST() {}
	virtual string to_string() const = 0;
	/*AST(const string& s) : s(s) {}
	string s;
	vector<AST*> ast;*/
	friend ostream& operator<<(ostream& os, const AST& ast);
};
class IntegerAST : public AST
{
public:
	IntegerAST(int value) : value(value) {}
	string to_string() const { ostringstream os; os << value; return os.str(); }
private:
	int value;
};
class IdentifierAST : public AST
{
public:
	IdentifierAST(string name) : name(name) {}
	string to_string() const { return name; }
private:
	string name;
};
class StringAST : public AST
{
public:
	StringAST(string name) : name(name) {}
	string to_string() const { return name; }
private:
	string name;
};
class SequenceAST : public AST
{
public:
	SequenceAST() {}
	void append(AST* ast) { members.push_back(ast); }
	string to_string() const 
	{
		ostringstream os;
		if (members.size())
			os << '[' << members[0]->to_string();
		for (size_t i=1; i<members.size(); ++i)
			os << ' ' << members[i]->to_string();
		os << ']';
		return os.str();
	}
private:
	vector<AST*> members;
};

ostream& operator<<(ostream& os, const AST& ast)
{
	os << ast.to_string();
	return os;
#if 0
  if (ast.s.size())
	return os << ast.s;
  if (ast.ast.size())
  {
	os << *(ast.ast[0]);
	for (size_t i=1; i<ast.ast.size(); ++i)
	  os << ' ' << *(ast.ast[i]);
  }
  return os;
#endif
}

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

/**
* Parsers derive from this.
*/
struct ParserBase
{
  ParserBase() : parser_id(parser_id_gen++) {}
  virtual ~ParserBase() {}
  ParseResultPtr operator()(const ParseStatePtr& start)
#if 0
  {
	  return parseOp(start);
  }
  ParseResultPtr parseOp(const ParseStatePtr& start)
#endif
  {
	pair<bool,ParseResultPtr> cached = start->GetCached(parser_id);
	if (cached.first)
		return cached.second;
	ParseResultPtr result = parse(start);
	start->PutCached(parser_id, result);
	return result;
  }
  virtual ParseResultPtr parse(const ParseStatePtr& start) = 0;
  int parser_id;
};
typedef shared_ptr<ParserBase> ParserPtr;
/*
 * Basic function that matches a string at the current state.
 * Not a good way to match keywords as most languages want them.
 */
struct token : public ParserBase
{
  token(const string& match) : match(match) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	if (start->length() >= match.length())
	{
	  if (start->substr(0, match.length()) == match)

		  return make_shared<ParseResult>(start->from(match.length()), new IdentifierAST(match));
	}
	return 0;
  }
  string match;
};
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
		  cout << "ttoken "<< match << " matched " << start->substr(0) << endl;
		return make_shared<ParseResult>(start->from(ml), new StringAST(match));
	  }
	}
	return 0;
  }
};

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
	  return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
	}
	return 0;
  }
};
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
	  return make_shared<ParseResult>(start->from(ic), new IntegerAST(value));
	}
	return 0;
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
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
};
// Quoted strings in HTML can use either the single quote or
// the double quote. HTML doesn't allow escaping the quoting
// character but an entity &apos; or &quote; can be used.
template <int quote> struct Quoted : public ParserBase
{
  Quoted() {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	size_t ic =0;
	if (start->length() < 2)
	  return 0;
	if (start->at(0) != quote)
	  return 0;
	++ic;
	while (ic < start->length()-1 && start->at(ic) != quote)
	{
	  ++ic;
	}
	if (start->at(ic) != quote)
	  return 0;
	return make_shared<ParseResult>(start->from(ic), new StringAST(start->substr(0, ic)));
  }
};

// When a character can be from a large set such as a-z this object
// represents testing for a range established at construction time.
struct range : public ParserBase
{
  range(int rangeBegin, int rangeEnd) : rangeBegin(rangeBegin), rangeEnd(rangeEnd) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	int ch = start->at(0);
	if (ch >= rangeBegin && ch <= rangeEnd)
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
  int rangeBegin;
  int rangeEnd;
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
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
};
// When a character has to be a given character, this object represents
// testing for a character code established at runtime.
struct ch : public ParserBase
{
  ch(int c) : c(c) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	int ch = start->at(0);
	if (ch == c)
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
  int c;
};
// When a character must not be a given character, this object represents
// testing for a character code established at runtime.
struct notch : public ParserBase
{
  notch(int c) : c(c) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	int ch = start->at(0);
	if (ch == 0)
	  return 0;
	if (ch != c)
		return make_shared<ParseResult>(start->from(1), new StringAST(string(1, ch)));
	return 0;
  }
  int c;
};
// When a character must not be a given character, this object represents
// testing for a character code established at compile time.
template <int c> struct tnotch : public ParserBase
{
  tnotch() {}
  ParseResult* parse(ParseState* start)
  {
	int ch = start->at(0);
	if (ch == 0)
	  return 0;
	if (ch != c)
		return make_shared<ParseResult>(start->from(1), new AST(string(1, ch)));
	return 0;
  }
};

// When the current text has to derive a given parser at least once,
// this parser handles it.
#if 0
template <typename Parser> struct repeat1 : public ParserBase
{
  repeat1(const shared_ptr<Parser>& next) : next(next) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	SequenceAST* ast = new SequenceAST;
	ParseResultPtr rep = (*next)(start);
	if (!rep)
		return rep;
	ast->append(rep->ast);
	ParseResultPtr rep2 = (*next)(rep->remaining);
	while (rep2)
	{
		ast->append(rep2->ast);
		rep = rep2;
		rep2 = (*next)(rep2->remaining);
	}
	cout << rep->remaining->substr(0) << endl;
	return make_shared<ParseResult>(rep->remaining, ast);
  }
  shared_ptr<Parser> next;
};
template <typename Parser> shared_ptr<repeat1<Parser>> Repeat1(const shared_ptr<Parser>& parser)
{
	return make_shared<repeat1<Parser>>(parser);
}
#else
struct repeat1 : public ParserBase
{
  repeat1(const string& name, const shared_ptr<ParserBase>& next) : name(name), next(next) {}
  string name;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	SequenceAST* ast = new SequenceAST;
	ParseResultPtr rep = (*next)(start);
	if (!rep)
	{
		cout << name<< " failed [" << start->substr(0) << "] " << __LINE__ << endl;
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
	cout << name<< ' ' << rep->remaining->substr(0) << endl;
	return make_shared<ParseResult>(rep->remaining, ast);
  }
  shared_ptr<ParserBase> next;
};
shared_ptr<ParserBase> Repeat1(const string& name, const shared_ptr<ParserBase>& parser)
{
	return shared_ptr<ParserBase>(new repeat1(name, parser));
}
#endif
#if 0
template <typename Parser> struct repeat0 : public ParserBase
{
  repeat0(const shared_ptr<Parser>& next) : next(next) {}
  shared_ptr<Parser> next;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	SequenceAST* ast = new SequenceAST;
	ParseResultPtr rep = (*next)(start);
	if (!rep)
	{
		return make_shared<ParseResult>(start, ast);
	}
	ast->append(rep->ast);
	ParseResultPtr rep2 = (*next)(rep->remaining);
	while (rep2)
	{
		ast->append(rep2->ast);
		rep = rep2;
		rep2 = (*next)(rep2->remaining);
	}
	cout <<  __FUNCTION__<< ' ' << rep->remaining->substr(0) << endl;
	return make_shared<ParseResult>(rep->remaining, ast);
  }
};
template <typename Parser> repeat0<Parser> Repeat0(const shared_ptr<Parser>& parser)
{
	return make_shared<repeat0<Parser>>(parser);
}
#else
template <typename Parser> struct repeat0 : public ParserBase
{
  repeat0(const shared_ptr<Parser>& next) : next(next) {}
  shared_ptr<Parser> next;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	SequenceAST* ast = new SequenceAST;
	ParseResultPtr rep = (*next)(start);
	if (!rep)
	{
		return make_shared<ParseResult>(start, ast);
	}
	ast->append(rep->ast);
	ParseResultPtr rep2 = (*next)(rep->remaining);
	while (rep2)
	{
		ast->append(rep2->ast);
		rep = rep2;
		rep2 = (*next)(rep2->remaining);
	}
	cout <<  __FUNCTION__<< ' ' << rep->remaining->substr(0) << endl;
	return make_shared<ParseResult>(rep->remaining, ast);
  }
};
shared_ptr<ParserBase> Repeat0(const shared_ptr<ParserBase>& parser)
{
	//return make_shared<repeat0<ParserBase>>(parser);
	return shared_ptr<ParserBase>(new repeat0<ParserBase>(parser));
}
#endif

template <typename Parser> struct optional : public ParserBase
{
  optional(const shared_ptr<Parser>& next) : next(next) {}
  shared_ptr<Parser> next;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseResultPtr rep = (*next)(start);
	if (!rep)
	{
		cout << "optional failed [" << start->substr(0) << "] at " << __LINE__ << endl;
		SequenceAST* ast = new SequenceAST;
		return make_shared<ParseResult>(start, ast);
	}
	return rep;
  }
};
template <typename Parser> shared_ptr<optional<Parser>> Optional(const shared_ptr<Parser>& parser)
{
	return make_shared<optional<Parser>>(parser);
}

// When you want a parse's failure to be a good thing.
// This parser does not "consume" input.
template <typename Parser> struct isnt : public ParserBase
{
  isnt(Parser& next) : next(next) {}
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	string ast;
	ParseResultPtr rep = next(start);
	if (!rep)
		return start; // don't advance, but do succeed
	return 0
  }
  Parser& next;
};

template <typename Parser> struct Skipwhite : public ParserBase
{
  Skipwhite(const shared_ptr<Parser>& next) : next(next) {}
  shared_ptr<Parser> next;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	size_t i;
	for (i=0; i<start->length() && isspace(start->at(i)); ++i)
		;
	ParseStatePtr nonblank = start->from(i);
	return (*next)(nonblank);
  }
};
template <typename Parser> shared_ptr<Skipwhite<Parser>> SkipWhite(const shared_ptr<Parser>& parser)
{
	return make_shared<Skipwhite<Parser>>(parser);
}

template <typename Parser> ParseResultPtr skipwhite(const ParseStatePtr& start, const shared_ptr<Parser>& next)
{
	size_t i;
	for (i=0; i<start->length() && isspace(start->at(i)); ++i)
		;
	ParseStatePtr nonblank = start->from(i);
	return (*next)(nonblank);
}

template <typename Parser1, typename Parser2> struct choice2 : public ParserBase
{
  choice2(const string& name, const shared_ptr<Parser1>& next1, const shared_ptr<Parser2>& next2) : name(name), next1(next1), next2(next2) {}
  //choice2(Parser1& n1, Parser2& n2) : next1(make_shared<Parser1>(n1)), next2(make_shared<Parser2>(n2)) {}
  string name;
  shared_ptr<Parser1> next1;
  shared_ptr<Parser2> next2;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	Parser1* pn1 = next1.get();
	ParseResultPtr r = (*next1)(t1);
	if (!r)
		r = (*next2)(t1);
	if (!r)
	{
		cout << name << " fail " << t1->substr(0) << ' ' << __LINE__ << endl;
		return r;
	}
	cout << name << " [" << r->remaining->substr(0) << ']' << endl;
	return r;
  }
};

template <typename Parser1, typename Parser2, typename Parser3> struct choice3 : public ParserBase
{
  choice3(const string& name, const shared_ptr<Parser1>& next1, const shared_ptr<Parser2>& next2, const shared_ptr<Parser3>& next3) : name(name), next1(next1), next2(next2), next3(next3) {}
  string name;
  shared_ptr<Parser1> next1;
  shared_ptr<Parser2> next2;
  shared_ptr<Parser3> next3;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = (*next1)(t1);
	if (!r)
		r = (*next2)(t1);
	if (!r)
		r = (*next3)(t1);
	if (!r)
		return r;
	cout << name << ' '<< r->remaining->substr(0) << endl;
	return r;
  }
};

template <typename Parser1, typename Parser2> struct sequence2 : public ParserBase
{
  sequence2(const shared_ptr<Parser1>& next1, const shared_ptr<Parser2>& next2) : next1(next1), next2(next2) {}
  shared_ptr<Parser1> next1;
  shared_ptr<Parser2> next2;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = (*next1)(t1);
	if (!r)
		return r;
	SequenceAST* ast = new SequenceAST;
	ast->append(r->ast);
	t1 = r->remaining;
	r = (*next2)(t1);
	if (!r)
		return r;
	ast->append(r->ast);
	r->ast = ast;
	cout << __FUNCTION__<< ' ' << r->remaining->substr(0) << endl;
	return r;
  }
};

template <typename Parser1, typename Parser2> struct wsequence2 : public ParserBase
{
  wsequence2(const string& name, const shared_ptr<Parser1>& next1, const shared_ptr<Parser2>& next2) : name(name), next1(next1), next2(next2) {}
  string name;
  shared_ptr<Parser1> next1;
  shared_ptr<Parser2> next2;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = skipwhite(t1, next1);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	SequenceAST* ast = new SequenceAST;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next2);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	ast->append(r->ast);
	r->ast = ast;
	cout << name << ' ' << r->remaining->substr(0) << endl;
	return r;
  }
};


template <typename Parser1, typename Parser2, typename Parser3> struct wsequence3 : public ParserBase
{
  wsequence3(const string& name, const shared_ptr<Parser1>& next1, const shared_ptr<Parser2>& next2, const shared_ptr<Parser3>& next3) : name(name), next1(next1), next2(next2), next3(next3) {}
  string name;
  shared_ptr<Parser1> next1;
  shared_ptr<Parser2> next2;
  shared_ptr<Parser3> next3;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r =0;
	r = skipwhite(t1, next1);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	SequenceAST* ast = new SequenceAST;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next2);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next3);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	ast->append(r->ast);
	cout << name << ' ' << r->remaining->substr(0) << endl;
	r->ast = ast;
	return r;
  }
};

template <typename Parser1, typename Parser2, typename Parser3, typename Parser4> struct wsequence4 : public ParserBase
{
	typedef shared_ptr<Parser1> Parser1Ptr;
	typedef shared_ptr<Parser2> Parser2Ptr;
	typedef shared_ptr<Parser3> Parser3Ptr;
	typedef shared_ptr<Parser4> Parser4Ptr;
  wsequence4(const string& name, const Parser1Ptr& next1, const Parser2Ptr& next2, const Parser3Ptr& next3, const Parser4Ptr& next4) 
	: name(name), next1(next1), next2(next2), next3(next3), next4(next4) {}
  string name;
  Parser1Ptr next1;
  Parser2Ptr next2;
  Parser3Ptr next3;
  Parser4Ptr next4;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = skipwhite(t1, next1);
	if (!r)
	{
		if (name=="lambdaabs")
			cout << __FUNCTION__ << ' ';
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	SequenceAST* ast = new SequenceAST;
	ast->append(r->ast);
	if (name=="lambdaabs")
		cout << name << ' ' << *ast << endl;
	t1 = r->remaining;
	r = skipwhite(t1, next2);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	ast->append(r->ast);
	if (name=="lambdaabs")
		cout << name << ' ' << *ast << endl;
	t1 = r->remaining;
	r = skipwhite(t1, next3);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	ast->append(r->ast);
	if (name=="lambdaabs")
		cout << name << ' ' << *ast << endl;
	t1 = r->remaining;
	r = skipwhite(t1, next4);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	ast->append(r->ast);
	if (name=="lambdaabs")
		cout << name << ' ' << *ast << endl;
	r->ast = ast;
	cout << name << ' ' << r->remaining->substr(0) << endl;
	return r;
  }
};

template <typename Parser1, typename Parser2, typename Parser3, typename Parser4, typename Parser5> struct wsequence5 : public ParserBase
{
	typedef shared_ptr<Parser1> Parser1Ptr;
	typedef shared_ptr<Parser2> Parser2Ptr;
	typedef shared_ptr<Parser3> Parser3Ptr;
	typedef shared_ptr<Parser4> Parser4Ptr;
	typedef shared_ptr<Parser5> Parser5Ptr;
  wsequence5(const string& name,const Parser1Ptr& next1, const Parser2Ptr& next2, const Parser3Ptr& next3, const Parser4Ptr& next4, const Parser5Ptr& next5) 
	: name(name),next1(next1), next2(next2), next3(next3), next4(next4), next5(next5) {}
  string name;
  Parser1Ptr next1;
  Parser2Ptr next2;
  Parser3Ptr next3;
  Parser4Ptr next4;
  Parser5Ptr next5;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = skipwhite(t1, next1);
	if (!r)
	  return r;
	SequenceAST* ast = new SequenceAST;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next2);
	if (!r)
	  return r;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next3);
	if (!r)
	  return r;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next4);
	if (!r)
	  return r;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next5);
	if (!r)
	  return r;
	ast->append(r->ast);
	r->ast = ast;
	cout <<  name << " [" << r->remaining->substr(0) << ']' << endl;
	return r;
  }
};
template <typename Parser1, typename Parser2, typename Parser3, typename Parser4, typename Parser5, typename Parser6> struct wsequence6 : public ParserBase
{
	typedef shared_ptr<Parser1> Parser1Ptr;
	typedef shared_ptr<Parser2> Parser2Ptr;
	typedef shared_ptr<Parser3> Parser3Ptr;
	typedef shared_ptr<Parser4> Parser4Ptr;
	typedef shared_ptr<Parser5> Parser5Ptr;
	typedef shared_ptr<Parser5> Parser6Ptr;
  wsequence6(const string& name, const Parser1Ptr& next1, const Parser2Ptr& next2, const Parser3Ptr& next3, const Parser4Ptr& next4, const Parser5Ptr& next5, const Parser6Ptr& next6) 
	: name(name), next1(next1), next2(next2), next3(next3), next4(next4), next5(next5), next6(next6) {}
  string name;
  Parser1Ptr next1;
  Parser2Ptr next2;
  Parser3Ptr next3;
  Parser4Ptr next4;
  Parser5Ptr next5;
  Parser6Ptr next6;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = skipwhite(t1, next1);
	if (!r)
	  return r;
	SequenceAST* ast = new SequenceAST;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next2);
	if (!r)
	  return r;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next3);
	if (!r)
	  return r;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next4);
	if (!r)
	  return r;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next5);
	if (!r)
	  return r;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next6);
	if (!r)
	  return r;
	ast->append(r->ast);
	r->ast = ast;
	cout << name << ' ' << r->remaining->substr(0) << endl;
	return r;
  }
};

template <typename Parser1, typename Parser2, typename Parser3> struct sequence3 : public ParserBase
{
  sequence3(Parser1& next1, Parser2& next2, Parser3& next3) : next1(next1), next2(next2), next3(next3) {}
  Parser1& next1;
  Parser2& next2;
  Parser3& next3;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = next1(t1);
	if (!r)
	  return r;
	SequenceAST* ast = new SequenceAST;
	ast->append(r->ast);
	t1 = r->remaining;
	r = next2(t1);
	if (!r)
	  return r;
	ast->append(r->ast);
	t1 = r->remaining;
	r = next3(t1);
	if (!r)
	  return r;
	ast->append(r->ast);
	r->ast = ast;
	cout << __FUNCTION__<< ' ' << r->remaining->substr(0) << endl;
	return r;
  }
};

template <typename Parser1, typename Parser2, typename Parser3, typename Parser4> struct sequence4 : public ParserBase
{
  sequence4(Parser1& next1, Parser2& next2, Parser3& next3, Parser4& next4) : next1(next1), next2(next2), next3(next3), next4(next4) {}
  Parser1 next1;
  Parser2 next2;
  Parser3 next3;
  Parser4 next4;
  ParseResult* parse(ParseState* start)
  {
	ParseState* t1 = start;
	ParseResult* r =0;
	r = next1(t1);
	if (!r)
	  return r;
	AST* ast = new AST;
	ast->ast.push_back(r->ast);
	t1 = r->remaining;
	r = next2(t1);
	if (!r)
	  return r;
	ast->ast.push_back(r->ast);
	t1 = r->remaining;
	r = next3(t1);
	if (!r)
	  return r;
	ast->ast.push_back(r->ast);
	t1 = r->remaining;
	r = next4(t1);
	if (!r)
	  return r;
	ast->ast.push_back(r->ast);
	r->ast = ast;
	cout << __FUNCTION__<< ' ' << r->remaining->substr(0) << endl;
	return r;
  }
};


template <typename Parser1, typename Parser2, typename Parser3, typename Parser4> struct choice4 : public ParserBase
{
	typedef shared_ptr<Parser1> Parser1Ptr;
	typedef shared_ptr<Parser2> Parser2Ptr;
	typedef shared_ptr<Parser3> Parser3Ptr;
	typedef shared_ptr<Parser4> Parser4Ptr;
  choice4(const string& name, const Parser1Ptr& next1, const Parser2Ptr& next2, const Parser3Ptr& next3, const Parser4Ptr& next4) 
	  : name(name), next1(next1), next2(next2), next3(next3), next4(next4) {}
  string name;
  Parser1Ptr next1;
  Parser2Ptr next2;
  Parser3Ptr next3;
  Parser4Ptr next4;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = (*next1)(t1);
	if (!r)
	  r = (*next2)(t1);
	if (!r)
	  r = (*next3)(t1);
	if (!r)
	  r = (*next4)(t1);
	if (!r)
	{
		cout << name << " fail "<< t1->substr(0) << ' ' << __LINE__ << endl;
		return r;
	}
	cout << name << ' '<< r->remaining->substr(0) << endl;
	return r;
  }
};

#if 1
template <typename Parser1, typename Parser2, typename Parser3, typename Parser4, typename Parser5> struct choice5 : public ParserBase
{
	typedef shared_ptr<Parser1> Parser1Ptr;
	typedef shared_ptr<Parser2> Parser2Ptr;
	typedef shared_ptr<Parser3> Parser3Ptr;
	typedef shared_ptr<Parser4> Parser4Ptr;
	typedef shared_ptr<Parser5> Parser5Ptr;
	choice5(const string& name, const Parser1Ptr& next1, const Parser2Ptr& next2, const Parser3Ptr& next3, const Parser4Ptr& next4, const Parser5Ptr& next5) 
	  : name(name), next1(next1), next2(next2), next3(next3), next4(next4), next5(next5) {}
	string name;
  Parser1Ptr next1;
  Parser2Ptr next2;
  Parser3Ptr next3;
  Parser4Ptr next4;
  Parser5Ptr next5;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = (*next1)(t1);
	if (!r)
	  r = (*next2)(t1);
	if (!r)
	  r = (*next3)(t1);
	if (!r)
	  r = (*next4)(t1);
	if (!r)
	  r = (*next5)(t1);
	if (!r)
	{
		cout << name << " fail " << t1->substr(0) << ' ' << __LINE__ << endl;
		return r;
	}
	cout << name <<' '<< r->remaining->substr(0) << endl;
	return r;
  }
};

struct Choices : public ParserBase
{
	typedef shared_ptr<ParserBase> ParserPtr;
	Choices(const string& name): name(name) {}
	string name;
	typedef vector<ParserPtr> Parsers;
	vector<ParserPtr> choices;
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		ParseStatePtr t1 = start;
		ParseResultPtr r = 0;
		Parsers::const_iterator i = choices.begin();
		while (r == 0 && i != choices.end())
		{
			ParserPtr parser = *i;
			r = (*parser)(t1);
			++i;
		}
		if (!r)
		{
			cout << name << " fail " << t1->substr(0) << ' ' << __LINE__ << endl;
			return r;
		}
		cout << name <<' '<< r->remaining->substr(0) << endl;
		return r;
	}
};

#if 0
template <typename Parser1, typename Parser2> struct wsequence2 : public ParserBase
{
  wsequence2(const string& name, const shared_ptr<Parser1>& next1, const shared_ptr<Parser2>& next2) : name(name), next1(next1), next2(next2) {}
  string name;
  shared_ptr<Parser1> next1;
  shared_ptr<Parser2> next2;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = skipwhite(t1, next1);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	SequenceAST* ast = new SequenceAST;
	ast->append(r->ast);
	t1 = r->remaining;
	r = skipwhite(t1, next2);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
		return r;
	}
	ast->append(r->ast);
	r->ast = ast;
	cout << name << ' ' << r->remaining->substr(0) << endl;
	return r;
  }
};
#endif

struct WSequenceN : public ParserBase
{
	typedef shared_ptr<ParserBase> ParserPtr;
	WSequenceN(const string& name): name(name) {}
	string name;
	typedef vector<ParserPtr> Parsers;
	vector<ParserPtr> elements;
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		ParseStatePtr t1 = start;
		ParseResultPtr r = 0;
		SequenceAST* ast = 0;
		Parsers::const_iterator i = elements.begin();
		while (i != elements.end())
		{
			ParserPtr parser = *i;
			r = skipwhite(t1,parser);
			if (!r)
			{
				delete ast;
				cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
				return r;
			}
			if (!ast)
				ast = new SequenceAST;
			ast->append(r->ast);
			t1 = r->remaining;
			++i;
		}
		r->ast = ast;
		cout << name <<' '<< r->remaining->substr(0) << endl;
		return r;
	}
};

#else
struct choice5 : public ParserBase
{
	choice5(const string& name, const ParserPtr& next1, const ParserPtr& next2, const ParserPtr& next3, const ParserPtr& next4, const ParserPtr& next5) 
	  : name(name), next1(next1), next2(next2), next3(next3), next4(next4), next5(next5) {}
  string name;
  ParserPtr next1;
  ParserPtr next2;
  ParserPtr next3;
  ParserPtr next4;
  ParserPtr next5;
  ParseResultPtr parse(const ParseStatePtr& start)
  {
	ParseStatePtr t1 = start;
	ParseResultPtr r = (*next1)(t1);
	if (!r)
	  r = (*next2)(t1);
	if (!r)
	  r = (*next3)(t1);
	if (!r)
	  r = (*next4)(t1);
	if (!r)
	  r = (*next5)(t1);
	if (!r)
	{
		//cout << name << " fail " << t1->substr(0) << ' ' << __LINE__ << endl;
		return r;
	}
	cout << name << ' '<< r->remaining->substr(0) << endl;
	return r;
  }
};
#endif

#if 1
void foo()
{
  auto kw = make_shared<token>("kw");
  auto kws = Repeat1("kws",kw);
  ParseResultPtr pr = (*kws)(0);
  auto kws0 = Repeat0(kw);
  pr = (*kws0)(0);
  typedef Skipwhite<token> SWToken;
  SWToken swToken(kw);
  pr = swToken(0);
  typedef sequence2<token,token> TokenToken;
  TokenToken tokens2(kw,kw);
  pr = tokens2(0);
}
#endif

#if 1
template <typename Parser1, typename Parser2> shared_ptr<choice2<Parser1,Parser2>> operator||(const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2) {
  return make_shared<choice2<Parser1,Parser2>>("||",parser1,parser2);
}
template <typename Parser1, typename Parser2> shared_ptr<sequence2<Parser1,Parser2>> operator&&(const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2) {
  return make_shared<sequence2<Parser1,Parser2>>(parser1,parser2);
}
template <typename Parser1> repeat0<Parser1> operator*(const shared_ptr<Parser1>& parser1) {
	return repeat0<Parser1>(parser1);
}
#endif
#if 1
shared_ptr<ParserBase> x()
{
  //return trange<'0','9'>() || trange<'a','z'>() || trange<'A','Z'>();
	auto digits = make_shared<trange<'0','9'>>();// digits;
	auto lc = make_shared<trange<'a','z'>>();// lc;
	auto uc = make_shared<trange<'A','Z'>>();// uc;
  //static auto p = *(trange<'0','9'>() || trange<'a','z'>() || trange<'A','Z'>());
	auto p = Repeat0(digits || lc || uc);
  //static choice2<choice2<trange<'0','9'>,trange<'a','z'> >,trange<'A','Z'> > p = digits || lc || uc;
  return p;
}
#endif
template <typename a> 
Skipwhite<a> whiteskip(a p) 
{ 
	return Skipwhite<a>(p); 
}
#if 1
shared_ptr<ParserBase> y()
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
#endif
/**
* ParserReference allows us to resolve a circular grammer such as the typical
* parenthesized expression grammer using an after-the-fact resolution.
* This method of handling a loop in the grammar is also used by JParsec.
*/
struct ParserReference : public ParserBase
{
  ParserReference(shared_ptr<ParserBase> res): resolution(res) {}
  ParserReference() : resolution(0) {}
  void resolve(shared_ptr<ParserBase> res) { resolution = res; }
  ParseResultPtr parse(const ParseStatePtr& start) {
	if (resolution)
	  return resolution->parse(start);
	if (!resolution)
		throw "ParserReference";
	return 0;
  }
  shared_ptr<ParserBase> resolution;
};

struct OldRLoop : public ParserBase 
{
  OldRLoop(shared_ptr<ParserBase> item, shared_ptr<ParserBase> join) : item(item), join(join) {}
  shared_ptr<ParserBase> item;
  shared_ptr<ParserBase> join;
  ParseResultPtr parse(const ParseStatePtr& start) {
	ParseStatePtr t1 = start;
	ParseResultPtr r = skipwhite(t1, item);
	if (!r)
	  return r;
	SequenceAST* ast = new SequenceAST;
	while (r)
	{
	  ast->append(r->ast);
	  t1 = r->remaining;
	  ParseResultPtr joinResult = skipwhite(t1, join);
	  if (!joinResult)
	  {
		// This is fine, the sequence has ended.
		r->ast = ast;
		cout << __FUNCTION__<< ' ' << r->remaining->substr(0) << endl;
		return r;
	  }
	  r = joinResult;
	  ast->append(r->ast);
	  t1 = r->remaining;
	  r = skipwhite(t1, item);
	  if (!r)
	  {
		cout << __FUNCTION__<< " fail " << t1->substr(0) << ' ' << __LINE__<< endl;
		return r; // This means something like "a, a, a, " happened which is not always okay.
	  }
	}
	ast->append(r->ast);
	r->ast = ast;
	cout << __FUNCTION__<< ' ' << r->remaining->substr(0) << endl;
	return r;
  }
};
template <typename Item, typename Join, bool endsWithJoin=false>
	struct rLoop : public ParserBase 
{
  rLoop(const string& name, const shared_ptr<Item>& item, const shared_ptr<Join>& join) : name(name), item(item), join(join) {}
  string name;
  shared_ptr<Item> item;
  shared_ptr<Join> join;
  ParseResultPtr parse(const ParseStatePtr& start) {
	ParseStatePtr t1 = start;
	ParseResultPtr r = skipwhite(t1, item);
	if (!r)
	{
		cout << name << " failed [" << t1->substr(0) << "] at " << __LINE__ << endl;
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
		cout << name << ' ' << r->remaining->substr(0) << endl;
		return r;
	  }
	  if (!joinResult && endsWithJoin)
	  {
		cout << name << ' ' << '[' << t1->substr(0) << "] fail at " << __LINE__ << endl;
		return r; // This means something like "a, a, a " happened which is not okay.
	  }
	  r = joinResult;
	  ast->append(r->ast);
	  t1 = r->remaining;
	  r = skipwhite(t1, item);
	  if (!r && !endsWithJoin)
	  {
		cout << name << ' ' << '[' << t1->substr(0) << "] fail at " << __LINE__ << endl;
		return r; // This means something like "a, a, a, " happened which is not okay.
	  }
	  if (!r && endsWithJoin)
	  {
		  // Item parse failed, so last success was join.
		  // This means something like "a, a, a, " happened which is okay.
		  // The ast is complete and already stored in joinResult which is what we return.
		  cout << name << ' ' << '[' << joinResult->remaining->substr(0) << "] " << __LINE__ << endl;
		  return joinResult;
	  }
	}
	cout << name << " failed "<<  __LINE__ <<  endl;
	return r;
  }
};
#if 0
	template <typename Parser1, typename Parser2> 
		shared_ptr<ParserBase> 
			WSequence(const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2)
	{
		return shared_ptr<ParserBase>(new wsequence2<Parser1,Parser2>(parser1,parser2));
	}
#else
	template <typename Parser1, typename Parser2> 
		shared_ptr<wsequence2<Parser1,Parser2>> 
			WSequence(const string& name, const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2)
	{
		return make_shared<wsequence2<Parser1,Parser2>>(name,parser1,parser2);
	}
#endif
	template <typename P1, typename P2, typename P3> 
		shared_ptr<wsequence3<P1,P2,P3>> 
			WSequence(const string& name, const shared_ptr<P1>& parser1, const shared_ptr<P2>& parser2, const shared_ptr<P3>& parser3)
	{
		return make_shared<wsequence3<P1,P2,P3>>(name,parser1,parser2,parser3);
	}
	template <typename P1, typename P2, typename P3, typename P4> 
		shared_ptr<wsequence4<P1,P2,P3,P4>> 
			WSequence(const string& name, const shared_ptr<P1>& parser1, const shared_ptr<P2>& parser2, const shared_ptr<P3>& parser3, const shared_ptr<P4>& parser4)
	{
		return make_shared<wsequence4<P1,P2,P3,P4>>(name,parser1,parser2,parser3,parser4);
	}
#if 0
	template <typename Parser1, typename Parser2, typename Parser3, typename Parser4, typename Parser5> 
		shared_ptr<wsequence5<Parser1,Parser2,Parser3,Parser4,Parser5>> 
			WSequence(const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2, const shared_ptr<Parser3>& parser3, const shared_ptr<Parser4>& parser4, const shared_ptr<Parser5>& parser5)
	{
		return make_shared<wsequence6<Parser1,Parser2,Parser3,Parser4,Parser5,Parser6>>(name,parser1,parser2,parser3,parser4,parser5);
	}
	template <typename Parser1, typename Parser2, typename Parser3, typename Parser4, typename Parser5, typename Parser6> 
		shared_ptr<wsequence5<Parser1,Parser2,Parser3,Parser4,Parser5,Parser6>> 
			WSequence(const string& name, const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2, const shared_ptr<Parser3>& parser3, const shared_ptr<Parser4>& parser4, const shared_ptr<Parser5>& parser5, const shared_ptr<Parser6>& parser6)
	{
		return make_shared<wsequence6<Parser1,Parser2,Parser3,Parser4,Parser5,Parser6>>(name,parser1,parser2,parser3,parser4,parser5,parser6);
	}
#else
		shared_ptr<wsequence5<ParserBase,ParserBase,ParserBase,ParserBase,ParserBase>> 
			WSequence(const string& name,const ParserPtr& parser1, const ParserPtr& parser2, const ParserPtr& parser3, const ParserPtr& parser4, const ParserPtr& parser5)
	{
		return make_shared<wsequence5<ParserBase,ParserBase,ParserBase,ParserBase,ParserBase>>(name,parser1,parser2,parser3,parser4,parser5);
	}
		shared_ptr<wsequence6<ParserBase,ParserBase,ParserBase,ParserBase,ParserBase,ParserBase>> 
			WSequence(const string& name, const ParserPtr& parser1, const ParserPtr& parser2, const ParserPtr& parser3, const ParserPtr& parser4, const ParserPtr& parser5, const ParserPtr& parser6)
	{
		return make_shared<wsequence6<ParserBase,ParserBase,ParserBase,ParserBase,ParserBase,ParserBase>>(name,parser1,parser2,parser3,parser4,parser5,parser6);
	}
#endif
	template <typename Parser1, typename Parser2> 
		shared_ptr<choice2<Parser1,Parser2>> 
			Choice(const string& name, const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2)
	{
		return make_shared<choice2<Parser1,Parser2>>(name, parser1,parser2);
	}
	template <typename Parser1, typename Parser2, typename Parser3> 
		shared_ptr<choice3<Parser1,Parser2,Parser3>> 
			Choice(const string& name, const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2, const shared_ptr<Parser3>& parser3)
	{
		return make_shared<choice3<Parser1,Parser2,Parser3>>(name, parser1,parser2,parser3);
	}
	template <typename Parser1, typename Parser2, typename Parser3, typename Parser4> 
		shared_ptr<choice4<Parser1,Parser2,Parser3,Parser4>> 
			Choice(const string& name, const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2, const shared_ptr<Parser3>& parser3, const shared_ptr<Parser4>& parser4)
	{
		return make_shared<choice4<Parser1,Parser2,Parser3,Parser4>>(name, parser1,parser2,parser3,parser4);
	}
#if 1
	template <typename Parser1, typename Parser2, typename Parser3, typename Parser4, typename Parser5> 
		shared_ptr<choice5<Parser1,Parser2,Parser3,Parser4,Parser5> >
			Choice(const string& name, const shared_ptr<Parser1>& parser1, const shared_ptr<Parser2>& parser2, const shared_ptr<Parser3>& parser3, const shared_ptr<Parser4>& parser4, const shared_ptr<Parser5>& parser5)
	{
		return make_shared<choice5<Parser1,Parser2,Parser3,Parser4,Parser5>>(name, parser1,parser2,parser3,parser4,parser5);
	}
#else
	ParserPtr
			Choice(const string& name, const ParserPtr& parser1, const ParserPtr& parser2, const ParserPtr& parser3, const ParserPtr& parser4, const ParserPtr& parser5)
	{
		return ParserPtr(new choice5(name, parser1,parser2,parser3,parser4,parser5));
	}
#endif
	template <typename P1, typename P2>
		shared_ptr<rLoop<P1,P2,false>>
			RLoop(const string& name, const shared_ptr<P1>& p1, const shared_ptr<P2>& p2)
		{
			return make_shared<rLoop<P1,P2,false>>(name, p1, p2);
		}
	template <typename P1, typename P2>
		shared_ptr<rLoop<P1,P2,true>>
			RLoop2(const string& name, const shared_ptr<P1>& p1, const shared_ptr<P2>& p2)
		{
			return make_shared<rLoop<P1,P2,true>>(name, p1, p2);
		}

#if 1
	//ID idparse;
		auto idparse = make_shared<ID>();
	//NUM numparse;
		auto numparse = make_shared<NUM>();
	//tch<'='> equals;
		auto equals = make_shared<tch<'='>>();
	//tch<'+'> plus;
	auto plus = make_shared<tch<'+'>>();
	//tch<'*'> times;
	auto times = make_shared<tch<'*'>>();
	//typedef sequence3<tch<'('>, ParserReference, tch<')'> > Parenthesized;
	extern ParserBase* pRhs;
	tch<'('> leftParen; tch<')'> rightParen;
	auto rhsRef = make_shared<ParserReference>();//ParserReference rhsRef;
	//Parenthesized parenthesized(leftParen, rhsRef, rightParen);
	auto parenthesized = WSequence("parenthesized",make_shared<tch<'('>>(),rhsRef,make_shared<tch<')'>>());
	typedef wsequence2<ID, repeat0<Skipwhite<ParserReference> > > Application;
	// Application of a function is a sequence, starting with an id, of Factor's.
	// E.g. "sqr cos x" does the square of the cosine of x.
	// E.g. hypot o a = sqrt (sqr o + sqr a)
	//      hypot 3 4 -- Computes 5
	// The arity of the functions involved determines the meaning
	// and validity.
	// An id can be treated as a zero-arity function that always returns the same thing.
	// Therefore Application takes the place of ID in some contexts.
	auto factorRef = make_shared<ParserReference>();  // factorRef->resolve(factor)
	//typedef Skipwhite<ParserReference> WParserReference; WParserReference anArgumentW(factorRef);
	auto anArgumentW = SkipWhite<ParserReference>(factorRef);
	//typedef repeat0<WParserReference> Arguments0; Arguments0 arguments(anArgumentW);
	auto arguments = Repeat0(anArgumentW);
	//Application application(idparse, arguments);
	auto application = WSequence("application",idparse, arguments);
	//typedef choice3<Application,NUM,Parenthesized> Factor; Factor factor(application,numparse,parenthesized);
	auto factor = Choice("factor",application,numparse,parenthesized);
	auto multiplication = RLoop("multiplication",factor,times);// multiplication(factor, times);
	auto addition = RLoop("addition",multiplication,plus);// addition(multiplication, plus);
	//typedef Skipwhite<ID> WID; WID widparse(idparse);
	//typedef repeat1<WID> Lhs; Lhs lhs(widparse);
	auto lhs = Repeat1("lhs",SkipWhite(idparse));
	//typedef wsequence3<Lhs,tch<'='>, RLoop> Equation; Equation equation(lhs,equals,addition);
	auto equation = WSequence("equation",lhs,equals,addition);
#endif

	bool reservedid(const string& sym)
	{
		return sym=="case"||sym=="class"||sym=="data"||sym=="default"||
			sym=="deriving"||sym=="do"||sym=="else"||sym=="foreign"||
			sym=="if"||sym=="import"||sym=="in"||sym=="infix"||sym=="infixl"||
			sym=="infixr"||sym=="instance"||sym=="let"||sym=="module"||
			sym=="newtype"||sym=="of"||sym=="then"||sym=="type"||sym=="where"||
			sym=="_";
	}
	struct ConID : public ParserBase
	{
		ParseResultPtr parse(const ParseStatePtr& start)
		{
			size_t ic = 0;
			if (start->length() > 0 && isalpha(start->at(0)) && isupper(start->at(0)))
			{
				++ic;
				while (start->length() > ic && (isalnum(start->at(ic)) || start->at(ic)=='.'))
					++ic;
				return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
			}
			return 0;
		}
	};
	struct VarID : public ParserBase
	{
		ParseResultPtr parse(const ParseStatePtr& start)
		{
			size_t ic = 0;
			if (start->length() > 0 && isalpha(start->at(0)) && islower(start->at(0)))
			{
				++ic;
				while (start->length() > ic && (isalnum(start->at(ic))))
					++ic;
				if (reservedid(start->substr(0,ic)))
					return false;
				return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
			}
			return 0;
		}
	};
	bool asciiSymbolChar(int ch) 
	{
		switch (ch) {
		case '!': case '#': case '$': case '%': case '&': case '*': case '+': case '.': case '/': case '<': case '=': case '>': case '?': case '@': case '\\': case '^': case '|': case '-': case '~': case ':': 
			return true;
		}
		return false;
	}
	bool asciiSymbolCharExceptColon(int ch) 
	{
		switch (ch) {
		case '!': case '#': case '$': case '%': case '&': case '*': case '+': case '.': case '/': case '<': case '=': case '>': case '?': case '@': case '\\': case '^': case '|': case '-': case '~': 
			return true;
		}
		return false;
	}
	bool reservedOp(const string& sym)
	{
		/// .. | : | :: | = | \ | | | <- | -> |  @ | ~ | =>
		// length 2 --- .. | :: | <- | -> | =>
		// length 1 --- : | = | \ | | | @ | ~
		switch (sym.length())
		{
		default:
			return false;
		case 1:
			switch (sym[0])
			{
			case ':': case '=': case '\\': case '|': case '@': case '~':
				return true;
			}
			return false;
		case 2:
			if (sym[0]=='.' && sym[1]=='.')
				return true;
			if (sym[0]==':' && sym[1]==':')
				return true;
			if (sym[0]=='<' && sym[1]=='-')
				return true;
			if (sym[1]=='>' && (sym[0]=='-' || sym[0]=='='))
				return true;
			return false;
		}
		return false;
	}
	struct VarSym : public ParserBase
	{
		ParseResultPtr parse(const ParseStatePtr& start)
		{
			size_t ic = 0;
			if (start->length() > 0 && asciiSymbolCharExceptColon(start->at(0)))
			{
				++ic;
				while (start->length() > ic && (asciiSymbolChar(start->at(ic))))
					++ic;
				//cout << "tentative VarSym [" << start->substr(0,ic) << ']' << endl;
				if (reservedOp(start->substr(0,ic)))
				{
					//cout << "reservedop NOT VarSym [" << start->substr(0,ic) << ']' << endl;
					return 0;
				}
				cout << "VarSym [" << start->substr(0,ic) << ']' << endl;
				return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
			}
			return 0;
		}
	}; 
	struct ConSym : public ParserBase
	{
		ParseResultPtr parse(const ParseStatePtr& start)
		{
			size_t ic = 0;
			if (start->length() > 0 && start->at(0)==':')
			{
				++ic;
				while (start->length() > ic && (asciiSymbolChar(start->at(0))))
					++ic;
				if (reservedOp(start->substr(0,ic)))
					return false;
				return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
			}
			return 0;
		}
	}; 
	struct ReservedOp : public ParserBase
	{
		ParseResultPtr parse(const ParseStatePtr& start)
		{
			size_t ic = 0;
			if (start->length() > 0 && asciiSymbolChar(start->at(0)))
			{
				++ic;
				while (start->length() > ic && (asciiSymbolChar(start->at(0))))
					++ic;
				if (!reservedOp(start->substr(0,ic)))
					return false;
				return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
			}
			return 0;
		}
	}; 
	template <typename P1>
	shared_ptr<Choices>
		operator /(const shared_ptr<Choices>& c, const shared_ptr<P1>& p1)
	{
		shared_ptr<Choices> r = c;
		r->choices.push_back(p1);
		return r;
	}
#if 0
	template <typename P1, typename P2>
	shared_ptr<choice2<P1,P2>>
		operator /(const shared_ptr<P1>& p1, const shared_ptr<P2>& p2)
	{
		return make_shared<choice2<P1,P2>>("",p1,p2);
	}
#else
	template <typename P1, typename P2>
	shared_ptr<Choices>
		operator /(const shared_ptr<P1>& p1, const shared_ptr<P2>& p2)
	{
		shared_ptr<Choices> r = make_shared<Choices>("");
		r->choices.push_back(p1);
		r->choices.push_back(p2);
		return r;
	}
#endif
#if 0
	template <typename P1, typename P2,typename P3>
	shared_ptr<choice3<P1,P2,P3>>
		operator /(const shared_ptr<choice2<P1,P2>>& p12, const shared_ptr<P3>& p3)
	{
		return make_shared<choice3<P1,P2,P3>>("",p12->next1,p12->next2,p3);
	}
	template <typename P1, typename P2,typename P3,typename P4>
	shared_ptr<choice4<P1,P2,P3,P4>>
		operator /(const shared_ptr<choice3<P1,P2,P3>>& p123, const shared_ptr<P4>& p4)
	{
		return make_shared<choice4<P1,P2,P3,P4>>("",p123.next1,p123.next2,p123.next3,p4);
	}
	template <typename P1, typename P2,typename P3,typename P4,typename P5>
	shared_ptr<choice5<P1,P2,P3,P4,P5>>
		operator /(const shared_ptr<choice4<P1,P2,P3,P4>>& p1234, const shared_ptr<P5>& p5)
	{
		return make_shared<choice5<P1,P2,P3,P4,P5>>("",p1234.next1,p1234.next2,p1234.next3,p1234.next4,p5);
	}
#endif
	shared_ptr<WSequenceN>
		operator ,(const shared_ptr<ParserBase>& p1, const shared_ptr<ParserBase>& p2)
	{
		auto r = make_shared<WSequenceN>("");
		r->elements.push_back(p1);
		r->elements.push_back(p2);
		return r;
	}
	shared_ptr<WSequenceN>
		operator ,(const shared_ptr<WSequenceN>& p1, const shared_ptr<ParserBase>& p2)
	{
		auto r = p1;
		p1->elements.push_back(p2);
		return r;
	}
#if 0
	template <typename P1, typename P2,typename P3>
	shared_ptr<wsequence3<P1,P2,P3>>
		operator ,(const shared_ptr<wsequence2<P1,P2>>& p12, const shared_ptr<P3>& p3)
	{
		return make_shared<wsequence3<P1,P2,P3>>("",p12->next1,p12->next2,p3);
	}
	template <typename P1, typename P2,typename P3,typename P4>
	shared_ptr<wsequence4<P1,P2,P3,P4>>
		operator ,(const shared_ptr<wsequence3<P1,P2,P3>>& p123, const shared_ptr<P4>& p4)
	{
		return make_shared<wsequence4<P1,P2,P3,P4>>("",p123.next1,p123.next2,p123.next3,p4);
	}
	template <typename P1, typename P2,typename P3,typename P4,typename P5>
	shared_ptr<wsequence5<P1,P2,P3,P4,P5>>
		operator ,(const shared_ptr<wsequence4<P1,P2,P3,P4>>& p1234, const shared_ptr<P5>& p5)
	{
		return make_shared<wsequence5<P1,P2,P3,P4,P5>>("",p1234.next1,p1234.next2,p1234.next3,p1234.next4,p5);
	}
#endif
			
	// module ::= 'module' name '(' exports ')' 'where' body
	// body ::= '{' impdecls ';' topdecls '}'
	// body ::= '{' impdecls '}'
	// body ::= '{' topdecls '}'
	// topdecls ::= topdecl (';' topdecl)*
	// topdecl ::= topdecl-kw | decl
	// decls ::= '{' decl (';' decl)* '}'
	// decl ::= gendecl | (funlhs|pat) rhs
	extern char module_kw[] = "module";
	extern char where_kw[] = "where";
	extern char qualified_kw[] = "qualified";
	extern char import_kw[] = "import";
	extern char as_kw[] = "as";
	extern char hiding_kw[] = "hiding";
	extern char let_kw[] = "let";
	extern char in_kw[] = "in";
	extern char if_kw[] = "if";
	extern char then_kw[] = "then";
	extern char else_kw[] = "else";
	extern char case_kw[] = "case";
	extern char of_kw[] = "of";
	extern char do_kw[] = "do";
	typedef ttoken<module_kw> ModuleKw;
	typedef ttoken<where_kw> WhereKw;
	typedef ttoken<qualified_kw> QualifiedKW;
	typedef ttoken<import_kw> ImportKW;
	typedef ttoken<as_kw> AsKW;
	typedef ttoken<hiding_kw> HidingKW;
	typedef ttoken<let_kw> LetKW;
	typedef ttoken<in_kw> InKW;
	typedef ttoken<if_kw> IfKW;
	typedef ttoken<then_kw> ThenKW;
	typedef ttoken<else_kw> ElseKW;
	typedef ttoken<case_kw> CaseKW;
	typedef ttoken<of_kw> OfKW;
	typedef ttoken<do_kw> DoKW;
	extern const char colonColonStr[] = "::";
	extern const char fnToStr[] = "->";
	extern const char patFromStr[] = "<-";
	extern const char dotdotStr[] = "..";
	extern const char contextToStr[] = "=>";
	typedef ttoken<colonColonStr> ColonColon;
	typedef ttoken<fnToStr> FnTo;
	typedef ttoken<dotdotStr> DotDot;
	typedef ttoken<contextToStr> ContextTo;
	typedef ttoken<patFromStr> PatFrom;
void hs()
{
	auto moduleKw = make_shared<ModuleKw>();// moduleKw;
	auto whereKw = make_shared<WhereKw>();// whereKw;
	auto importKw = make_shared<ImportKW>(); // import Kw;
	auto qualifiedKw = make_shared<QualifiedKW>(); 
	auto asKw = make_shared<AsKW>();
	auto hidingKw = make_shared<HidingKW>();
	auto letKw = make_shared<LetKW>();
	auto inKw = make_shared<InKW>();
	auto ifKw = make_shared<IfKW>();
	auto thenKw = make_shared<ThenKW>();
	auto elseKw = make_shared<ElseKW>();
	auto caseKw = make_shared<CaseKW>();
	auto ofKw = make_shared<OfKW>();
	auto doKw = make_shared<DoKW>();
	auto moduleID = make_shared<ConID>();// moduleID;
	auto varID = make_shared<VarID>();// varID;
	auto conID = make_shared<ConID>();// conID;
	auto lparen = make_shared<tch<'('>>();// lparen;
	auto rparen = make_shared<tch<')'>>();// rparen;
	auto lbrace = make_shared<tch<'{'>>();// lbrace;
	auto rbrace = make_shared<tch<'}'>>();// rbrace;
	auto lbracket = make_shared<tch<'['>>();// lbracket;
	auto rbracket = make_shared<tch<']'>>();// rbracket;
	auto comma = make_shared<tch<','>>();
	auto semicolon = make_shared<tch<';'>>();// semicolon;
	auto equals = make_shared<tch <'='>>();// equals;
	auto minus = make_shared<tch <'-'>>();// minus;
	auto backTick = make_shared<tch <'`'>>();// backTick;
	auto atSign = make_shared<tch<'@'>>();
	auto colon = make_shared<tch<':'>>();
	auto wildcard = make_shared<tch<'_'>>();
	auto tilde = make_shared<tch<'~'>>();// tilde;
	auto vertbar = make_shared<tch<'|'>>();
	auto colonColon = make_shared<ColonColon>();// colonColon;
	auto fnTo = make_shared<FnTo>();// fnTo;
	auto patFrom = make_shared<PatFrom>();
	auto contextTo = make_shared<ContextTo>();
	auto dotdot = make_shared<DotDot>();
	auto exportItem = Choice("exportItem",varID,conID);
	auto exportComma = RLoop("exportComma",exportItem,comma);
	auto exports = WSequence("exports",lparen, exportComma, rparen);
	auto varsym = make_shared<VarSym>();// varsym;
	auto consym = make_shared<ConSym>();// consym;
	//auto hliteral = Choice("literal",make_shared<NUM>(),make_shared<Quoted<'\''>>(),make_shared<Quoted<'"'>>());
	auto hliteral = make_shared<NUM>() / make_shared<Quoted<'\''>>() / make_shared<Quoted<'"'>>();
	hliteral->name = "hliteral";
	//auto varop = Choice("varop",varsym,WSequence("varIDop",backTick,varID,backTick));
	auto varop = varsym / (backTick,varID,backTick);
	varop->name = "varop";
	//auto conop = Choice("conop",consym,WSequence("conIDop",backTick,conID,backTick));
	auto conop = consym / (backTick,conID,backTick);
	conop->name = "conop";
	auto qop = Choice("qop",varop,conop);
	auto unit = WSequence("unit",lparen,rparen);
	auto elist = WSequence("[]",lbracket,rbracket);
	auto etuple = WSequence("etuple",lparen,Repeat1(",",comma),rparen);
	auto efn = WSequence("efn",lparen,fnTo,rparen);
	auto qconsym = consym;
	auto gconsym = Choice("gconsym",colon,qconsym);
	auto qcon = Choice("qcon",conID,WSequence("(gconsym)",lparen,gconsym,rparen));
	auto gcon = Choice("gcon",unit,elist,etuple,qcon);
	auto qvarid = varID;
	auto qvarsym = varsym;
	auto qvar = Choice("qvar",qvarid,WSequence("(qvarsym)",lparen,qvarsym,rparen));
	auto expRef = make_shared<ParserReference>();
	auto infixexpRef = make_shared<ParserReference>();
	auto expRefComma = RLoop("expRefComma",expRef,comma);
	auto fbind = WSequence("fbind",qvar,equals,expRef);
#if 0
	auto aexp = Choice("aexp",varID,
					gcon,
					hliteral,
					WSequence("(exp)",lparen,expRef,rparen),
					Choice("aexp2",WSequence("(exp,exp)",lparen,expRefComma,rparen),
						WSequence("[exp,exp]",lbracket,expRefComma,rbracket),
						WSequence("[exp,exp..exp]",lbracket,expRef,Optional(WSequence(",exp",comma,expRef)),dotdot,expRef,rbracket),
						//WSequence("[exp|qual,..]",lbracket,expRef,vertbar,RLoop(qual,comma),rbracket),
						WSequence("(infixexp qop)",lparen,infixexpRef,qop,rparen),
						Choice("aexp3",WSequence("(qop<-> infixexp)",lparen,qop,infixexpRef,rparen),
									WSequence("qcon { fbind+ }",qcon,lbrace,Repeat1("fbind+",fbind),rbrace)
						/*more*/)));
#else
	auto aexp = varID / 
					gcon/
					hliteral/
					(make_shared<WSequenceN>("(exp)"),lparen,expRef,rparen)/
					(make_shared<WSequenceN>("(exp,exp)"),lparen,expRefComma,rparen)/
					(make_shared<WSequenceN>("[exp,exp]"),lbracket,expRefComma,rbracket)/
					(make_shared<WSequenceN>("[exp,exp..exp]"),lbracket,expRef,Optional(WSequence(",exp",comma,expRef)),dotdot,expRef,rbracket)/
					//WSequence("[exp|qual,..]",lbracket,expRef,vertbar,RLoop(qual,comma),rbracket)/
					(make_shared<WSequenceN>("(infixexp qop)"),lparen,infixexpRef,qop,rparen)/
					(make_shared<WSequenceN>("(qop<-> infixexp)"),lparen,qop,infixexpRef,rparen)/
					(make_shared<WSequenceN>("qcon{fbind+}"),qcon,lbrace,Repeat1("fbind+",fbind),rbrace);
	aexp->name = "aexp";
#endif
	auto fexpRef = make_shared<ParserReference>();
	//auto fexp = WSequence(Optional(fexpRef),aexp);
	auto fexp = Repeat1("fexp",SkipWhite(aexp));
	fexpRef->resolve(fexp);
	auto declsRef = make_shared<ParserReference>();
	auto patRef = make_shared<ParserReference>();// patRef;
	auto wheres = WSequence("where decls",whereKw,declsRef);
	auto guard = Choice("guard", WSequence("pat->infixexp",patRef,patFrom,infixexpRef)
								, WSequence("let decls",letKw,declsRef)
								, infixexpRef
						);
	auto guards = WSequence("guards",make_shared<tch<'|'>>(),Repeat1("guard+",guard));
	auto gdpat = Repeat1("gdpatr",WSequence("gdpat",guards,fnTo,expRef));
	auto alt = Choice("alt", WSequence("pat<-exp",patRef,patFrom,expRef,Optional(wheres)),
							WSequence("pat gdpat",patRef,gdpat,Optional(wheres))
							);
	auto alts = RLoop2("altSemiAlt",alt,semicolon);
	auto stmt = Choice("stmt",
						WSequence("stmt:exp",expRef,semicolon),
						WSequence("stmt:pat->exp",patRef,fnTo,expRef,semicolon),
						WSequence("stmt:let decls",letKw,declsRef,semicolon),
						semicolon);
	auto stmts = WSequence("stmts",Repeat1("stmt+",SkipWhite(stmt)),expRef,Optional(semicolon));
	auto lexp = Choice("lexp",
						WSequence("lambdaabs",make_shared<tch<'\\'>>(),Repeat1("\\aexp+",SkipWhite(aexp)),fnTo,expRef), 
						Choice("lexp-kw",
							WSequence("let",letKw,declsRef,inKw,expRef),
							WSequence("if",ifKw,expRef,Optional(semicolon),WSequence("ifthen",thenKw,expRef,Optional(semicolon)),WSequence("ifelse",elseKw,expRef)),
							WSequence("case",caseKw,expRef,ofKw,alts),
							WSequence("do",doKw,lbrace,stmts,rbrace)),
						fexp);
	auto infixexp = Choice("infixexp", WSequence("lexp,qop,infixexp",lexp,qop,infixexpRef),WSequence("-infixexp",minus,infixexpRef),lexp);
	auto typeRef = make_shared<ParserReference>();
	auto atypeRef = make_shared<ParserReference>();
	auto tycls = conID;
	auto qtycls = tycls;
	auto tyvar = varID;
	auto klass = Choice("klass",WSequence("klassNulTyVar",qtycls,tyvar),WSequence("klassMultiTyVar",qtycls,lparen,tyvar,Repeat1("",atypeRef)));
	auto context = Choice("context", klass, WSequence("(klass ,klass*)",lparen,RLoop("klassComma",klass,comma),rparen));
	auto exp = WSequence("exp",infixexpRef,Optional(WSequence("typesig",colonColon,Optional(WSequence("context",context,contextTo)),typeRef))) ;
	infixexpRef->resolve(infixexp);
	expRef->resolve(exp);
	auto rhs = WSequence("rhs",equals, exp);
	auto tycon = conID;
	auto qtycon = tycon;
	auto gtycon = Choice("gtycon",qtycon,unit,elist,efn,etuple);
	auto var = Choice("var",varID,WSequence("(varsym)",lparen,varsym,rparen));
	auto con = Choice("con",conID,WSequence("(consym)",lparen,consym,rparen));
	auto tupleCon = WSequence("tupleCon",lparen,RLoop("type,",typeRef,comma),rparen);
	auto listCon = WSequence("listCon",lbracket, typeRef, rbracket);
	auto parType = WSequence("parType",lparen,typeRef,rparen);
	auto atype = Choice("atype",gtycon,tyvar,tupleCon,listCon,parType);
	atypeRef->resolve(atype);
	auto btypeRef = make_shared<ParserReference>();
	//auto btype = WSequence(Optional(btypeRef),atype); // need to reformulate
	auto btype = Repeat1("btype",SkipWhite(atype)); // maybe?
	//auto btype = atype;
	btypeRef->resolve(btype);
	auto type = btype; 
	typeRef->resolve(type);
	auto gendecl = WSequence("gendecl",Repeat1("varID",SkipWhite(varID)),colonColon,type);
	auto apatRef = make_shared<ParserReference>();// apatRef;
	auto fpat = WSequence("fpat",varID,equals,patRef);
	auto patternComma = RLoop("patternComma",patRef,comma);
	auto patterns = Choice("patterns",WSequence("(pat)",lparen,patRef,rparen)
					, WSequence("(pat,pat)",lparen,patternComma,rparen)
					, WSequence("[pat,pat]",lbracket,patternComma,lbracket)
					, WSequence("~apat",tilde,apatRef)
					);
	auto apat = Choice("apat",WSequence("var@apat",var,Optional(WSequence("@apat",atSign,apatRef)))
				, gcon
				, WSequence("qcon{fpat,fpat}",qcon, lbrace, Optional(RLoop("fpat,",fpat,comma)), rbrace)
				, wildcard
				, patterns);
	apatRef->resolve(apat);
	auto funlhsRef = make_shared<ParserReference>();// funlhsRef;
	auto funlhs = Choice("funlhs",WSequence("var apat+",varID,Repeat1("apat+",apat)),WSequence("pat,varop,pat",patRef,varop,patRef),WSequence("(funlhs)apat",lparen,funlhsRef,rparen,Repeat1("apat+",apat)));
	funlhsRef->resolve(funlhs);
	auto qconop = Choice("qconop",gconsym,WSequence("`conID`",backTick,conID,backTick));
	auto lpat = Choice("lpat",apat,WSequence("gcon apat+",gcon,Repeat1("apat+",apat)));
	auto pat = Choice("pat",WSequence("lpat,qconop,pat",lpat,qconop,patRef),lpat);
	patRef->resolve(pat);
	auto decl = Choice("decl",gendecl,WSequence("funlhs|pat rhs",Choice("funlhs|pat",funlhs,pat),rhs));
	auto decls = RLoop("decls",decl,semicolon);
	declsRef->resolve(decls);
	auto cname = Choice("cname",var,con);
	auto import = Choice("import", var, 
							WSequence("tyconwcnames",tycon,Optional(Choice("dotsorcnames",WSequence("(..)",lparen,dotdot,rparen),
															   WSequence("(cnames)",lparen,RLoop("cnames",cname,comma),rparen)))),
							WSequence("tyclswvars",tycls,Optional(Choice("dotsorvars",WSequence("(..)",lparen,dotdot,rparen),
															   WSequence("(vars)",lparen,RLoop("vars",var,comma),rparen)))));
	auto impspec = WSequence("impspec",Optional(hidingKw),lparen,RLoop("importComma",import,comma),rparen);
	auto impdecl = WSequence("impdecl",importKw,Optional(qualifiedKw),moduleID,Optional(WSequence("as modopt",asKw,moduleID)),Optional(impspec));
	auto imports = RLoop2("imports", impdecl, semicolon);
	auto body = Choice("body", WSequence("bodyOfDeclsWithImports", lbrace, imports, /*semicolon,*/ decls, rbrace), 
								WSequence("bodyOfImports",lbrace, imports, rbrace), 
								WSequence("bodyOfDecls",lbrace, decls, rbrace));
	auto module = WSequence("module",moduleKw, moduleID, exports, whereKw, body);
	string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4=(\\a->a+1) 5; circumference r=2*pi*r }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4= \\a->a+1; circumference r=2*pi*r }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; circumference r=2*pi*r }");
	ParseStatePtr st = make_shared<ParseState>(input);
	ParseResultPtr r = (*module)(st);
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
int main(int argc, const char* argv[]) {
	cout << "PEGHS starts" << endl;
	//hs();
#if 1
	shared_ptr<ParserBase> p = x();
	string input1("12345ABCDabcd");
	ParseStatePtr st = make_shared<ParseState>(input1);
	ParseResultPtr r = (*p)(st);
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
	string input1b("x = a + b + c * 9 + 10 * d");
	ParseStatePtr st1b = make_shared<ParseState>(input1b);
	r = (*y())(st1b);
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
	ParseStatePtr st1c = make_shared<ParseState>(input1c);
	r = (*y())(st1c);

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
	string input2("circumference r = 2 * pi * r");

	//typedef trange<32, 0x1FFFF> AnyChar;
	//AnyChar anyChar;
	//typedef repeat0<AnyChar> Anything;
	//Anything anything(anyChar);
	auto anything = Repeat0(make_shared<trange<32,0x1FFFF>>());


	/*
equation : lhs '=' rhs
     rhs : term addl
rhs : term
addl : '+' term
term : factor mult
term : factor
mult : '*' factor
factor : application
factor : num
factor : '(' rhs ')'
application : id factor*
lhs : id*
*/

	ParseStatePtr doc1=make_shared<ParseState>(input2);

	rhsRef->resolve(addition);
	factorRef->resolve(factor);
	

	ParseResultPtr e0 = (*equation)(doc1);
	//ParseResult* e0 = docparse(&doc1);
	if (e0)
	{
		cout << "*e0 [" << *e0->remaining << ']' << endl;
		cout << "AST " << *e0->ast;
		cout << endl;
	}
	else
	{
		cout << "e0==null" << endl;
	}
#endif
	return 0;
}
