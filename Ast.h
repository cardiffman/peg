#ifndef INCLUDED_AST_H
#define INCLUDED_AST_H

#include <string>
#include <iosfwd>
#include <vector>

class AST
{
public:
	AST() {}
	virtual ~AST() {}
	virtual std::string to_string() const = 0;
	friend std::ostream& operator<<(std::ostream& os, const AST& ast);
};
class IntegerAST : public AST
{
public:
	IntegerAST(int value) : value(value) {}
	std::string to_string() const;
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
	std::string to_string() const ;
private:
	std::vector<AST*> members;
};
#endif // INCLUDED_AST_H
