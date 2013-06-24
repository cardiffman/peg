#include "peg.h"
#include "ParseState.h"
#include "Ast.h"
#include "Action.h"

#include <utility>
#include <memory>
#include <cctype>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <stack>

using std::string;
using std::make_shared;
using std::shared_ptr;
using std::cout;
using std::endl;

struct ConID : public ParserBase
{
	ConID() : ParserBase("ConID") {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t ic = 0;
		if (start->length() > 0 && isalpha(start->at(0)) && isupper(start->at(0)))
		{
			++ic;
			while (start->length() > ic && (isalnum(start->at(ic)) || start->at(ic)=='.'))
				++ic;
			return make_shared<ParseResult>(start->from(ic), std::make_shared<IdentifierAST>(start->substr(0, ic)));
		}
		return ParseResultPtr();
	}
};
bool reservedid(const string& sym)
{
	return sym=="case"||sym=="class"||sym=="data"||sym=="default"||
			sym=="deriving"||sym=="do"||sym=="else"||sym=="foreign"||
			sym=="if"||sym=="import"||sym=="in"||sym=="infix"||sym=="infixl"||
			sym=="infixr"||sym=="instance"||sym=="let"||sym=="module"||
			sym=="newtype"||sym=="of"||sym=="then"||sym=="type"||sym=="where"||
			sym=="_";
}
struct VarID : public ParserBase
{
	VarID() : ParserBase("VarID") {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t ic = 0;
		if (start->length() > 0 && isalpha(start->at(0)) && islower(start->at(0)))
		{
			++ic;
			while (start->length() > ic && (isalnum(start->at(ic))))
				++ic;
			if (reservedid(start->substr(0,ic)))
				return ParseResultPtr();
			return make_shared<ParseResult>(start->from(ic), std::make_shared<IdentifierAST>(start->substr(0, ic)));
		}
		return ParseResultPtr();
	}
};
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
bool asciiSymbolChar(int ch)
{
	switch (ch) {
	case '!': case '#': case '$': case '%': case '&': case '*': case '+':
		case '.': case '/': case '<': case '=': case '>': case '?': case '@':
			case '\\': case '^': case '|': case '-': case '~': case ':':
		return true;
	}
	return false;
}
bool asciiSymbolCharExceptColon(int ch)
{
	switch (ch) {
	case '!': case '#': case '$': case '%': case '&': case '*': case '+':
		case '.': case '/': case '<': case '=': case '>': case '?': case '@':
			case '\\': case '^': case '|': case '-': case '~':
		return true;
	}
	return false;
}
struct ReservedOp : public ParserBase
{
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t ic = 0;
		if (start->length() > 0 && asciiSymbolChar(start->at(0)))
		{
			++ic;
			while (start->length() > ic && (asciiSymbolChar(start->at(ic))))
				++ic;
			if (!reservedOp(start->substr(0,ic)))
				return ParseResultPtr();
			return make_shared<ParseResult>(start->from(ic), std::make_shared<IdentifierAST>(start->substr(0, ic)));
		}
		return ParseResultPtr();
	}
};
struct VarSym : public ParserBase
{
	VarSym() : ParserBase("VarSym") {}
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
				return ParseResultPtr();
			}
			cout << "VarSym [" << start->substr(0,ic) << ']' << endl;
			return make_shared<ParseResult>(start->from(ic), std::make_shared<IdentifierAST>(start->substr(0, ic)));
		}
		return ParseResultPtr();
	}
};
struct ConSym : public ParserBase
{
	ConSym() : ParserBase("ConSym") {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t ic = 0;
		if (start->length() > 0 && start->at(0)==':')
		{
			++ic;
			while (start->length() > ic && (asciiSymbolChar(start->at(ic))))
				++ic;
			if (reservedOp(start->substr(0,ic)))
				return ParseResultPtr();
			cout <<"ConSym [" << start->substr(0,ic) << ']' << endl;
			return make_shared<ParseResult>(start->from(ic), std::make_shared<IdentifierAST>(start->substr(0, ic)));
		}
		return ParseResultPtr();
	}
};
// Quoted strings in HTML can use either the single quote or
// the double quote. HTML doesn't allow escaping the quoting
// character but an entity &apos; or &quote; can be used.
template <int quote> struct Quoted : public ParserBase
{
	Quoted() : ParserBase("Quoted-"+string(1,quote)) {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t ic =0;
		if (start->length() < 2)
			return ParseResultPtr();
		if (start->at(0) != quote)
			return ParseResultPtr();
		++ic;
		while (ic < start->length()-1 && start->at(ic) != quote)
		{
			if (start->at(ic)=='\\')
				++ic;
			++ic;
		}
		if (start->at(ic) != quote)
			return ParseResultPtr();
		return make_shared<ParseResult>(start->from(ic+1), std::make_shared<StringAST>(start->substr(0, ic+1)));
	}
};

shared_ptr<Choices> ChoicesName(const std::string& name)
								{
	shared_ptr<Choices> r = make_shared<Choices>(name);
	return r;
								}
shared_ptr<WSequenceN> SequenceName(const std::string& name)
		{
	shared_ptr<WSequenceN> r = make_shared<WSequenceN>(name);
	return r;
		}

// When you want a parse's failure to be a good thing.
// This parser does not "consume" input.
#if 0
template <typename Parser> struct isnt : public ParserBase
{
	isnt(const shared_ptr<Parser>& next) : next(next) {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		ParseResultPtr rep = (*next)(start);
		if (!rep) // don't advance, but do succeed
			return make_shared<ParseResult>(start->from(0), std::make_shared<StringAST>(""));
		return ParseResultPtr(); // whatever it is, is present, so fail.
	}
	shared_ptr<Parser> next;
};
template <typename Parser> shared_ptr<isnt<Parser>> Isnt(const shared_ptr<Parser>& parser)
		{
	return make_shared<isnt<Parser>>(parser);
		}
#else
struct isnt : public ParserBase
{
	isnt(const shared_ptr<ParserBase>& next) : next(next) {}
	static int recur;
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		if (recur== next->parser_id || recur != 0)
		{
			cout << __PRETTY_FUNCTION__ << " Recursion? " << endl;
			return ParseResultPtr();
		}
		recur = next->parser_id;
		ParseResultPtr rep = (*next)(start);
		if (!rep) // don't advance, but do succeed
		{
			recur = 0;
			return make_shared<ParseResult>(start->from(0), std::make_shared<StringAST>(""));
		}
		cout << __PRETTY_FUNCTION__ << " Is failing! " << rep->getAST() << endl;
		recur = 0;
		return ParseResultPtr(); // whatever it is, is present, so fail.
	}
	shared_ptr<ParserBase> next;
};
int isnt::recur=0;
shared_ptr<ParserBase> Isnt(const shared_ptr<ParserBase>& parser)
		{
	return make_shared<isnt>(parser);
		}
#endif


extern const char module_kw[] = "module";
extern const char where_kw[] = "where";
extern const char qualified_kw[] = "qualified";
extern const char import_kw[] = "import";
extern const char as_kw[] = "as";
extern const char hiding_kw[] = "hiding";
extern const char let_kw[] = "let";
extern const char in_kw[] = "in";
extern const char if_kw[] = "if";
extern const char then_kw[] = "then";
extern const char else_kw[] = "else";
extern const char case_kw[] = "case";
extern const char of_kw[] = "of";
extern const char do_kw[] = "do";
extern const char type_kw[] = "type";
extern const char data_kw[] = "data";
extern const char newtype_kw[] = "newtype";
extern const char class_kw[] = "class";
extern const char instance_kw[] = "instance";
extern const char default_kw[] = "default";
extern const char foreign_kw[] = "foreign";
extern const char deriving_kw[] = "deriving";
extern const char safe_kw[] = "safe";
extern const char unsafe_kw[] = "unsafe";
extern const char ccall_kw[] = "ccall";
extern const char stdcall_kw[] = "stdcall";
extern const char cplusplus_kw[] = "cplusplus";
extern const char jvm_kw[] = "jvm";
extern const char dotnet_kw[] = "dotnet_kw";
extern const char export_kw[] = "export";
extern const char infix_kw[] = "infix";
extern const char infixl_kw[] = "infixl";
extern const char infixr_kw[] = "infixr";
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
typedef ttoken<type_kw> TypeKW;
typedef ttoken<data_kw> DataKW;
typedef ttoken<newtype_kw> NewtypeKW;
typedef ttoken<class_kw> ClassKW;
typedef ttoken<instance_kw> InstanceKW;
typedef ttoken<default_kw> DefaultKW;
typedef ttoken<foreign_kw> ForeignKW;
typedef ttoken<deriving_kw> DerivingKW;
typedef ttoken<safe_kw> SafeKW;
typedef ttoken<unsafe_kw> UnsafeKW;
typedef ttoken<ccall_kw> CcallKw;
typedef ttoken<stdcall_kw> StdcallKw;
typedef ttoken<cplusplus_kw> CplusplusKw;
typedef ttoken<jvm_kw> JvmKw;
typedef ttoken<dotnet_kw> DotnetKw;
typedef ttoken<export_kw> ExportKw;
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

class DeclAST : public AST
{
public:
	string to_string() const;
};
ASTPtr Decl(const ASTPtr& ast)
{
	cout << __PRETTY_FUNCTION__ << endl;
	SequenceAST* sequence = dynamic_cast<SequenceAST*>(ast.get());
	if (sequence)
	{
		cout << "Decl 0: " << sequence->at(0)->to_string() << endl;
		if (sequence->size()>1)
		cout << "Decl 1: " << sequence->at(1)->to_string() << endl;
	}
	else
	{
		cout << "Decl that's not a sequence: " << ast->to_string() << endl;
	}
	return ast;
}
ASTPtr LetClause(const ASTPtr& ast)
{
	cout << __PRETTY_FUNCTION__ << endl;
	SequenceAST* sequence = dynamic_cast<SequenceAST*>(ast.get());
	if (sequence)
	{
		cout << "Let 0: " << sequence->at(0)->to_string() << endl;
		if (sequence->size()>1)
		cout << "Let 1: " << sequence->at(1)->to_string() << endl;
	}
	else
	{
		cout << "Let that's not a sequence: " << ast->to_string() << endl;
	}
	return ast;
}
ASTPtr Pat(const ASTPtr& ast)
{
	cout << __PRETTY_FUNCTION__ << endl;
	cout << "Pattern: " << ast->to_string() << endl;
	return ast;
}
void hs(string& input)
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
	auto typeKw = make_shared<TypeKW>();
	auto dataKw = make_shared<DataKW>();
	auto newtypeKw = make_shared<NewtypeKW>();
	auto classKw = make_shared<ClassKW>();
	auto instanceKw = make_shared<InstanceKW>();
	auto defaultKw = make_shared<DefaultKW>();
	auto foreignKw = make_shared<ForeignKW>();
	auto derivingKw = make_shared<DerivingKW>();
	auto safeKw = make_shared<SafeKW>();
	auto unsafeKw = make_shared<UnsafeKW>();
	auto ccallKw = make_shared<CcallKw>();
	auto stdcallKw = make_shared<StdcallKw>();
	auto cplusplusKw = make_shared<CplusplusKw>();
	auto jvmKw = make_shared<JvmKw>();
	auto dotnetKw = make_shared<DotnetKw>();
	auto exportKw = make_shared<ExportKw>();
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
	auto dot = make_shared<tch<'.'>>();
	auto colonColon = make_shared<ColonColon>();// colonColon;
	auto fnTo = make_shared<FnTo>();// fnTo;
	auto patFrom = make_shared<PatFrom>();
	auto contextTo = make_shared<ContextTo>();
	auto dotdot = make_shared<DotDot>();
	auto varsym = make_shared<VarSym>();// varsym;
	auto consym = make_shared<ConSym>();// consym;
	auto hliteral = make_shared<NUM>() || make_shared<Quoted<'\''>>() || make_shared<Quoted<'"'>>();
	hliteral->name = "hliteral";
	auto varop = ChoicesName("varop") || varsym || (SequenceName("varIDop") && backTick && varID && backTick);
	auto conop = ChoicesName("conop") || consym || (SequenceName("conIDop") && backTick && conID && backTick);

	auto modid = SequenceName("modid") && Repeat0(SequenceName("[conID.]") && conID && dot) && conID;

	auto modQual = SequenceName("modQual") && Optional(SequenceName("modid.") && modid && dot);

	auto qconsym = SequenceName("qconsym") && modQual && consym;
	auto qvarid = SequenceName("qvarid") && modQual && varID;
	auto qconid = SequenceName("qvarid") && modQual && conID;
	auto qvarsym = SequenceName("qvarsym") && modQual && varsym;
	auto qvarop = ChoicesName("qvarop") || qvarsym || (SequenceName("qvarIDop") && backTick && qvarid && backTick);
	auto qconop = ChoicesName("qconop") || qconsym || (SequenceName("qconIDop") && backTick && qconid && backTick);
	auto qop = ChoicesName("qop")|| qvarop || qconop;
	auto unit = (SequenceName("unit") && lparen && rparen);
	auto elist = (SequenceName("[]") && lbracket && rbracket);
	auto etuple = (SequenceName("etuple") && lparen && Repeat1(",",comma) && rparen);
	auto efn = (SequenceName("efn") && lparen && fnTo && rparen);
	auto qvar = ChoicesName("qvar") || qvarid || (SequenceName("(qvarsym)") && lparen && qvarsym && rparen);
	auto gconsym = ChoicesName("gconsym") || colon || qconsym;
	auto qcon = ChoicesName("qcon") || conID || (SequenceName("(gconsym)") && lparen && gconsym &&rparen);
	auto gcon = ChoicesName("gcon") || unit || elist || etuple || qcon;

	auto tycls = conID;
	auto tyvar = varID;
	auto tycon = conID;

	auto qtycon = SequenceName("qtycon") && modQual && tycon;
	auto qtycls = SequenceName("qtycls") && modQual && tycls;

	auto exportItem = ChoicesName("exportItem") || qvar || conID;
	auto exportComma = RLoop("exportComma",exportItem,comma);
	auto exports = (SequenceName("exports") && lparen && exportComma && rparen);
	auto expRef = make_shared<ParserReference>();
	auto infixexpRef = make_shared<ParserReference>();
	auto expRefComma = RLoop("expRefComma",expRef,comma);
	auto fbind = (SequenceName("fbind") && qvar && equals && expRef);
	auto fexpRef = make_shared<ParserReference>();
	auto aexpRef = make_shared<ParserReference>();
	auto declsRef = make_shared<ParserReference>();
	auto patRef = make_shared<ParserReference>();// patRef;
	auto wheres = (SequenceName("where decls") && whereKw && declsRef);
	auto lets = Action(LetClause, SequenceName("let decls") && letKw && declsRef);
	//auto lets = make_shared<ParserReference>();
	auto guard = ChoicesName("guard")
								|| (SequenceName("pat->infixexp") && patRef && patFrom && infixexpRef)
								|| lets//(SequenceName("let decls") && lets)
								|| infixexpRef
						        ;
	auto guards = (SequenceName("guards") && make_shared<tch<'|'>>() && Repeat1("guard+",guard));
	auto gdpat = Repeat1("gdpatr",(SequenceName("gdpat") && guards && fnTo && expRef));
	auto alt = ChoicesName("alt") || (SequenceName("pat<-exp") && patRef && patFrom && expRef && Optional(wheres))
										   || (SequenceName("pat gdpat") && patRef && gdpat && Optional(wheres))
											;
	auto alts = RLoop2("altSemiAlt",alt,semicolon);
	auto stmt = ChoicesName("stmt")
							||(SequenceName("stmt:exp") && expRef && semicolon)
							||(SequenceName("stmt:pat->exp") && patRef && fnTo && expRef && semicolon)
							||(SequenceName("stmt:let decls") && lets && semicolon)
							||semicolon
							;
	auto stmts = (SequenceName("stmts") && Repeat1("stmt+",SkipWhite(stmt)) && expRef && Optional(semicolon));
	auto lexp = ChoicesName("lexp")
						||(SequenceName("lambdaabs") && make_shared<tch<'\\'>>() && Repeat1("\\aexp+",SkipWhite(aexpRef)) && fnTo && expRef)
						||(ChoicesName("lexp-kw")
							||(SequenceName("let") && lets && inKw && expRef)
							||(SequenceName("if") && ifKw && expRef && Optional(semicolon) && (SequenceName("ifthen") && thenKw && expRef && Optional(semicolon)) && (SequenceName("ifelse") && elseKw && expRef))
							||(SequenceName("case") && caseKw && expRef && ofKw && alts)
							||(SequenceName("do") && doKw && lbrace && stmts && rbrace))
						||fexpRef
						;
	auto infixexp = ChoicesName("infixexp")
						|| (SequenceName("lexp,qop,infixexp") && lexp && qop && infixexpRef)
						|| (SequenceName("-infixexp") && minus && infixexpRef)
						|| lexp
						;
	auto typeRef = make_shared<ParserReference>();
	auto atypeRef = make_shared<ParserReference>();
	auto klass = ChoicesName("klass")
						||(SequenceName("klassNulTyVar") && qtycls && tyvar)
						||(SequenceName("klassMultiTyVar") && qtycls && lparen && tyvar && Repeat1("",atypeRef) && rparen)
						;
	auto context = ChoicesName("context")
						|| klass
						|| (SequenceName("(klass ,klass*)") && lparen && RLoop("klassComma",klass,comma) && rparen)
						;
	auto exp = SequenceName("exp")
			&& infixexp
			&& Optional(SequenceName("typesig")
					&& colonColon
					&& Optional(SequenceName("context") && context && contextTo)
					&& typeRef) ;
	infixexpRef->resolve(infixexp);
	expRef->resolve(exp);


	auto rhs = (SequenceName("rhs") && equals && exp);
	auto gtycon = ChoicesName("gtycon") || qtycon || unit || elist || efn || etuple;
	auto var = ChoicesName("var")
						|| varID 
						|| (SequenceName("(varsym)") && lparen && varsym && rparen)
						;
	auto con = ChoicesName("con")
						||conID
						||(SequenceName("(consym)") && lparen && consym && rparen)
						;
	auto tupleCon = (SequenceName("tupleCon") && lparen && RLoop("type,",typeRef,comma) && rparen);
	auto listCon = (SequenceName("listCon") && lbracket && typeRef && rbracket);
	auto parType = (SequenceName("parType") && lparen && typeRef && rparen);
	auto atype = ChoicesName("atype")
						|| gtycon
						|| tyvar
						|| tupleCon
						|| listCon
						|| parType
						;
	atypeRef->resolve(atype);
	auto btypeRef = make_shared<ParserReference>();
	//auto btype = Optional(btypeRef) && atype; // need to reformulate
	auto btype = Repeat1("btype",SkipWhite(atype)); // maybe?
	//auto btype = atype;
	btypeRef->resolve(btype);
	auto type = RLoop("type", btype, fnTo); // aka btype [-> type]
	typeRef->resolve(type);
	auto vars = RLoop("vars",var,comma);
	auto infixl = make_shared<ttoken<infixl_kw>>();
	auto infixr = make_shared<ttoken<infixr_kw>>();
	auto infix = make_shared<ttoken<infix_kw>>();
	auto fixity = ChoicesName("fixity")|| infixl || infixr || infix;
	auto op = ChoicesName("op") || varop || conop;
	auto ops = Repeat1("ops",SkipWhite(op));
	auto gendecl = ChoicesName("gendecl")
					|| (SequenceName("vars::type") && vars && Optional(context && contextTo) && colonColon && type)
					|| (SequenceName("opfixity") && fixity && Optional(make_shared<NUM>()) && ops)
					;
	auto apatRef = make_shared<ParserReference>();
	auto fpat = (SequenceName("fpat") && varID && equals && patRef);
	auto patternComma = RLoop("patternComma",patRef,comma);
	auto patterns = ChoicesName("patterns")
						|| (SequenceName("(pat)") && lparen && patRef && rparen)
						|| (SequenceName("(pat,pat)") && lparen && patternComma && rparen)
						|| (SequenceName("[pat,pat]") && lbracket && patternComma && rbracket)
						|| (SequenceName("~apat") && tilde && apatRef)
						;
	auto apat = ChoicesName("apat")
						|| (SequenceName("var@apat") && var && Optional((SequenceName("@apat") && atSign && apatRef)))
						|| gcon
						|| (SequenceName("qcon{fpat,fpat}") && qcon && lbrace && Optional(RLoop("fpat,",fpat,comma)) && rbrace)
						|| wildcard
						|| patterns
						;
	apatRef->resolve(apat);
	auto funlhsRef = make_shared<ParserReference>();// funlhsRef;
	auto funlhs = ChoicesName("funlhs")
						|| (SequenceName("var apat+") && var && Repeat1("apat+",SkipWhite(apat)))
						|| (SequenceName("pat,varop,pat") && patRef && varop && patRef)
						|| (SequenceName("(funlhs)apat+") && (lparen && funlhsRef) && rparen && Repeat1("apat+",SkipWhite(apat)))
						;
	cout << "funlhs examination " << funlhs->name << " id " << funlhs->parser_id << " has " << funlhs->choices.size() << " parsers " << endl;
	for (size_t c =0; c<funlhs->choices.size(); ++c)
		cout << "funlhs examination " << funlhs->name << " id " << funlhs->parser_id << " parser["<<c<<"] "
				<< funlhs->choices[c]->name << endl;
	funlhsRef->resolve(funlhs);
	auto lpat = ChoicesName("lpat")
						||apat
						||(SequenceName("gcon apat+") && gcon && Repeat1("apat+",SkipWhite(apat)))
						;
	auto pat = make_shared<ActionCaller>(Pat, ChoicesName("pat")
						||(SequenceName("lpat,qconop,pat") && lpat && qconop && patRef)
						||lpat
						);
	patRef->resolve(pat);
	auto decl = make_shared<ActionCaller>(Decl, ChoicesName("decl")
						||gendecl
						||(SequenceName("funlhs|pat rhs") && (ChoicesName("funlhs|pat") || funlhs ||pat) && rhs)
						);
	auto decls = SequenceName("decls") && lbrace && RLoop("decl;",decl,semicolon) && rbrace;
	auto qual = ChoicesName("qual")
				|| SequenceName("pat<-exp")&& pat && patFrom && expRef
				|| lets
				|| expRef
				;
	auto isntminus = Isnt(minus);
	auto isntqcon = Isnt(qcon);
	auto aexp = ChoicesName("aexp")
				|| qvar
				|| gcon
				|| hliteral
				|| (SequenceName("(exp)") && lparen && expRef && rparen)
				|| (SequenceName("(exp,exp)") && lparen && expRefComma && rparen)
				|| (SequenceName("[exp,exp]") && lbracket && expRefComma && rbracket)
				||(SequenceName("[exp,exp..exp]") && lbracket && expRef && Optional((SequenceName(",exp") && comma && expRef)) && dotdot && expRef && rbracket)
				||(SequenceName("[exp|qual,..]") && lbracket && expRef && vertbar && RLoop("qual,",qual,comma) && rbracket)
				||(SequenceName("(infixexp qop)") && lparen && infixexpRef && qop && rparen)
				||(SequenceName("(qop<-> infixexp)") && lparen &&isntminus && qop && infixexpRef && rparen)
				||(SequenceName("qcon{fbind+}") && qcon && lbrace && Repeat1("fbind+",SkipWhite(fbind)) && rbrace)
				//||((SequenceName("aexp<qcon>{fbind+}") && isntqcon) && aexp && lbrace && Repeat1("fbind+",SkipWhite(fbind)) && rbrace)
				//||((SequenceName("aexp<qcon>{fbind+}") && ChoicesName("isntqcon")) && aexp && lbrace && Repeat1("fbind+",SkipWhite(fbind)) && rbrace)
				//||(SequenceName("aexp<qcon>{fbind+}") && isntqcon && aexpRef && lbrace && Repeat1("fbind+",SkipWhite(fbind)) && rbrace)
				// So far the recursion here has bit us bad:
				// First we got compile time problems because we didn't notice the aexp-self-reference.
				// Then when it was worked around with aexpRef, it was effectively full-on left-recursion
				// and in fact that's our situation now. It needs to be refactored
				// probably something like
				// auto aexp = (my entire current expression except last choice) && Optional(lbrace && fbind+ && rbrace)
				//  with a little isntqcon action thrown in.
				//  but for now I want to debug let forms.
				;
	aexpRef->resolve(aexp);
	//auto fexp = WSequence(Optional(fexpRef),aexp);
	auto fexp = Repeat1("fexp",SkipWhite(aexp));
	fexpRef->resolve(fexp);
	auto exclam = make_shared<tch<'!'>>();
	auto fielddecl = SequenceName("fielddecl") && vars && colonColon && (type || (exclam && atype));
	auto constr = ChoicesName("constrs")
						|| SequenceName("con atype") && con && Repeat0(SkipWhite(Optional(exclam)) && SkipWhite(atype))
						|| SequenceName("type conop type") && (btype || (exclam && atype)) && conop && (btype || (exclam && atype))
						|| SequenceName("con {fields}") && con && lbrace && RLoop("fielddecl,",fielddecl, comma) && rbrace
						;
	auto constrs = RLoop("constrs",constr,vertbar);
	auto simpletype = SequenceName("simpletype") && tycon && Repeat1("tyvars", SkipWhite(tyvar));
	auto newconstr = ChoicesName("newconstr")
						|| SequenceName("con atype") && con && atype
						|| SequenceName("con{var::type}") && con && lbrace && var && colonColon && type && rbrace
						;
	auto simpleclass = SequenceName("simpleclass") && qtycls && tyvar ;
	auto scontext = ChoicesName("scontext")
						|| simpleclass
						|| SequenceName("(simpleclass,)")&& lparen && RLoop("simpleclass,", simpleclass, comma) && rparen
						;
	auto funlhsvar = ChoicesName("funlhsvar") || funlhs || var;
	auto cdecl_  = ChoicesName("cdecl")
						|| gendecl
						|| SequenceName("(funlhs|var) rhs") && funlhsvar && rhs
						;
	auto cdecls = SequenceName("(cdecls,)") && lparen && RLoop("cdecl,",cdecl_,comma) && rparen ;
	auto inst = ChoicesName("inst")
						|| gtycon
						|| SequenceName("(gtycon tyvar*)")&& lparen && gtycon && Repeat0(tyvar) && rparen
						|| SequenceName("(tyvar,)")&&lparen && RLoop("tyvar,",tyvar,comma) && rparen
						|| SequenceName("[tyvar]")&& lbracket && tyvar && rbracket
						|| SequenceName("(tyvar->tyvar)") && lparen && tyvar && fnTo && tyvar && rparen
						;
	auto idecl = SequenceName("(funlhs|var)?rhs") && Optional(funlhsvar) && rhs;
	auto idecls = SequenceName("idecls") && lbrace && RLoop("idecl,",idecl, comma) && rbrace ;
	auto optString = Optional(make_shared<Quoted<'"'>>());
	auto impent = optString;
	auto expent = optString;
	auto safety = ChoicesName("safety") || safeKw || unsafeKw;
	auto callconv = ChoicesName("callconv") || ccallKw || stdcallKw || cplusplusKw || jvmKw || dotnetKw ;
	auto fatype = SequenceName("fatype") && qtycon && Repeat0(atype) ;
	auto frtype = fatype
				|| unit 	
				;
	auto ftypeRef = make_shared<ParserReference>();
	auto ftype = ChoicesName("ftype")
					|| frtype
					|| SequenceName("fatype->ftype") && fatype && fnTo && ftypeRef
					;
	ftypeRef->resolve(ftype);
  
	auto fdecl = ChoicesName("fdecl")
						|| SequenceName("import") && importKw && callconv && Optional(safety) && impent && var && colonColon && ftype
						|| SequenceName("export") && exportKw && callconv && expent && var && colonColon && ftype
						;
	auto topdecl = ChoicesName("topdecl")
						|| SequenceName("type") && typeKw && simpletype && equals && type
						|| SequenceName("data") && dataKw && Optional(context && contextTo) && simpletype && Optional(equals && constrs) && Optional(derivingKw)
						|| SequenceName("newtype") && newtypeKw && Optional(context && contextTo) && simpletype && equals && newconstr && Optional(derivingKw)
						|| SequenceName("class") && classKw && Optional(scontext && contextTo) && tycls && tyvar && Optional(whereKw && cdecls)
						|| SequenceName("instance") && instanceKw && Optional(scontext && contextTo) && qtycls && inst && Optional(whereKw && idecls)
						|| SequenceName("default") && defaultKw && lparen && RLoop("type,",type,comma) && rparen
						|| SequenceName("foreign") && foreignKw && fdecl
						|| decl
						;
	auto topdecls = RLoop("topdecls",topdecl,semicolon);
	declsRef->resolve(decls);
	auto cname = ChoicesName("cname") ||var ||con;
	auto import = ChoicesName("import")
						|| var
						|| (SequenceName("tyconwcnames") && tycon && Optional(ChoicesName("dotsorcnames") || (SequenceName("(..)") && lparen && dotdot && rparen)
								|| (SequenceName("(cnames)") && lparen && RLoop("cnames",cname,comma) && rparen)))
						|| (SequenceName("tyclswvars") && tycls && Optional(ChoicesName("dotsorvars") || (SequenceName("(..)") && lparen && dotdot && rparen)
								|| (SequenceName("(vars)") && lparen && RLoop("vars",var,comma) && rparen)))
						;
	auto impspec = (SequenceName("impspec") && Optional(hidingKw) && lparen && RLoop("importComma",import,comma) && rparen);
	auto impdecl = (SequenceName("impdecl") && importKw && Optional(qualifiedKw)&& moduleID && Optional((SequenceName("as modopt") && asKw && moduleID)) && Optional(impspec));
	auto imports = RLoop2("imports", impdecl, semicolon);
	auto body = ChoicesName("body")
						|| (SequenceName("bodyOfDeclsWithImports") && lbrace && imports && /*semicolon && */ topdecls && rbrace)
		                || (SequenceName("bodyOfImports") && lbrace && imports && rbrace)
						|| (SequenceName("bodyOfTopDecls") && lbrace && topdecls && rbrace)
						;
	auto module = (SequenceName("module") && Optional(moduleKw && moduleID && exports && whereKw) && body);
	cout << "funlhs examination " << funlhs->name << " id " << funlhs->parser_id << " has " << funlhs->choices.size() << " parsers " << endl;
	for (size_t c =0; c<funlhs->choices.size(); ++c)
		cout << "funlhs examination " << funlhs->name << " id " << funlhs->parser_id << " parser["<<c<<"] "
				<< funlhs->choices[c]->name << endl;
	ParseStatePtr st = make_shared<ParseState>(input);
	ParseResultPtr r = (*module)(st);
	if (r)
	{
		cout << "r [" << *r->getState() << ']' << endl;
		cout << "AST " << *r->getAST();
		cout << endl;
	}
	else
	{
		cout << "r==null" << endl;
	}
}
class reservedWord : public ParserBase
{
public:
	reservedWord() {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t i=0;
		while (start->at(i)!=' ' && start->at(i)!='\t' && start->at(i)!='\n' && start->at(i)!='\r' && start->at(i)!=0)
			++i;
		//if (i!=0)
		//	cout << __PRETTY_FUNCTION__ << ' ' << "trying [" << start->substr(0,i) << ']'<< endl;
		if (reservedid(start->substr(0,i)))
		{
			//cout << __PRETTY_FUNCTION__ << ' ' << "advancing " << *(start->from(i)) << endl;
			return make_shared<ParseResult>(start->from(i), make_shared<StringAST>(start->substr(0,i)));
		}
		return ParseResultPtr();
	}
};
class reservedOperator : public ParserBase
{
public:
	reservedOperator() {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t i=0;
		while (asciiSymbolChar(start->at(i)))
			++i;
		if (i==0)
			return ParseResultPtr();
		//cout << __PRETTY_FUNCTION__ << ' ' << "trying [" << start->substr(0,i) << ']'<< endl;
		if (reservedOp(start->substr(0,i)))
		{
			//cout << __PRETTY_FUNCTION__ << ' ' << "advancing " << *(start->from(i)) << endl;
			return make_shared<ParseResult>(start->from(i), make_shared<StringAST>(start->substr(0,i)));
		}
		return ParseResultPtr();
	}
};
class specialChar : public ParserBase
{
public:
	specialChar() {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		switch (start->at(0))
		{
		case '(': case ')': case ',': case '[': case ']': case '`': case '{': case '}':
			return make_shared<ParseResult>(start->from(1), make_shared<StringAST>(start->substr(0,1)));
		default:
			return ParseResultPtr();
		}
	}
};
class whiteSpace : public ParserBase
{
public:
	whiteSpace() {}
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t i=0;
		while (start->at(i)==' ' || start->at(i)=='\t' || start->at(i)=='\n' || start->at(i)=='\r')
			++i;
		return make_shared<ParseResult>(start->from(i), make_shared<StringAST>(start->substr(0,i)));
	}
};
class layoutTracker
{
public:
	layoutTracker()
	: column(0)
	, line(0)
	, lastNewline(0)
	,lastFirstBlank(0)
	,blanksOnly(true)
	,moduleStart(true)
	,kwNeedsBrace(false)
	,yield(' ')
	{}
	int column;
	int line;
	int lastNewline;
	int lastFirstBlank;
	bool blanksOnly;
	bool moduleStart;
	bool kwNeedsBrace;
	int yield;
	struct LayoutToken
	{
		enum Type { Brace, Angle, Text, Space };
		Type type;
		string text;
		int column;
	};
	std::list<LayoutToken> pendingInput;
	std::stack<int> indents;
	void post(const string& token)
	{
		if (token[0]!=' ' && token[0]!='\t' && token[0]!='\r' && token[0] != '\n')
		{
			if (kwNeedsBrace && token!="{")
			{
				LayoutToken lt;
				lt.type = LayoutToken::Brace;
				lt.column = column;
				pendingInput.push_back(lt);
				kwNeedsBrace = false;
			}
			if (token=="let" || token=="where" || token=="do"||token=="of")
			{
				kwNeedsBrace=true;
				if (blanksOnly)
				{
					LayoutToken lt;
					lt.type = LayoutToken::Angle;
					lt.column = column;
					pendingInput.push_back(lt);
				}
			}
			else if (token=="{" && !moduleStart)
			{
				kwNeedsBrace=false;
				if (blanksOnly)
				{
					LayoutToken lt;
					lt.type = LayoutToken::Angle;
					lt.column = column;
					pendingInput.push_back(lt);
				}
			}
			else if (moduleStart && (token!="{"&&token!="module"))
			{
				LayoutToken lt;
				lt.type = LayoutToken::Brace;
				lt.column = column;
				pendingInput.push_back(lt);
				moduleStart = false;
			}
			else
			{
				if (blanksOnly)
				{
					LayoutToken lt;
					lt.type = LayoutToken::Angle;
					lt.column = column;
					pendingInput.push_back(lt);
				}
			}
			LayoutToken lt;
			lt.type = LayoutToken::Text;
			lt.column = column;
			lt.text = token;
			pendingInput.push_back(lt);
			blanksOnly = false;
			lastFirstBlank = column;
			column += token.size();
		}
		else
		{
			int prevColumn = column;
			for (size_t i=0; i<token.size(); ++i)
			{
				int c = token[i];
				switch (c)
				{
				case '\r': // These should just be no-ops
					break;
				case '\n':
					lastNewline = column;
					column = 1;
					++line;
					blanksOnly = true;
					break;
				case '\t':
					column = column + 8-(column%8);
					break;
				case ' ':
					column++;
					break;
				default:
					cout << "****** " << endl;
					blanksOnly = false;
					lastFirstBlank = column;
					column++;
					break;
				}
			}
			LayoutToken lt;
			lt.type = LayoutToken::Space;
			lt.column = prevColumn;
			lt.text = token;
			pendingInput.push_back(lt);
		}
	}
	bool empty() const {
		return false;
	}
	string next()
	{
		//cout << "What's next "; status(); cout << endl;
		if (pendingInput.size()>0 && pendingInput.front().type == LayoutToken::Angle)
		{
			int n = pendingInput.front().column;
			int m = indents.top();
			//cout << "Official handling of <" << n << "> vs indent m "<< m << endl;
			if (n==m)
			{
				pendingInput.pop_front();
				//cout << "Returning ;" << endl;
				return ";";
			}
			if (n<m)
			{
				indents.pop();
				//cout << "Returning }" << endl;
				return "}";
			}
		}
		if (pendingInput.size()>0 && pendingInput.front().type==LayoutToken::Brace)
		{
			int n = pendingInput.front().column;
			int m = -1;
			if (indents.size()) m = indents.top();
			if (n > m)
			{
				pendingInput.pop_front();
				indents.push(n);
				//cout << "Returning {" << endl;
				return "{";
			}
			if (indents.empty())
			{
				pendingInput.pop_front();
				indents.push(n);
				//cout << "Returning {" << endl;
				return "{";
			}
			pendingInput.front().type = LayoutToken::Angle;
			//cout << "Returning {}" << endl;
			return "{}";
		}
		if (pendingInput.size()>0 && pendingInput.front().type==LayoutToken::Text)
		{
			int m = indents.top();
			if (pendingInput.front().text=="}" && m==0)
			{
				indents.pop();
				pendingInput.pop_front();
				//cout << "Returning }" << endl;
				return "}";
			}
			if (pendingInput.front().text=="}")
			{
				cout << " ---------------------- error" << endl;
			}
			if (pendingInput.front().text=="{")
			{
				indents.push(0);
				pendingInput.pop_front();
				//cout << "Returning {" << endl;
				return "{";
			}
			// There's also the parse error case but I'm not parsing
			string r = pendingInput.front().text;
			pendingInput.pop_front();
			//cout << "Returning " << r << endl;
			return r;
		}
		if (pendingInput.size()>0 && pendingInput.front().type == LayoutToken::Space)
		{
			string r = pendingInput.front().text;
			pendingInput.pop_front();
			//cout << "Returning " << r << endl;
			return r;
		}
		if (pendingInput.empty() && indents.empty())
		{
			//cout << "Returning ''" << endl;
			return "";
		}
		if (pendingInput.empty() && indents.top()!=0)
		{
			indents.pop();
			//cout << "Returning }" << endl;
			return "}";
		}
		cout << "Fell off the end why? ";
		status();
		cout << endl;
		return "";
	}
	void status()
	{
		if (pendingInput.size())
		{
			cout << " n ";
			if (pendingInput.front().type==LayoutToken::Angle)
				cout << "<" << pendingInput.front().column << ">";
			else if (pendingInput.front().type == LayoutToken::Brace)
				cout << "{" << pendingInput.front().column << "}";
			else if (pendingInput.front().type == LayoutToken::Text)
				cout << pendingInput.front().column << ' ' << pendingInput.front().text.substr(0,3);
			else
				cout << pendingInput.front().column << ' ' << pendingInput.front().text.size() << " ws chrs";
			cout << " ";
		}
		else
		{
			cout << " n 0 ";
		}
		if (indents.size())
		{
			cout << " m " << indents.top();
		}
	}
};
string layout(std::istream& file)
{
	string input;
	string line;
	while (file)
	{
		line="";
		std::getline(file,line);
		if (line.size())
		{
			input += '\n';
			input += line;
		}
	}
	cout << "layout [\n" << input << "\n]"<< endl;
	auto tp = make_shared<VarID>()
			|| make_shared<ConID>()
			|| make_shared<VarSym>()
			|| make_shared<ConSym>()
			|| make_shared<NUM>()
			|| make_shared<Quoted<'"'>>()
			|| make_shared<reservedWord>()
			|| make_shared<reservedOperator>()
			|| make_shared<specialChar>()
			|| make_shared<whiteSpace>()
			;
	ParseStatePtr start = make_shared<ParseState>(input);
	string results;
	layoutTracker tracker;
	while (start->at(0)!=0)
	{
		ParseResultPtr token = (*tp)(start);
		if (!token)
		{
			cout << "No token" << endl;
			break;
		}
		if (token->getAST()->to_string().size()==0)
		{
			cout << "zero-length token " << *(token->getState()) << endl;
			break;
		}
		tracker.post(token->getAST()->to_string());
		string yield = tracker.next();
		if (yield != "")
			results += yield;
		start = token->getState();
		if (start->at(0)==0)
			cout << "Nothing left" << endl;
	}
	while (tracker.pendingInput.size()||tracker.indents.size())
	{
		string nxt = tracker.next();
		if (nxt[0]==' '||nxt[0]=='\t'||nxt[0]=='\r'||nxt[0]=='\n')
			results += '\n'+nxt;
		else
			results += nxt;
	}
	return results;
}
extern bool showFails;
extern bool showRemainder;
int main(int argc, const char* argv[]) {
	cout << "PEGHS starts" << endl;
	std::ifstream file;
	if (argc > 1)
	{
		file.open(argv[1]);
		string laidout = layout(file);
		cout << "laid out " << laidout << endl;
		ParseState::reset();
		hs(laidout);
		return 0;
	}
	showFails = true;
	showRemainder = true;
	string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4=(\\a->a+1) 5; circumference :: a->a; circumference r=2*pi*r; party = let { a=5 } in a }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4=(\\a->a+1) 5; circumference r=2*pi*r; party = let a=5 in a }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4=(\\a->a+1) 5; circumference r=2*pi*r }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4= \\a->a+1; circumference r=2*pi*r }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; circumference r=2*pi*r }");
	hs(input);

	tests();
	return 0;
}
