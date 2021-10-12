/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * OSF/1 Release 1.0
 */

%token	sySkip
%token	syRoutine
%token	sySimpleRoutine
%token	syCamelotRoutine
%token	sySimpleProcedure
%token	syProcedure
%token	syFunction

%token	sySubsystem
%token	syKernel
%token	syCamelot

%token	syMsgType
%token	syWaitTime
%token	syNoWaitTime
%token	syErrorProc
%token	syServerPrefix
%token	syUserPrefix
%token	syRCSId

%token	syImport
%token	syUImport
%token	sySImport

%token	syIn
%token	syOut
%token	syInOut
%token	syRequestPort
%token	syReplyPort

%token	syType
%token	syArray
%token	syStruct
%token	syOf

%token	syInTran
%token	syOutTran
%token	syDestructor
%token	syCType
%token	syCUserType
%token	syCServerType

%token	syColon
%token	sySemi
%token	syComma
%token	syPlus
%token	syMinus
%token	syStar
%token	syDiv
%token	syLParen
%token	syRParen
%token	syEqual
%token	syCaret
%token	syTilde
%token	syLAngle
%token	syRAngle
%token	syLBrack
%token	syRBrack
%token	syBar

%token	syError			/* lex error */

%token	<number>	syNumber
%token	<symtype>	sySymbolicType
%token	<identifier>	syIdentifier
%token	<string>	syString syQString
%token	<string>	syFileName
%token	<flag>		syIPCFlag

%left	syPlus,syMinus
%left	syStar,syDiv


%type	<statement_kind> ImportIndicant
%type	<number> VarArrayHead ArrayHead StructHead IntExp
%type	<type> NamedTypeSpec TransTypeSpec TypeSpec
%type	<type> BasicTypeSpec PrevTypeSpec ArgumentType
%type	<symtype> PrimIPCType IPCType
%type	<routine> RoutineDecl Routine SimpleRoutine CamelotRoutine
%type	<routine> Procedure SimpleProcedure Function
%type	<direction> Direction
%type	<argument> Argument Arguments ArgumentList
%type	<flag> IPCFlags

%{

#include "lexxer.h"
#include "string.h"
#include "type.h"
#include "routine.h"
#include "statement.h"
#include "global.h"

static char *import_name();

%}

%union
{
    u_int number;
    identifier_t identifier;
    string_t string;
    statement_kind_t statement_kind;
    ipc_type_t *type;
    struct
    {
	u_int innumber;		/* msg_type_name value, when sending */
	string_t instr;
	u_int outnumber;	/* msg_type_name value, when receiving */
	string_t outstr;
	u_int size;		/* 0 means there is no default size */
    } symtype;
    routine_t *routine;
    arg_kind_t direction;
    argument_t *argument;
    ipc_flags_t flag;
}

%%

Statements		:	/* empty */
			|	Statements Statement
			;

Statement		:	Subsystem sySemi
			|	WaitTime sySemi
			|	MsgType sySemi
			|	Error sySemi
			|	ServerPrefix sySemi
			|	UserPrefix sySemi
			|	TypeDecl sySemi
			|	RoutineDecl sySemi
{
    register statement_t *st = stAlloc();

    st->stKind = skRoutine;
    st->stRoutine = $1;
    rtCheckRoutine($1);
    if (BeVerbose)
	rtPrintRoutine($1);
}
			|	sySkip sySemi
				{ rtSkip(); }
			|	Import sySemi
			|	RCSDecl sySemi
			|	sySemi
			|	error sySemi
				{ yyerrok; }
			;

Subsystem		:	SubsystemStart SubsystemMods
				SubsystemName SubsystemBase
{
    if (BeVerbose)
    {
	printf("Subsystem %s: base = %u, IsKernel = %s, IsCamelot = %s\n\n",
	       SubsystemName, SubsystemBase,
	       strbool(IsKernel), strbool(IsCamelot));
    }
}
			;

SubsystemStart		:	sySubsystem
{
    if (SubsystemName != strNULL)
    {
	warn("previous Subsystem decl (of %s) will be ignored", SubsystemName);
	IsCamelot = FALSE;
	IsKernel = FALSE;
	strfree(SubsystemName);
    }
}
			;

SubsystemMods		:	/* empty */
			|	SubsystemMods SubsystemMod
			;

SubsystemMod		:	syKernel
{
    if (IsKernel)
	warn("duplicate Kernel keyword");
    warn("the Kernel subsystem keyword is obsolete");
    IsKernel = TRUE;
}
			|	syCamelot
{
    if (IsCamelot)
	warn("duplicate Camelot keyword");
    IsCamelot = TRUE;
}
			;

SubsystemName		:	syIdentifier	{ SubsystemName = $1; }
			;

SubsystemBase		:	syNumber	{ SubsystemBase = $1; }
			;

MsgType			:	LookString syMsgType syString
{
	if (streql($3,"MSG_TYPE_NORMAL"))
	    MsgType = strNULL;
	else
	    MsgType = $3;
	if (BeVerbose)
	    printf("MsgType %s\n\n",$3);
}
			;

WaitTime		:	LookString syWaitTime syString
{
    WaitTime = $3;
    if (BeVerbose)
	printf("WaitTime %s\n\n", WaitTime);
}
			|	syNoWaitTime
{
    WaitTime = strNULL;
    if (BeVerbose)
	printf("NoWaitTime\n\n");
}
			;

Error			:	syErrorProc syIdentifier
{
    ErrorProc = $2;
    if (BeVerbose)
	printf("ErrorProc %s\n\n", ErrorProc);
}
			;

ServerPrefix		:	syServerPrefix syIdentifier
{
    ServerPrefix = $2;
    if (BeVerbose)
	printf("ServerPrefix %s\n\n", ServerPrefix);
}
			;

UserPrefix		:	syUserPrefix syIdentifier
{
    UserPrefix = $2;
    if (BeVerbose)
	printf("UserPrefix %s\n\n", UserPrefix);
}
			;

Import			:	LookFileName ImportIndicant syFileName
{
    register statement_t *st = stAlloc();
    st->stKind = $2;
    st->stFileName = $3;

    if (BeVerbose)
	printf("%s %s\n\n", import_name($2), $3);
}
			;

ImportIndicant		:	syImport	{ $$ = skImport; }
			|	syUImport	{ $$ = skUImport; }
			|	sySImport	{ $$ = skSImport; }
			;

RCSDecl			:	LookQString syRCSId syQString
{
    if (RCSId != strNULL)
	warn("previous RCS decl will be ignored");
    if (BeVerbose)
	printf("RCSId %s\n\n", $3);
    RCSId = $3;
}
			;

TypeDecl		:	syType NamedTypeSpec
{
    register identifier_t name = $2->itName;

    if (itLookUp(name) != itNULL)
	warn("overriding previous definition of %s", name);
    itInsert(name, $2);
}
			;

NamedTypeSpec		:	syIdentifier syEqual TransTypeSpec
				{ itTypeDecl($1, $$ = $3); }
			;

TransTypeSpec		:	TypeSpec
				{ $$ = itResetType($1); }
			|	TransTypeSpec syInTran syColon syIdentifier
				syIdentifier syLParen syIdentifier syRParen 
{
    $$ = $1;

    if (($$->itTransType != strNULL) && !streql($$->itTransType, $4))
	warn("conflicting translation types (%s, %s)",
	     $$->itTransType, $4);
    $$->itTransType = $4;

    if (($$->itInTrans != strNULL) && !streql($$->itInTrans, $5))
	warn("conflicting in-translation functions (%s, %s)",
	     $$->itInTrans, $5);
    $$->itInTrans = $5;

    if (($$->itServerType != strNULL) && !streql($$->itServerType, $7))
	warn("conflicting server types (%s, %s)",
	     $$->itServerType, $7);
    $$->itServerType = $7;
}
			|	TransTypeSpec syOutTran syColon syIdentifier
				syIdentifier syLParen syIdentifier syRParen
{
    $$ = $1;

    if (($$->itServerType != strNULL) && !streql($$->itServerType, $4))
	warn("conflicting server types (%s, %s)",
	     $$->itServerType, $4);
    $$->itServerType = $4;

    if (($$->itOutTrans != strNULL) && !streql($$->itOutTrans, $5))
	warn("conflicting out-translation functions (%s, %s)",
	     $$->itOutTrans, $5);
    $$->itOutTrans = $5;

    if (($$->itTransType != strNULL) && !streql($$->itTransType, $7))
	warn("conflicting translation types (%s, %s)",
	     $$->itTransType, $7);
    $$->itTransType = $7;
}
			|	TransTypeSpec syDestructor syColon syIdentifier
				syLParen syIdentifier syRParen
{
    $$ = $1;

    if (($$->itDestructor != strNULL) && !streql($$->itDestructor, $4))
	warn("conflicting destructor functions (%s, %s)",
	     $$->itDestructor, $4);
    $$->itDestructor = $4;

    if (($$->itTransType != strNULL) && !streql($$->itTransType, $6))
	warn("conflicting translation types (%s, %s)",
	     $$->itTransType, $6);
    $$->itTransType = $6;
}
			|	TransTypeSpec syCType syColon syIdentifier
{
    $$ = $1;

    if (($$->itUserType != strNULL) && !streql($$->itUserType, $4))
	warn("conflicting user types (%s, %s)",
	     $$->itUserType, $4);
    $$->itUserType = $4;

    if (($$->itServerType != strNULL) && !streql($$->itServerType, $4))
	warn("conflicting server types (%s, %s)",
	     $$->itServerType, $4);
    $$->itServerType = $4;
}
			|	TransTypeSpec syCUserType syColon syIdentifier
{
    $$ = $1;

    if (($$->itUserType != strNULL) && !streql($$->itUserType, $4))
	warn("conflicting user types (%s, %s)",
	     $$->itUserType, $4);
    $$->itUserType = $4;
}
			|	TransTypeSpec syCServerType
				syColon syIdentifier
{
    $$ = $1;

    if (($$->itServerType != strNULL) && !streql($$->itServerType, $4))
	warn("conflicting server types (%s, %s)",
	     $$->itServerType, $4);
    $$->itServerType = $4;
}
			|	TransTypeSpec syRAngle syIdentifier
				syLAngle syIdentifier syColon syIdentifier
{
    warn("obsolete translation spec");
    $$ = $1;

    if (($$->itInTrans != strNULL) && !streql($$->itInTrans, $3))
	warn("conflicting in-translation functions (%s, %s)",
	     $$->itInTrans, $3);
    $$->itInTrans = $3;

    if (($$->itOutTrans != strNULL) && !streql($$->itOutTrans, $5))
	warn("conflicting out-translation functions (%s, %s)",
	     $$->itOutTrans, $5);
    $$->itOutTrans = $5;

    if (($$->itTransType != strNULL) && !streql($$->itTransType, $7))
	warn("conflicting translation types (%s, %s)",
	     $$->itTransType, $7);
    $$->itTransType = $7;
}
			|	TransTypeSpec syRAngle syIdentifier
				syLAngle syIdentifier syColon syIdentifier
				syTilde syIdentifier
{
    warn("obsolete translation spec");
    $$ = $1;

    if (($$->itInTrans != strNULL) && !streql($$->itInTrans, $3))
	warn("conflicting in-translation functions (%s, %s)",
	     $$->itInTrans, $3);
    $$->itInTrans = $3;

    if (($$->itOutTrans != strNULL) && !streql($$->itOutTrans, $5))
	warn("conflicting out-translation functions (%s, %s)",
	     $$->itOutTrans, $5);
    $$->itOutTrans = $5;

    if (($$->itTransType != strNULL) && !streql($$->itTransType, $7))
	warn("conflicting translation types (%s, %s)",
	     $$->itTransType, $7);
    $$->itTransType = $7;

    if (($$->itServerType != strNULL) && !streql($$->itServerType, $9))
	warn("conflicting server types (%s, %s)",
	     $$->itServerType, $9);
    $$->itServerType = $9;
}
			|	TransTypeSpec syMinus syIdentifier
{
    warn("obsolete destructor spec");
    $$ = $1;

    if (($$->itDestructor != strNULL) && !streql($$->itDestructor, $3))
	warn("conflicting destructor functions (%s, %s)",
	     $$->itDestructor, $3);
    $$->itDestructor = $3;
}
			;

TypeSpec		:	BasicTypeSpec
				{ $$ = $1; }
			|	PrevTypeSpec
				{ $$ = $1; }
			|	VarArrayHead TypeSpec
				{ $$ = itVarArrayDecl($1, $2); }
			|	ArrayHead TypeSpec
				{ $$ = itArrayDecl($1, $2); }
			|	syCaret TypeSpec
				{ $$ = itPtrDecl($2); }
			|	StructHead TypeSpec
				{ $$ = itStructDecl($1, $2); }
			;

BasicTypeSpec		:	IPCType
{
    $$ = itShortDecl($1.innumber, $1.instr,
		     $1.outnumber, $1.outstr,
		     $1.size);
}
			|	syLParen IPCType syComma IntExp
				IPCFlags syRParen
{
    $$ = itLongDecl($2.innumber, $2.instr,
		    $2.outnumber, $2.outstr,
		    $2.size, $4, $5);
}
			;

IPCFlags		:	/* empty */
				{ $$ = flNone; }
			|	IPCFlags syComma syIPCFlag
{
    if ($1 & $3)
	warn("redundant IPC flag ignored");
    else
	$$ = $1 | $3;
}
			;

PrimIPCType		:	syNumber
{
    $$.innumber = $$.outnumber = $1;
    $$.instr = $$.outstr = strNULL;
    $$.size = 0;
}
			|	sySymbolicType
				{ $$ = $1; }
			;

IPCType			:	PrimIPCType
				{ $$ = $1; }
			|	PrimIPCType syBar PrimIPCType
{
    if ($1.size != $3.size)
    {
	if ($1.size == 0)
	    $$.size = $3.size;
	else if ($3.size == 0)
	    $$.size = $1.size;
	else
	{
	    error("sizes in IPCTypes (%d, %d) aren't equal",
		  $1.size, $3.size);
	    $$.size = 0;
	}
    }
    else
	$$.size = $1.size;
    $$.innumber = $1.innumber;
    $$.instr = $1.instr;
    $$.outnumber = $3.outnumber;
    $$.outstr = $3.outstr;
}
			;

PrevTypeSpec		:	syIdentifier
				{ $$ = itPrevDecl($1); }
			;

VarArrayHead		:	syArray syLBrack syRBrack syOf
				{ $$ = 0; }
			|	syArray syLBrack syStar syRBrack syOf
				{ $$ = 0; }
			|	syArray syLBrack syStar syColon IntExp 
				syRBrack syOf
				{ $$ = $5; }
			;

ArrayHead		:	syArray syLBrack IntExp syRBrack syOf
				{ $$ = $3; }
			;

StructHead		:	syStruct syLBrack IntExp syRBrack syOf
				{ $$ = $3; }
			;

IntExp			: 	IntExp	syPlus	IntExp
				{ $$ = $1 + $3;	}
			| 	IntExp	syMinus	IntExp
				{ $$ = $1 - $3;	}
			|	IntExp	syStar	IntExp
				{ $$ = $1 * $3;	}
			| 	IntExp	syDiv	IntExp
				{ $$ = $1 / $3;	}
			|	syNumber
				{ $$ = $1;	}
			|	syLParen IntExp syRParen
				{ $$ = $2;	}
			;

 
RoutineDecl		:	Routine			{ $$ = $1; }
			|	SimpleRoutine		{ $$ = $1; }
			|	CamelotRoutine		{ $$ = $1; }
			|	Procedure		{ $$ = $1; }
			|	SimpleProcedure		{ $$ = $1; }
			|	Function		{ $$ = $1; }
			;

Routine			:	syRoutine syIdentifier Arguments
				{ $$ = rtMakeRoutine($2, $3); }
			;

SimpleRoutine		:	sySimpleRoutine syIdentifier Arguments
				{ $$ = rtMakeSimpleRoutine($2, $3); }
			;

CamelotRoutine		:	syCamelotRoutine syIdentifier Arguments
				{ $$ = rtMakeCamelotRoutine($2, $3); }
			;

Procedure		:	syProcedure syIdentifier Arguments
				{ $$ = rtMakeProcedure($2, $3); }
			;

SimpleProcedure		:	sySimpleProcedure syIdentifier Arguments
				{ $$ = rtMakeSimpleProcedure($2, $3); }
			;

Function		:	syFunction syIdentifier Arguments ArgumentType
				{ $$ = rtMakeFunction($2, $3, $4); }
			;

Arguments		:	syLParen syRParen
				{ $$ = argNULL; }
			|	syLParen ArgumentList syRParen
				{ $$ = $2; }

			;

ArgumentList		:	Argument
				{ $$ = $1; }
			|	Argument sySemi ArgumentList
{
    $$ = $1;
    $$->argNext = $3;
}
			;

Argument		:	Direction syIdentifier ArgumentType IPCFlags
{
    $$ = argAlloc();
    $$->argKind = $1;
    $$->argName = $2;
    $$->argType = $3;
    $$->argFlags = $4;
}
			;

Direction		:	/* empty */	{ $$ = akNone; }
			|	syIn		{ $$ = akIn; }
			|	syOut		{ $$ = akOut; }
			|	syInOut		{ $$ = akInOut; }
			|	syRequestPort	{ $$ = akRequestPort; }
			|	syReplyPort	{ $$ = akReplyPort; }
			|	syWaitTime	{ $$ = akWaitTime; }
			|	syMsgType	{ $$ = akMsgType; }
			;

ArgumentType		:	syColon syIdentifier
{
    $$ = itLookUp($2);
    if ($$ == itNULL)
	error("type '%s' not defined", $2);
}
			|	syColon NamedTypeSpec
				{ $$ = $2; }
			;

LookString		:	/* empty */
				{ LookString(); }
			;

LookFileName		:	/* empty */
				{ LookFileName(); }
			;

LookQString		:	/* empty */
				{ LookQString(); }
			;

%%

yyerror(s)
    char *s;
{
    error(s);
}

static char *
import_name(sk)
    statement_kind_t sk;
{
    switch (sk)
    {
      case skImport:
	return "Import";
      case skSImport:
	return "SImport";
      case skUImport:
	return "UImport";
      default:
	fatal("import_name(%d): not import statement", (int) sk);
	/*NOTREACHED*/
    }
}
