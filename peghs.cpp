#include "peg.h"
#include "ParseState.h"
#include "Ast.h"
#include "Action.h"

#include <utility>
#include <memory>
#include <cctype>
#include <string>
#include <iostream>

using std::string;
using std::make_shared;
using std::shared_ptr;
using std::cout;
using std::endl;

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
				return ParseResultPtr();
			return make_shared<ParseResult>(start->from(ic), std::make_shared<IdentifierAST>(start->substr(0, ic)));
		}
		return ParseResultPtr();
	}
};
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
	ParseResultPtr parse(const ParseStatePtr& start)
	{
		size_t ic = 0;
		if (start->length() > 0 && start->at(0)==':')
		{
			++ic;
			while (start->length() > ic && (asciiSymbolChar(start->at(0))))
				++ic;
			if (reservedOp(start->substr(0,ic)))
				return ParseResultPtr();
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
	Quoted() {}
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
			++ic;
		}
		if (start->at(ic) != quote)
			return ParseResultPtr();
		return make_shared<ParseResult>(start->from(ic), std::make_shared<StringAST>(start->substr(0, ic)));
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
extern char type_kw[] = "type";
extern char data_kw[] = "data";
extern char newtype_kw[] = "newtype";
extern char class_kw[] = "class";
extern char instance_kw[] = "instance";
extern char default_kw[] = "default";
extern char foreign_kw[] = "foreign";
extern char deriving_kw[] = "deriving";
extern char safe_kw[] = "safe";
extern char unsafe_kw[] = "unsafe";
extern char ccall_kw[] = "ccall";
extern char stdcall_kw[] = "stdcall";
extern char cplusplus_kw[] = "cplusplus";
extern char jvm_kw[] = "jvm";
extern char dotnet_kw[] = "dotnet_kw";
extern char export_kw[] = "export";
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
	auto colonColon = make_shared<ColonColon>();// colonColon;
	auto fnTo = make_shared<FnTo>();// fnTo;
	auto patFrom = make_shared<PatFrom>();
	auto contextTo = make_shared<ContextTo>();
	auto dotdot = make_shared<DotDot>();
	auto exportItem = ChoicesName("exportItem") || varID || conID;
	auto exportComma = RLoop("exportComma",exportItem,comma);
	auto exports = (SequenceName("exports") && lparen && exportComma && rparen);
	auto varsym = make_shared<VarSym>();// varsym;
	auto consym = make_shared<ConSym>();// consym;
	auto hliteral = make_shared<NUM>() || make_shared<Quoted<'\''>>() || make_shared<Quoted<'"'>>();
	hliteral->name = "hliteral";
	//auto varop = Choice("varop",varsym,WSequence("varIDop",backTick,varID,backTick));
	auto varop = varsym || (backTick && varID && backTick);
	varop->name = "varop";
	//auto conop = Choice("conop",consym,WSequence("conIDop",backTick,conID,backTick));
	auto conop = ChoicesName("conop") || consym || (SequenceName("conIDop") && backTick && conID && backTick);
	auto qop = ChoicesName("qop")|| varop || conop;
	auto unit = (SequenceName("unit") && lparen && rparen);
	auto elist = (SequenceName("[]") && lbracket && rbracket);
	auto etuple = (SequenceName("etuple") && lparen && Repeat1(",",comma) && rparen);
	auto efn = (SequenceName("efn") && lparen && fnTo && rparen);
	auto qconsym = consym;
	auto gconsym = ChoicesName("gconsym") || colon || qconsym;
	auto qcon = ChoicesName("qcon") || conID || (SequenceName("(gconsym)") && lparen && gconsym &&rparen);
	auto gcon = ChoicesName("gcon") || unit || elist || etuple || qcon;
	auto qvarid = varID;
	auto qvarsym = varsym;
	auto qvar = ChoicesName("qvar") || qvarid || (SequenceName("(qvarsym)") && lparen && qvarsym && rparen);
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
	auto tycls = conID;
	auto qtycls = tycls;
	auto tyvar = varID;
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
	auto tycon = conID;
	auto qtycon = tycon;
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
	auto gendecl = (SequenceName("gendecl") && Repeat1("varID",SkipWhite(varID)) && colonColon && type);
	auto apatRef = make_shared<ParserReference>();
	auto fpat = (SequenceName("fpat") && varID && equals && patRef);
	auto patternComma = RLoop("patternComma",patRef,comma);
	auto patterns = ChoicesName("patterns")
						|| (SequenceName("(pat)") && lparen && patRef && rparen)
						|| (SequenceName("(pat,pat)") && lparen && patternComma && rparen)
						|| (SequenceName("[pat,pat]") && lbracket && patternComma && lbracket)
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
						|| (SequenceName("var apat+") && varID && Repeat1("apat+",SkipWhite(apat)))
						|| (SequenceName("pat,varop,pat") && patRef && varop && patRef)
						|| (SequenceName("(funlhs)apat") && lparen && funlhsRef && rparen && Repeat1("apat+",SkipWhite(apat)))
						;
	funlhsRef->resolve(funlhs);
	auto qconop = ChoicesName("qconop")
						|| gconsym 
						|| (SequenceName("`conID`") && backTick && conID && backTick)
						;
	auto lpat = ChoicesName("lpat")
						||apat
						||(SequenceName("gcon apat+") && gcon && Repeat1("apat+",SkipWhite(apat)))
						;
	auto pat = ChoicesName("pat")
						||(SequenceName("lpat,qconop,pat") && lpat && qconop && patRef)
						||lpat
						;
	patRef->resolve(pat);
	auto decl = make_shared<ActionCaller>(Decl, ChoicesName("decl")
						||gendecl
						||(SequenceName("funlhs|pat rhs") && (ChoicesName("funlhs|pat") ||funlhs ||pat) && rhs)
						);
	auto decls = RLoop("decls",decl,semicolon);
	auto qual = ChoicesName("qual")
				|| pat && patFrom && expRef
				|| lets
				|| expRef
				;
	auto isntminus = Isnt(minus);
	auto isntqcon = Isnt(qcon);
	auto aexp = varID  
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
	aexp->name = "aexp";
	aexpRef->resolve(aexp);
	//auto fexp = WSequence(Optional(fexpRef),aexp);
	auto fexp = Repeat1("fexp",SkipWhite(aexp));
	fexpRef->resolve(fexp);
	auto exclam = make_shared<tch<'!'>>();
	auto vars = RLoop("vars",var,comma);
	auto fielddecl = vars && colonColon && (type || (exclam && atype));
	auto constr = ChoicesName("constrs")
						|| SequenceName("con atype") && con && Repeat0(SkipWhite(Optional(exclam)) && SkipWhite(atype))
						|| SequenceName("type conop type") && (btype || (exclam && atype)) && conop && (btype || (exclam && atype))
						|| SequenceName("con {fields}") && con && lbrace && RLoop("fielddecl,",fielddecl, comma) && rbrace
						;
	auto constrs = RLoop("constrs",constr,vertbar);
	auto simpletype = SequenceName("simpletype") && tycon && Repeat1("tyvars", SkipWhite(tyvar));
	auto newconstr = ChoicesName("newconstr")
						|| con && atype
						|| con && lbrace && var && colonColon && type && rbrace
						;
	auto simpleclass = SequenceName("simpleclass") && qtycls && tyvar ;
	auto scontext = ChoicesName("scontext")
						|| simpleclass
						|| lparen && RLoop("simpleclass,", simpleclass, comma) && rparen
						;
	auto cdecl_  = ChoicesName("cdecl")
						|| gendecl
						|| (funlhs || var ) && rhs
						;
	auto cdecls = SequenceName("cdecls") && lparen && RLoop("cdecl,",cdecl_,comma) && rparen ;
	auto inst = ChoicesName("inst")
						|| gtycon
						|| lparen && gtycon && Repeat0(tyvar) && rparen
						|| lparen && RLoop("tyvar,",tyvar,comma) && rparen
						|| lbracket && tyvar && rbracket
						|| lparen && tyvar && fnTo && tyvar && rparen
						;
	auto idecl = Optional((funlhs || var) && rhs);
	auto idecls = SequenceName("idecls") && lbrace && RLoop("idecl,",idecl, comma) && rbrace ;
	auto optString = Optional(make_shared<Quoted<'"'>>());
	auto impent = optString;
	auto expent = optString;
	auto safety = safeKw || unsafeKw;
	auto callconv = ccallKw || stdcallKw || cplusplusKw || jvmKw || dotnetKw ;
	auto fatype = qtycon && Repeat0(atype) ;
	auto frtype = fatype
				|| unit 	
				;
	auto ftypeRef = make_shared<ParserReference>();
	auto ftype = frtype
				|| fatype && fnTo && ftypeRef
				;
	ftypeRef->resolve(ftype);
  
	auto fdecl = ChoicesName("fdecl")
						|| importKw && callconv && Optional(safety) && impent && var && colonColon && ftype
						|| exportKw && callconv && expent && var && colonColon && ftype
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
						|| (SequenceName("bodyOfDeclsWithImports") && lbrace && imports && /*semicolon && */ decls && rbrace)
		                || (SequenceName("bodyOfImports") && lbrace && imports && rbrace)
						|| (SequenceName("bodyOfTopDecls") && lbrace && topdecls && rbrace)
						;
	auto module = (SequenceName("module") && moduleKw && moduleID && exports && whereKw && body);
	string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4=(\\a->a+1) 5; circumference :: a->a; circumference r=2*pi*r; party = let a=5 in a }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4=(\\a->a+1) 5; circumference r=2*pi*r; party = let a=5 in a }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4=(\\a->a+1) 5; circumference r=2*pi*r }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; name4= \\a->a+1; circumference r=2*pi*r }");
	//string input("module Kindred (nails, snails, Puppydog.tails) where { import qualified Quality.Goods as Goods (machinery); name1=a; name2=b*c; name3=d<<e>>===f; circumference r=2*pi*r }");
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
int main(int argc, const char* argv[]) {
	cout << "PEGHS starts" << endl;
	hs();

	tests();
	return 0;
}
