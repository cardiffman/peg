#include "Ast.h"
#include <sstream>

std::string IntegerAST::to_string() const 
{ 
	std::ostringstream os; 
	os << value; 
	return os.str(); 
}

std::string FloatAST::to_string() const
{
	std::ostringstream os;
	os << value;
	return os.str();
}

std::string SequenceAST::to_string() const 
{
	std::ostringstream os;
	if (!members.size())
		os << "()";
	else
	{
		os << '(' << members[0]->to_string();
		for (size_t i=1; i<members.size(); ++i)
			os << ' ' << members[i]->to_string();
		os << ')';
	}
	return os.str();
}

void SequenceAST::append(ASTPtr ast)
{
	SequenceAST* inner = dynamic_cast<SequenceAST*>(ast.get());
	// Clearly one could obscure the hierarchical structure
	// of the nested sequences by endeavoring to concatenate
	// a SequenceAST if it were the ast argument. That is not
	// my intent. My intent is to avoid concatenating sequences
	// that have no members.
	if (!inner) // appendage is not a sequence
		members.push_back(ast);
	else if (inner->size())
		members.push_back(ast);
}

