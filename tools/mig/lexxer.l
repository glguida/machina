%option nounput
%option noyywrap

%k 10000
%n 5000
%a 20000
%e 10000
%p 25000

Ident		([A-Za-z_][A-Za-z_0-9]*)
Number		([0-9]+)
String		([-/._$A-Za-z0-9]+)
QString		(\"[^"\n]*\")
AString		(\<[^>\n]*\>)
FileName	({QString}|{AString})

%{
/* 
 * Mach Operating System
 * Copyright (c) 1993,1992,1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	lexxer.l,v $
 * Revision 2.11  93/11/17  18:48:05  dbg
 * 	stringize() macro for non-STDC preprocessors.
 * 	Changed TPRETURN to take an alignment argument.
 * 	Fixed all uses accordingly.
 * 	[93/09/14  12:52:12  af]
 * 
 * Revision 2.10  93/08/02  21:44:36  mrt
 * 	Removed stringize macro which was broken under ANSI C.
 * 	[93/07/28            pds]
 * 
 * Revision 2.9  93/05/10  17:49:16  rvb
 * 	Fix include to use < vs " for new ode shadowing
 * 	[93/05/10  10:27:59  rvb]
 * 
 * 	386BSD uses a different lex
 * 	[93/05/05  09:16:56  rvb]
 * 
 * Revision 2.8  93/01/14  17:58:16  danner
 * 	Converted file to ANSI C.
 * 	[92/12/08            pds]
 * 	64bit cleanup.
 * 	[92/12/01            af]
 * 
 * Revision 2.7  92/01/23  15:21:24  rpd
 * 	Fixed macros for STDC.
 * 	[92/01/16            rpd]
 * 
 * Revision 2.6  92/01/03  20:29:25  dbg
 * 	Add 'CountInOut' flag.
 * 	[91/11/11            dbg]
 * 
 * Revision 2.5  91/08/28  11:17:01  jsb
 * 	Added syServerDemux.
 * 	[91/08/13            rpd]
 * 
 * 	Removed syMsgKind, syCamelot, syCamelotRoutine, syTrapRoutine,
 * 	syTrapSimpleRoutine.  Added syMsgSeqno.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.4  91/07/31  18:09:31  dbg
 * 	Add 'serverCopy' keyword.
 * 	[91/06/05            dbg]
 * 
 * 	Add 'c_string' keyword.
 * 	[91/04/03            dbg]
 * 
 * Revision 2.3  91/02/05  17:54:51  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:54:35  mrt]
 * 
 * Revision 2.2  90/06/02  15:04:54  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:11:34  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include <machina/message.h>
#include "string.h"
#include "type.h"
#include "statement.h"
#include "global.h"
#include "parser.h"
#include "lexxer.h"
#include "cross64.h"

void yyerror(const char *s);

#ifdef	__STDC__
#define	stringize(x)	#x
#else
#define	stringize(x)	"x"
#endif

#ifdef	LDEBUG
#define RETURN(sym)							\
{									\
    printf("yylex: returning '%s' (%d)\n", #sym, (sym));		\
    return (sym);							\
}
#else	/* LDEBUG */
#define RETURN(sym)	return (sym)
#endif	/* LDEBUG */

#define	TPRETURN(intype, outtype, tsize, talign)			\
{									\
    yylval.symtype.innumber = (intype);					\
    yylval.symtype.instr = stringize(intype);				\
    yylval.symtype.outnumber = (outtype);				\
    yylval.symtype.outstr = stringize(outtype);				\
    yylval.symtype.size = (tsize);					\
    yylval.symtype.alignment = (talign);				\
    RETURN(sySymbolicType);						\
}

#define	TRETURN(type, tsize, talign)	TPRETURN(type,type,tsize,talign)

#define	FRETURN(val)							\
{									\
    yylval.flag = (val);						\
    RETURN(syIPCFlag);							\
}

static int oldYYBegin;
#define yybgin yy_start

static void doSharp(const char *body); /* process body of # directives */
%}

%Start	Normal String FileName QString SkipToEOL

%%

<Normal>[Rr][Oo][Uu][Tt][Ii][Nn][Ee]		RETURN(syRoutine);
<Normal>[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]	RETURN(syFunction);
<Normal>[Pp][Rr][Oo][Cc][Ee][Dd][Uu][Rr][Ee]	RETURN(syProcedure);
<Normal>[Ss][Ii][Mm][Pp][Ll][Ee][Pp][Rr][Oo][Cc][Ee][Dd][Uu][Rr][Ee] RETURN(sySimpleProcedure);
<Normal>[Ss][Ii][Mm][Pp][Ll][Ee][Rr][Oo][Uu][Tt][Ii][Nn][Ee] RETURN(sySimpleRoutine);
<Normal>[Ss][Uu][Bb][Ss][Yy][Ss][Tt][Ee][Mm]	RETURN(sySubsystem);
<Normal>[Mm][Ss][Gg][Oo][Pp][Tt][Ii][Oo][Nn]	RETURN(syMsgOption);
<Normal>[Mm][Ss][Gg][Ss][Ee][Qq][Nn][Oo]	RETURN(syMsgSeqno);
<Normal>[Ww][Aa][Ii][Tt][Tt][Ii][Mm][Ee]	RETURN(syWaitTime);
<Normal>[Nn][Oo][Ww][Aa][Ii][Tt][Tt][Ii][Mm][Ee]	RETURN(syNoWaitTime);
<Normal>[Ii][Nn]				RETURN(syIn);
<Normal>[Oo][Uu][Tt]				RETURN(syOut);
<Normal>[Ii][Nn][Oo][Uu][Tt]			RETURN(syInOut);
<Normal>[Rr][Ee][Qq][Uu][Ee][Ss][Tt][Pp][Oo][Rr][Tt]	RETURN(syRequestPort);
<Normal>[Rr][Ee][Pp][Ll][Yy][Pp][Oo][Rr][Tt]		RETURN(syReplyPort);
<Normal>[Uu][Rr][Ee][Pp][Ll][Yy][Pp][Oo][Rr][Tt]	RETURN(syUReplyPort);
<Normal>[Ss][Rr][Ee][Pp][Ll][Yy][Pp][Oo][Rr][Tt]	RETURN(sySReplyPort);
<Normal>[Aa][Rr][Rr][Aa][Yy]			RETURN(syArray);
<Normal>[Oo][Ff]				RETURN(syOf);
<Normal>[Ee][Rr][Rr][Oo][Rr]			RETURN(syErrorProc);
<Normal>[Ss][Ee][Rr][Vv][Ee][Rr][Pp][Rr][Ee][Ff][Ii][Xx] RETURN(syServerPrefix);
<Normal>[Uu][Ss][Ee][Rr][Pp][Rr][Ee][Ff][Ii][Xx]	RETURN(syUserPrefix);
<Normal>[Ss][Ee][Rr][Vv][Ee][Rr][Dd][Ee][Mm][Uu][Xx]	RETURN(syServerDemux);
<Normal>[Rr][Cc][Ss][Ii][Dd]			RETURN(syRCSId);
<Normal>[Ii][Mm][Pp][Oo][Rr][Tt]		RETURN(syImport);
<Normal>[Uu][Ii][Mm][Pp][Oo][Rr][Tt]		RETURN(syUImport);
<Normal>[Ss][Ii][Mm][Pp][Oo][Rr][Tt]		RETURN(sySImport);
<Normal>[Tt][Yy][Pp][Ee]			RETURN(syType);
<Normal>[Kk][Ee][Rr][Nn][Ee][Ll][Ss][Ee][Rr][Vv][Ee][Rr] RETURN(syKernelServer);
<Normal>[Kk][Ee][Rr][Nn][Ee][Ll][Uu][Ss][Ee][Rr]	RETURN(syKernelUser);
<Normal>[Kk][Ee][Rr][Nn][El][Ll][Cc][Ll][Ii][Ee][Nn][Tt]	RETURN(syKernelUser);
<Normal>[Ss][Kk][Ii][Pp]			RETURN(sySkip);
<Normal>[Ss][Tt][Rr][Uu][Cc][Tt]		RETURN(syStruct);
<Normal>[Ii][Nn][Tt][Rr][Aa][Nn]		RETURN(syInTran);
<Normal>[Oo][Uu][Tt][Tt][Rr][Aa][Nn]		RETURN(syOutTran);
<Normal>[Dd][Ee][Ss][Tt][Rr][Uu][Cc][Tt][Oo][Rr]	RETURN(syDestructor);
<Normal>[Cc][Tt][Yy][Pp][Ee]				RETURN(syCType);
<Normal>[Cc][Uu][Ss][Ee][Rr][Tt][Yy][Pp][Ee]		RETURN(syCUserType);
<Normal>[Cc][Ss][Ee][Rr][Vv][Ee][Rr][Tt][Yy][Pp][Ee]	RETURN(syCServerType);
<Normal>[Cc]_[Ss][Tt][Rr][Ii][Nn][Gg]			RETURN(syCString);

<Normal>[Ii][Ss][Ll][Oo][Nn][Gg]		FRETURN(flLong);
<Normal>[Ii][Ss][Nn][Oo][Tt][Ll][Oo][Nn][Gg]	FRETURN(flNotLong);
<Normal>[Dd][Ee][Aa][Ll][Ll][Oo][Cc]		FRETURN(flDealloc);
<Normal>[Nn][Oo][Tt][Dd][Ee][Aa][Ll][Ll][Oo][Cc] FRETURN(flNotDealloc);
<Normal>[Ss][Ee][Rr][Vv][Ee][Rr][Cc][Oo][Pp][Yy] FRETURN(flServerCopy);
<Normal>[Cc][Oo][Uu][Nn][Tt][Ii][Nn][Oo][Uu][Tt] FRETURN(flCountInOut);

<Normal>[Pp][Oo][Ll][Yy][Mm][Oo][Rr][Pp][Hh][Ii][Cc]	TRETURN(MCN_MSGTYPE_POLYMORPHIC,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_UNSTRUCTURED"	TRETURN(MCN_MSGTYPE_UNSTRUCTURED,0,0);
<Normal>"MCN_MSGTYPE_UNSTRUCTURED"	TRETURN(MCN_MSGTYPE_UNSTRUCTURED,0,0);

<Normal>"MACH_MSG_TYPE_BIT"		TRETURN(MCN_MSGTYPE_BIT,1,0);
<Normal>"MCN_MSGTYPE_BIT"		TRETURN(MCN_MSGTYPE_BIT,1,0);

<Normal>"MACH_MSG_TYPE_BOOLEAN"		TRETURN(MCN_MSGTYPE_BOOLEAN,32,32);
<Normal>"MCN_MSGTYPE_BOOLEAN"		TRETURN(MCN_MSGTYPE_BOOLEAN,32,32);

<Normal>"MACH_MSG_TYPE_INTEGER_16"	TRETURN(MCN_MSGTYPE_INT16,16,16);
<Normal>"MCN_MSGTYPE_INT16"		TRETURN(MCN_MSGTYPE_INT16,16,16);

<Normal>"MACH_MSG_TYPE_INTEGER_32"	TRETURN(MCN_MSGTYPE_INT32,32,32);
<Normal>"MCN_MSGTYPE_INT32"		TRETURN(MCN_MSGTYPE_INT32,32,32);

<Normal>"MACH_MSG_TYPE_INTEGER_64"     	TRETURN(MCN_MSGTYPE_INT64,64,64);
<Normal>"MCN_MSGTYPE_INT64"		TRETURN(MCN_MSGTYPE_INT64,64,64);

<Normal>"MACHO_MSG_TYPE_CHAR"		TRETURN(MCN_MSGTYPE_CHAR,8,0);
<Normal>"MCN_MSGTYPE_CHAR"		TRETURN(MCN_MSGTYPE_CHAR,8,0);

<Normal>"MACH_MSG_TYPE_BYTE"		TRETURN(MCN_MSGTYPE_BYTE,8,0);
<Normal>"MCN_MSGTYPE_BYTE"		TRETURN(MCN_MSGTYPE_BYTE,8,0);

<Normal>"MACH_MSG_TYPE_INTEGER_8"      	TRETURN(MCN_MSGTYPE_INT8,8,0);
<Normal>"MCN_MSGTYPE_INT8"		TRETURN(MCN_MSGTYPE_INT8,8,0);

<Normal>"MACH_MSG_TYPE_REAL"		TRETURN(MCN_MSGTYPE_REAL,0,0);
<Normal>"MCN_MSGTYPE_REAL"		TRETURN(MCN_MSGTYPE_REAL,0,0);

<Normal>"MACH_MSG_TYPE_STRING"		TRETURN(MCN_MSGTYPE_STRING,0,0);
<Normal>"MCN_MSGTYPE_STRING"		TRETURN(MCN_MSGTYPE_STRING,0,0);

<Normal>"MACH_MSG_TYPE_STRING_C"       	TRETURN(MCN_MSGTYPE_CSTRING,0,0);
<Normal>"MCN_MSGTYPE_CSTRING"		TRETURN(MCN_MSGTYPE_CSTRING,0,0);

<Normal>"MACH_MSG_TYPE_MOVE_RECEIVE"   	TPRETURN(MCN_MSGTYPE_MOVERECV,MCN_MSGTYPE_PORTRECV,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_MOVERECV"		TPRETURN(MCN_MSGTYPE_MOVERECV,MCN_MSGTYPE_PORTRECV,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_COPY_SEND"	TPRETURN(MCN_MSGTYPE_COPYSEND,MCN_MSGTYPE_PORTSEND,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_COPYSEND"		TPRETURN(MCN_MSGTYPE_COPYSEND,MCN_MSGTYPE_PORTSEND,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_MAKE_SEND"      	TPRETURN(MCN_MSGTYPE_MAKESEND,MCN_MSGTYPE_PORTSEND,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_MAKESEND"		TPRETURN(MCN_MSGTYPE_MAKESEND,MCN_MSGTYPE_PORTSEND,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_MOVE_SEND"	TPRETURN(MCN_MSGTYPE_MOVESEND,MCN_MSGTYPE_PORTSEND,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_MOVESEND"		TPRETURN(MCN_MSGTYPE_MOVESEND,MCN_MSGTYPE_PORTSEND,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_MAKE_SEND_ONCE" 	TPRETURN(MCN_MSGTYPE_MAKEONCE,MCN_MSGTYPE_PORTONCE,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_MAKEONCE"		TPRETURN(MCN_MSGTYPE_MAKEONCE,MCN_MSGTYPE_PORTONCE,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_MOVE_SEND_ONCE"	TPRETURN(MCN_MSGTYPE_MOVEONCE,MCN_MSGTYPE_PORTONCE,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_MOVEONCE"		TPRETURN(MCN_MSGTYPE_MOVEONCE,MCN_MSGTYPE_PORTONCE,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_PORT_NAME"      	TRETURN(MCN_MSGTYPE_PORTNAME,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_PORTNAME"		TRETURN(MCN_MSGTYPE_PORTNAME,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_PORT_RECEIVE"   	TPRETURN(MCN_MSGTYPE_POLYMORPHIC,MCN_MSGTYPE_PORTRECV,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_PORTRECV"		TPRETURN(MCN_MSGTYPE_POLYMORPHIC,MCN_MSGTYPE_PORTRECV,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_PORT_SEND"      	TPRETURN(MCN_MSGTYPE_POLYMORPHIC,MCN_MSGTYPE_PORTSEND,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_PORTSEND"		TPRETURN(MCN_MSGTYPE_POLYMORPHIC,MCN_MSGTYPE_PORTSEND,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_PORT_SEND_ONCE"	TPRETURN(MCN_MSGTYPE_POLYMORPHIC,MCN_MSGTYPE_PORTONCE,word_size_in_bits,word_size_in_bits);
<Normal>"MCN_MSGTYPE_PORTONCE"		TPRETURN(MCN_MSGTYPE_POLYMORPHIC,MCN_MSGTYPE_PORTONCE,word_size_in_bits,word_size_in_bits);

<Normal>"MACH_MSG_TYPE_POLYMORPHIC"	TRETURN(MCN_MSGTYPE_POLYMORPHIC,0,word_size_in_bits);
<Normal>"MCN_MSGTYPE_POLYMORPHIC"	TRETURN(MCN_MSGTYPE_POLYMORPHIC,0,word_size_in_bits);

<Normal>":"		RETURN(syColon);
<Normal>";"		RETURN(sySemi);
<Normal>","		RETURN(syComma);
<Normal>"+"		RETURN(syPlus);
<Normal>"-"		RETURN(syMinus);
<Normal>"*"		RETURN(syStar);
<Normal>"/"		RETURN(syDiv);
<Normal>"("		RETURN(syLParen);
<Normal>")"		RETURN(syRParen);
<Normal>"="		RETURN(syEqual);
<Normal>"^"		RETURN(syCaret);
<Normal>"~"		RETURN(syTilde);
<Normal>"<"		RETURN(syLAngle);
<Normal>">"		RETURN(syRAngle);
<Normal>"["		RETURN(syLBrack);
<Normal>"]"		RETURN(syRBrack);
<Normal>"|"		RETURN(syBar);

<Normal>{Ident}		{ yylval.identifier = strmake(yytext);
			  RETURN(syIdentifier); }
<Normal>{Number}	{ yylval.number = atoi(yytext); RETURN(syNumber); }

<String>{String}	{ yylval.string = strmake(yytext);
			  BEGIN Normal; RETURN(syString); }
<FileName>{FileName}	{ yylval.string = strmake(yytext);
			  BEGIN Normal; RETURN(syFileName); }
<QString>{QString}	{ yylval.string = strmake(yytext);
			  BEGIN Normal; RETURN(syQString); }

^\#[ \t]*{Number}[ \t]*\"[^"]*\"	{ doSharp(yytext+1);
					  oldYYBegin = yybgin;
					  BEGIN SkipToEOL; }
^\#\ *{Number}				{ doSharp(yytext+1);
					  oldYYBegin = yybgin;
					  BEGIN SkipToEOL; }
^\#					{ yyerror("illegal # directive");
					  oldYYBegin = yybgin;
					  BEGIN SkipToEOL; }

<SkipToEOL>\n		yybgin = oldYYBegin;
<SkipToEOL>.		;

[ \t\n]			;
.			{ BEGIN Normal; RETURN(syError); }

%%

extern void
LookNormal(void)
{
    BEGIN Normal;
}

extern void
LookString(void)
{
    BEGIN String;
}

extern void
LookQString(void)
{
    BEGIN QString;
}

extern void
LookFileName(void)
{
    BEGIN FileName;
}

static void
doSharp(const char *body)
{
    register const char *startName;

#ifndef	__386BSD__
    yylineno = atoi(body);
    startName = index(body, '"');
    if (startName != NULL)
    {
	*rindex(body, '"') = '\0';
	strfree(yyinname);
	yyinname = strmake(startName+1);
    }
#endif
}
