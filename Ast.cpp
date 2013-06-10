#include "Ast.h"
#include <sstream>

std::string IntegerAST::to_string() const 
{ 
	std::ostringstream os; 
	os << value; 
	return os.str(); 
}

std::string SequenceAST::to_string() const 
{
	std::ostringstream os;
	if (members.size())
		os << '[' << members[0]->to_string();
	for (size_t i=1; i<members.size(); ++i)
		os << ' ' << members[i]->to_string();
	os << ']';
	return os.str();
}
