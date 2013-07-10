#ifndef INCLUDED_AST_H
#define INCLUDED_AST_H

#include <string>
#include <iosfwd>
#include <vector>
#include <memory>

class AST
{
public:
	AST() {}
	virtual ~AST() {}
	virtual std::string to_string() const = 0;
	friend std::ostream& operator<<(std::ostream& os, const AST& ast);
};
typedef std::shared_ptr<AST> ASTPtr;
class IntegerAST : public AST
{
public:
	IntegerAST(int value) : value(value) {}
	std::string to_string() const;
private:
	int value;
};
class FloatAST : public AST
{
public:
	FloatAST(double value) : value(value) {}
	std::string to_string() const;
private:
	double value;
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
class CharAST : public AST
{
public:
	CharAST(const std::string& name) : name(name) {}
	explicit CharAST(int name) : name(1,name) {}
	std::string to_string() const { return name; }
private:
	std::string name;
};
class SequenceAST : public AST
{
public:
	SequenceAST() {}
	void append(ASTPtr ast);
	ASTPtr at(size_t m) { return members[m]; }
	size_t size() const { return members.size(); }
	std::string to_string() const ;
private:
	std::vector<ASTPtr> members;
};
#endif // INCLUDED_AST_H
