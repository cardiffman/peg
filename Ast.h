#ifndef INCLUDED_AST_H
#define INCLUDED_AST_H

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

class AST
{
public:
	AST() {}
	virtual ~AST() {}
	virtual std::string to_string() const = 0;
	/*AST(const string& s) : s(s) {}
	string s;
	vector<AST*> ast;*/
	friend std::ostream& operator<<(std::ostream& os, const AST& ast);
};
class IntegerAST : public AST
{
public:
	IntegerAST(int value) : value(value) {}
	std::string to_string() const { std::ostringstream os; os << value; return os.str(); }
private:
	int value;
};
class IdentifierAST : public AST
{
public:
	IdentifierAST(const std::string& name) : name(name) {}
	std::string to_string() const { return name; }
private:
	std::string name;
};
class StringAST : public AST
{
public:
	StringAST(const std::string& name) : name(name) {}
	std::string to_string() const { return name; }
private:
	std::string name;
};
class SequenceAST : public AST
{
public:
	SequenceAST() {}
	void append(AST* ast) { members.push_back(ast); }
	std::string to_string() const 
	{
		std::ostringstream os;
		if (members.size())
			os << '[' << members[0]->to_string();
		for (size_t i=1; i<members.size(); ++i)
			os << ' ' << members[i]->to_string();
		os << ']';
		return os.str();
	}
private:
	std::vector<AST*> members;
};
#endif // INCLUDED_AST_H
