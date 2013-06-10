#include "peg.h"
#include "ParseState.h"
#include "Ast.h"

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
			return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
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
			return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
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
			return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
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
			return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
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
			return make_shared<ParseResult>(start->from(ic), new IdentifierAST(start->substr(0, ic)));
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
		return make_shared<ParseResult>(start->from(ic), new StringAST(start->substr(0, ic)));
	}
};


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
	auto exportItem = make_shared<Choices>("exportItem") || varID || conID;
	auto exportComma = RLoop("exportComma",exportItem,comma);
	auto exports = (make_shared<WSequenceN>("exports") && lparen && exportComma && rparen);
	auto varsym = make_shared<VarSym>();// varsym;
	auto consym = make_shared<ConSym>();// consym;
	auto hliteral = make_shared<NUM>() || make_shared<Quoted<'\''>>() || make_shared<Quoted<'"'>>();
	hliteral->name = "hliteral";
	//auto varop = Choice("varop",varsym,WSequence("varIDop",backTick,varID,backTick));
	auto varop = varsym || (backTick && varID && backTick);
	varop->name = "varop";
	//auto conop = Choice("conop",consym,WSequence("conIDop",backTick,conID,backTick));
	auto conop = make_shared<Choices>("conop") || consym || (make_shared<WSequenceN>("conIDop") && backTick && conID && backTick);
	auto qop = make_shared<Choices>("qop")|| varop || conop;
	auto unit = (make_shared<WSequenceN>("unit") && lparen && rparen);
	auto elist = (make_shared<WSequenceN>("[]") && lbracket && rbracket);
	auto etuple = (make_shared<WSequenceN>("etuple") && lparen && Repeat1(",",comma) && rparen);
	auto efn = (make_shared<WSequenceN>("efn") && lparen && fnTo && rparen);
	auto qconsym = consym;
	auto gconsym = make_shared<Choices>("gconsym") || colon || qconsym;
	auto qcon = make_shared<Choices>("qcon") || conID || (make_shared<WSequenceN>("(gconsym)") && lparen && gconsym &&rparen);
	auto gcon = make_shared<Choices>("gcon") || unit || elist || etuple || qcon;
	auto qvarid = varID;
	auto qvarsym = varsym;
	auto qvar = make_shared<Choices>("qvar") || qvarid || (make_shared<WSequenceN>("(qvarsym)") && lparen && qvarsym && rparen);
	auto expRef = make_shared<ParserReference>();
	auto infixexpRef = make_shared<ParserReference>();
	auto expRefComma = RLoop("expRefComma",expRef,comma);
	auto fbind = (make_shared<WSequenceN>("fbind") && qvar && equals && expRef);
	auto fexpRef = make_shared<ParserReference>();
	auto aexpRef = make_shared<ParserReference>();
	auto declsRef = make_shared<ParserReference>();
	auto patRef = make_shared<ParserReference>();// patRef;
	auto wheres = (make_shared<WSequenceN>("where decls") && whereKw && declsRef);
	auto guard = make_shared<Choices>("guard") 
								|| (make_shared<WSequenceN>("pat->infixexp") && patRef && patFrom && infixexpRef)
								|| (make_shared<WSequenceN>("let decls") && letKw && declsRef)
								|| infixexpRef
						        ;
	auto guards = (make_shared<WSequenceN>("guards") && make_shared<tch<'|'>>() && Repeat1("guard+",guard));
	auto gdpat = Repeat1("gdpatr",(make_shared<WSequenceN>("gdpat") && guards && fnTo && expRef));
	auto alt = make_shared<Choices>("alt") || (make_shared<WSequenceN>("pat<-exp") && patRef && patFrom && expRef && Optional(wheres))
										   || (make_shared<WSequenceN>("pat gdpat") && patRef && gdpat && Optional(wheres))
											;
	auto alts = RLoop2("altSemiAlt",alt,semicolon);
	auto stmt = make_shared<Choices>("stmt") 
							||(make_shared<WSequenceN>("stmt:exp") && expRef && semicolon)
							||(make_shared<WSequenceN>("stmt:pat->exp") && patRef && fnTo && expRef && semicolon)
							||(make_shared<WSequenceN>("stmt:let decls") && letKw && declsRef && semicolon)
							||semicolon
							;
	auto stmts = (make_shared<WSequenceN>("stmts") && Repeat1("stmt+",SkipWhite(stmt)) && expRef && Optional(semicolon));
	auto lexp = make_shared<Choices>("lexp")
						||(make_shared<WSequenceN>("lambdaabs") && make_shared<tch<'\\'>>() && Repeat1("\\aexp+",SkipWhite(aexpRef)) && fnTo && expRef)
						||(make_shared<Choices>("lexp-kw")
							|| (make_shared<WSequenceN>("let") && letKw && declsRef && inKw && expRef)
							||(make_shared<WSequenceN>("if") && ifKw && expRef && Optional(semicolon) && (make_shared<WSequenceN>("ifthen") && thenKw && expRef && Optional(semicolon)) && (make_shared<WSequenceN>("ifelse") && elseKw && expRef))
							||(make_shared<WSequenceN>("case") && caseKw && expRef && ofKw && alts)
							||(make_shared<WSequenceN>("do") && doKw && lbrace && stmts && rbrace))
						||fexpRef
						;
	auto infixexp = make_shared<Choices>("infixexp")
						|| (make_shared<WSequenceN>("lexp,qop,infixexp") && lexp && qop && infixexpRef)
						|| (make_shared<WSequenceN>("-infixexp") && minus && infixexpRef) 
						|| lexp
						;
	auto typeRef = make_shared<ParserReference>();
	auto atypeRef = make_shared<ParserReference>();
	auto tycls = conID;
	auto qtycls = tycls;
	auto tyvar = varID;
	auto klass = make_shared<Choices>("klass")
						||(make_shared<WSequenceN>("klassNulTyVar") && qtycls && tyvar)
						||(make_shared<WSequenceN>("klassMultiTyVar") && qtycls && lparen && tyvar && Repeat1("",atypeRef) && rparen)
						;
	auto context = make_shared<Choices>("context")
						|| klass
						|| (make_shared<WSequenceN>("(klass ,klass*)") && lparen && RLoop("klassComma",klass,comma) && rparen)
						;
	auto exp = (make_shared<WSequenceN>("exp") && infixexpRef && Optional((make_shared<WSequenceN>("typesig") && colonColon && Optional((make_shared<WSequenceN>("context") && context && contextTo)) && typeRef))) ;
	infixexpRef->resolve(infixexp);
	expRef->resolve(exp);


	auto rhs = (make_shared<WSequenceN>("rhs") && equals && exp);
	auto tycon = conID;
	auto qtycon = tycon;
	auto gtycon = make_shared<Choices>("gtycon") || qtycon || unit || elist || efn || etuple;
	auto var = make_shared<Choices>("var") 
						|| varID 
						|| (make_shared<WSequenceN>("(varsym)") && lparen && varsym && rparen)
						;
	auto con = make_shared<Choices>("con")
						||conID
						||(make_shared<WSequenceN>("(consym)") && lparen && consym && rparen)
						;
	auto tupleCon = (make_shared<WSequenceN>("tupleCon") && lparen && RLoop("type,",typeRef,comma) && rparen);
	auto listCon = (make_shared<WSequenceN>("listCon") && lbracket && typeRef && rbracket);
	auto parType = (make_shared<WSequenceN>("parType") && lparen && typeRef && rparen);
	auto atype = make_shared<Choices>("atype")
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
	auto type = btype; 
	typeRef->resolve(type);
	auto gendecl = (make_shared<WSequenceN>("gendecl") && Repeat1("varID",SkipWhite(varID)) && colonColon && type);
	auto apatRef = make_shared<ParserReference>();
	auto fpat = (make_shared<WSequenceN>("fpat") && varID && equals && patRef);
	auto patternComma = RLoop("patternComma",patRef,comma);
	auto patterns = make_shared<Choices>("patterns")
						|| (make_shared<WSequenceN>("(pat)") && lparen && patRef && rparen)
						|| (make_shared<WSequenceN>("(pat,pat)") && lparen && patternComma && rparen)
						|| (make_shared<WSequenceN>("[pat,pat]") && lbracket && patternComma && lbracket)
						|| (make_shared<WSequenceN>("~apat") && tilde && apatRef)
						;
	auto apat = make_shared<Choices>("apat") 
						|| (make_shared<WSequenceN>("var@apat") && var && Optional((make_shared<WSequenceN>("@apat") && atSign && apatRef)))
						|| gcon
						|| (make_shared<WSequenceN>("qcon{fpat,fpat}") && qcon && lbrace && Optional(RLoop("fpat,",fpat,comma)) && rbrace)
						|| wildcard
						|| patterns
						;
	apatRef->resolve(apat);
	auto funlhsRef = make_shared<ParserReference>();// funlhsRef;
	auto funlhs = make_shared<Choices>("funlhs") 
						|| (make_shared<WSequenceN>("var apat+") && varID && Repeat1("apat+",apat))
						|| (make_shared<WSequenceN>("pat,varop,pat") && patRef && varop && patRef)
						|| (make_shared<WSequenceN>("(funlhs)apat") && lparen && funlhsRef && rparen && Repeat1("apat+",apat))
						;
	funlhsRef->resolve(funlhs);
	auto qconop = make_shared<Choices>("qconop") 
						|| gconsym 
						|| (make_shared<WSequenceN>("`conID`") && backTick && conID && backTick)
						;
	auto lpat = make_shared<Choices>("lpat") 
						||apat
						||(make_shared<WSequenceN>("gcon apat+") && gcon && Repeat1("apat+",apat))
						;
	auto pat = make_shared<Choices>("pat") 
						||(make_shared<WSequenceN>("lpat,qconop,pat") && lpat && qconop && patRef)
						||lpat
						;
	patRef->resolve(pat);
	auto decl = make_shared<Choices>("decl") 
						||gendecl
						||(make_shared<WSequenceN>("funlhs|pat rhs") && (make_shared<Choices>("funlhs|pat") ||funlhs ||pat) && rhs)
						;
	auto decls = RLoop("decls",decl,semicolon);
	auto qual = make_shared<Choices>("qual")
				|| pat && patFrom && expRef
				|| letKw && decls
				|| expRef
				;
	auto aexp = varID  
				|| gcon
				|| hliteral
				|| (make_shared<WSequenceN>("(exp)") && lparen && expRef && rparen)
				|| (make_shared<WSequenceN>("(exp,exp)") && lparen && expRefComma && rparen)
				|| (make_shared<WSequenceN>("[exp,exp]") && lbracket && expRefComma && rbracket)
				||(make_shared<WSequenceN>("[exp,exp..exp]") && lbracket && expRef && Optional((make_shared<WSequenceN>(",exp") && comma && expRef)) && dotdot && expRef && rbracket)
				||(make_shared<WSequenceN>("[exp|qual,..]") && lbracket && expRef && vertbar && RLoop("qual,",qual,comma) && rbracket)
				||(make_shared<WSequenceN>("(infixexp qop)") && lparen && infixexpRef && qop && rparen)
				||(make_shared<WSequenceN>("(qop<-> infixexp)") && lparen && qop && infixexpRef && rparen)
				||(make_shared<WSequenceN>("qcon{fbind+}") && qcon && lbrace && Repeat1("fbind+",fbind) && rbrace)
				;
	aexp->name = "aexp";
	aexpRef->resolve(aexp);
	//auto fexp = WSequence(Optional(fexpRef),aexp);
	auto fexp = Repeat1("fexp",SkipWhite(aexp));
	fexpRef->resolve(fexp);
	auto exclam = make_shared<tch<'!'>>();
	auto vars = RLoop("vars",var,comma);
	auto fielddecl = vars && colonColon && (type || (exclam && atype));
	auto constr = make_shared<Choices>("constrs")
						|| make_shared<WSequenceN>("con atype") && con && Repeat0(Optional(exclam) && atype)
						|| make_shared<WSequenceN>("type conop type") && (btype || (exclam && atype)) && conop && (btype || (exclam && atype))
						|| make_shared<WSequenceN>("con {fields}") && con && lbrace && RLoop("fielddecl,",fielddecl, comma) && rbrace
						;
	auto constrs = RLoop("constrs",constr,vertbar);
	auto simpletype = make_shared<WSequenceN>("simpletype") && tycon && Repeat1("tyvars", tyvar);
	auto newconstr = make_shared<Choices>("newconstr") 
						|| con && atype
						|| con && lbrace && var && colonColon && type && rbrace
						;
	auto simpleclass = make_shared<WSequenceN>("simpleclass") && qtycls && tyvar ;
	auto scontext = make_shared<Choices>("scontext")
						|| simpleclass
						|| lparen && RLoop("simpleclass,", simpleclass, comma) && rparen
						;
	auto cdecl_  = make_shared<Choices>("cdecl")
						|| gendecl
						|| lparen && (funlhs || var ) && rhs
						;
	auto cdecls = make_shared<WSequenceN>("cdecls") && lparen && RLoop("cdecl,",cdecl_,comma) && rparen ;
	auto inst = make_shared<Choices>("inst")
						|| gtycon
						|| lparen && gtycon && Repeat0(tyvar) && rparen
						|| lparen && RLoop("tyvar,",tyvar,comma) && rparen
						|| lbracket && tyvar && rbracket
						|| lparen && tyvar && fnTo && tyvar && rparen
						;
	auto idecl = Optional((funlhs || var) && rhs);
	auto idecls = make_shared<WSequenceN>("idecls") && lbrace && RLoop("idecl,",idecl, comma) && rbrace ;
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
  
	auto fdecl = make_shared<Choices>("fdecl")
						|| importKw && callconv && Optional(safety) && impent && var && colonColon && ftype
						|| exportKw && callconv && expent && var && colonColon && ftype
						;
	auto topdecl = make_shared<Choices>("topdecl")
						|| make_shared<WSequenceN>("type") && typeKw && simpletype && equals && type
						|| make_shared<WSequenceN>("data") && dataKw && Optional(context && contextTo) && simpletype && Optional(equals && constrs) && Optional(derivingKw)
						|| make_shared<WSequenceN>("newtype") && newtypeKw && Optional(context && contextTo) && simpletype && equals && newconstr && Optional(derivingKw)
						|| make_shared<WSequenceN>("class") && classKw && Optional(scontext && contextTo) && tycls && tyvar && Optional(whereKw && cdecls)
						|| make_shared<WSequenceN>("instance") && instanceKw && Optional(scontext && contextTo) && qtycls && inst && Optional(whereKw && idecls)
						|| make_shared<WSequenceN>("default") && defaultKw && lparen && RLoop("type,",type,comma) && rparen
						|| make_shared<WSequenceN>("foreign") && foreignKw && fdecl
						|| decl
						;
	auto topdecls = RLoop("topdecls",topdecl,semicolon);
	declsRef->resolve(decls);
	auto cname = make_shared<Choices>("cname") ||var ||con;
	auto import = make_shared<Choices>("import") 
						|| var
						|| (make_shared<WSequenceN>("tyconwcnames") && tycon && Optional(make_shared<Choices>("dotsorcnames") || (make_shared<WSequenceN>("(..)") && lparen && dotdot && rparen)
								|| (make_shared<WSequenceN>("(cnames)") && lparen && RLoop("cnames",cname,comma) && rparen)))
						|| (make_shared<WSequenceN>("tyclswvars") && tycls && Optional(make_shared<Choices>("dotsorvars") || (make_shared<WSequenceN>("(..)") && lparen && dotdot && rparen)
								|| (make_shared<WSequenceN>("(vars)") && lparen && RLoop("vars",var,comma) && rparen)))
						;
	auto impspec = (make_shared<WSequenceN>("impspec") && Optional(hidingKw) && lparen && RLoop("importComma",import,comma) && rparen);
	auto impdecl = (make_shared<WSequenceN>("impdecl") && importKw && Optional(qualifiedKw)&& moduleID && Optional((make_shared<WSequenceN>("as modopt") && asKw && moduleID)) && Optional(impspec));
	auto imports = RLoop2("imports", impdecl, semicolon);
	auto body = make_shared<Choices>("body") 
						|| (make_shared<WSequenceN>("bodyOfDeclsWithImports") && lbrace && imports && /*semicolon && */ decls && rbrace)
		                || (make_shared<WSequenceN>("bodyOfImports") && lbrace && imports && rbrace)
						|| (make_shared<WSequenceN>("bodyOfTopDecls") && lbrace && topdecls && rbrace)
						;
	auto module = (make_shared<WSequenceN>("module") && moduleKw && moduleID && exports && whereKw && body);
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
	hs();

	tests();
	return 0;
}
