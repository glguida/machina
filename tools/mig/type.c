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
 * $Log:	type.c,v $
 * Revision 2.10  93/11/17  18:49:43  dbg
 * 	Added itAlignmentOf, and initializations of itAlignment field.
 * 	Fixed prettyprinter to emit alignment of types.
 * 	Changed itShortDecl/itLongDecl to take up alignment arg.
 * 	Fixed all uses accordingly.
 * 	[93/09/14  12:57:37  af]
 * 
 * Revision 2.9  93/05/10  17:50:00  rvb
 * 	Fix include to use < vs " for new ode shadowing
 * 	[93/05/10  10:29:29  rvb]
 * 
 * Revision 2.8  93/01/14  17:59:11  danner
 * 	Removed obsolete itTidType.
 * 	[92/12/14            pds]
 * 	Converted file to ANSI C.
 * 	Removed initialization of (Camelot) tid_t support.
 * 	[92/12/08            pds]
 * 	64bit cleanup.
 * 	[92/12/01            af]
 * 
 * Revision 2.7  92/01/14  16:46:47  rpd
 * 	Changed Indefinite types from MustBeLong to ShouldBeLong.
 * 	Added itCheckFlags, itCheckDeallocate, itCheckIsLong.
 * 	Removed itServerCopy.
 * 	[92/01/09            rpd]
 * 
 * Revision 2.6  91/08/28  11:17:27  jsb
 * 	Removed itMsgKindType.
 * 	[91/08/12            rpd]
 * 
 * Revision 2.5  91/07/31  18:11:12  dbg
 * 	Indefinite-length inline arrays.
 * 
 * 	Change itDeallocate to an enumerated type, to allow for
 * 	user-specified deallocate flag.
 * 
 * 	Add itCStringDecl.
 * 	[91/07/17            dbg]
 * 
 * Revision 2.4  91/06/25  10:32:09  rpd
 * 	Changed itCalculateNameInfo to change type names from mach_port_t
 * 	to ipc_port_t for KernelServer and KernelUser interfaces.
 * 	[91/05/28            rpd]
 * 
 * Revision 2.3  91/02/05  17:56:02  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:56:12  mrt]
 * 
 * Revision 2.2  90/06/02  15:05:54  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:14:07  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 17-Aug-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed translation of MSG_TYPE_INVALID as that type
 *	is no longer defined by the kernel.
 * 
 * 19-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed itPrintTrans and itPrintDecl to reflect new translation syntax.
 *	Changed itCheckDecl to set itServerType to itType if is is strNULL.
 *
 *  4-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added a check to itCheckDecl to make sure that in-line
 *	variable length arrays have a non-zero maximum length.
 *
 * 22-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Removed warning message for translation.
 *
 * 16-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	Changed itVarArrayDecl to take a 'max' parameter for maximum
 *	number of elements, and to make type not be 'struct'.
 *	Added itDestructor.
 *
 * 18-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added initialization of itPortType
 *
 * 14-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added initialization for itTidType
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Fixed prototype for itAlloc; was missing itServerType field.
 *
 * 10-Jun-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed extern from procedure definitions to make hi-c happy
 *	Changed the c type names of itDummyType from caddr_t to
 *	char * and of itCountType from u_int to unsigned int to
 *	eliminate the need to import sys/types.h into the mig generated
 *	code.
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include <machina/message.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "global.h"
#include "type.h"
#include "cross64.h"

ipc_type_t *itRetCodeType;	/* used for return codes */
ipc_type_t *itDummyType;	/* used for camelot dummy args */
ipc_type_t *itRequestPortType;	/* used for default Request port arg */
ipc_type_t *itZeroReplyPortType;/* used for dummy Reply port arg */
ipc_type_t *itRealReplyPortType;/* used for default Reply port arg */
ipc_type_t *itWaitTimeType;	/* used for dummy WaitTime args */
ipc_type_t *itMsgOptionType;	/* used for dummy MsgOption args */

static ipc_type_t *list = itNULL;

/*
 *  Searches for a named type.  We use a simple
 *  self-organizing linked list.
 */
ipc_type_t *
itLookUp(identifier_t name)
{
    register ipc_type_t *it, **last;

    for (it = *(last = &list); it != itNULL; it = *(last = &it->itNext))
	if (streql(name, it->itName))
	{
	    /* move this type to the front of the list */
	    *last = it->itNext;
	    it->itNext = list;
	    list = it;

	    return it;
	}

    return itNULL;
}

/*
 *  Enters a new name-type association into
 *  our self-organizing linked list.
 */
void
itInsert(identifier_t name, ipc_type_t *it)
{
    it->itName = name;
    it->itNext = list;
    list = it;
}

static ipc_type_t *
itAlloc(void)
{
    static const ipc_type_t prototype =
    {
	strNULL,		/* identifier_t itName */
	0,			/* ipc_type_t *itNext */
	0,			/* u_int itTypeSize */
	0,			/* u_int itAlignment */
	0,			/* u_int itPadSize */
	0,			/* u_int itMinTypeSize */
	0,			/* u_int itInName */
	0,			/* u_int itOutName */
	0,			/* u_int itSize */
	1,			/* u_int itNumber */
	TRUE,			/* boolean_t itInLine */
	FALSE,			/* boolean_t itLongForm */
	d_NO,			/* dealloc_t itDeallocate */
	strNULL,		/* string_t itInNameStr */
	strNULL,		/* string_t itOutNameStr */
	flNone,			/* ipc_flags_t itFlags */
	TRUE,			/* boolean_t itStruct */
	FALSE,			/* boolean_t itString */
	FALSE,			/* boolean_t itVarArray */
	FALSE,			/* boolean_t itIndefinite */
	itNULL,			/* ipc_type_t *itElement */
	strNULL,		/* identifier_t itUserType */
	strNULL,		/* identifier_t itServerType */
	strNULL,		/* identifier_t itTransType */
	strNULL,		/* identifier_t itInTrans */
	strNULL,		/* identifier_t itOutTrans */
	strNULL,		/* identifier_t itDestructor */
    };
    register ipc_type_t *new;

    new = malloc(sizeof *new);
    if (new == itNULL)
	fatal("itAlloc(): %s", unix_error_string(errno));
    *new = prototype;
    return new;
}

/*
 * Convert an IPC type-name into a string.
 */
static string_t
itNameToString(u_int name)
{
    char buffer[100];

    (void) sprintf(buffer, "%u", name);
    return strmake(buffer);
}

/*
 * Calculate itTypeSize, itPadSize, itMinTypeSize.
 * Every type needs this info; it is recalculated
 * when itInLine, itNumber, or itSize changes.
 */
static void
itCalculateSizeInfo(register ipc_type_t *it)
{
    if (it->itInLine)
    {
	u_int bytes = (it->itNumber * it->itSize + 7) / 8;
	u_int padding = (4 - bytes%4)%4;

	it->itTypeSize = bytes;
	it->itPadSize = padding;
	if (it->itVarArray)
	    it->itMinTypeSize = 0;
	else
	    it->itMinTypeSize = bytes + padding;
    }
    else
    {
	/* out-of-line, so use size of pointers */
	u_int bytes = sizeof_pointer;

	it->itTypeSize = bytes;
	it->itPadSize = 0;
	it->itMinTypeSize = bytes;
    }

    /* Unfortunately, these warning messages can't give a type name;
       we haven't seen a name yet (it might stay anonymous.) */

    if ((it->itNumber * it->itSize) % 8 != 0)
	warn("size of C types must be multiples of 8 bits");

    if ((it->itTypeSize == 0) && !it->itVarArray)
	warn("sizeof(%s) == 0", it->itName);
}

/*
 * Fill in default values for some fields used in code generation:
 *	itInNameStr, itOutNameStr, itUserType, itServerType, itTransType
 * Every argument's type should have these values filled in.
 */
static void
itCalculateNameInfo(register ipc_type_t *it)
{
    if (it->itInNameStr == strNULL)
	it->itInNameStr = strmake(itNameToString(it->itInName));
    if (it->itOutNameStr == strNULL)
	it->itOutNameStr = strmake(itNameToString(it->itOutName));

    if (it->itUserType == strNULL)
	it->itUserType = it->itName;
    if (it->itServerType == strNULL)
	it->itServerType = it->itName;

    /* The following is not needed in machina, where ipcport_t is
       typedef to mcn_portid_t. */
#if 0
    /*
     *	KernelServer and KernelUser interfaces get special treatment here.
     *	On the kernel side of the interface, ports are really internal
     *	port pointers (ipc_port_t), not port names (mach_port_t).
     *	At this point, we don't know if the argument is in or out,
     *	so we don't know if we should look at itInName or itOutName.
     *	Looking at both should be OK.
     *
     *	This is definitely a hack, but I think it is cleaner than
     *	mucking with type declarations throughout the kernel .def files,
     *	hand-conditionalizing on KERNEL_SERVER and KERNEL_USER.
     */

    if (IsKernelServer &&
	streql(it->itServerType, "mcn_portid_t") &&
	(((it->itInName == MCN_MSGTYPE_POLYMORPHIC) &&
	  (it->itOutName == MCN_MSGTYPE_POLYMORPHIC)) ||
	 MCN_MSGTYPE_IS_PORT(it->itInName) ||
	 MCN_MSGTYPE_IS_PORT(it->itOutName)))
	it->itServerType = "ipcport_t";

    if (IsKernelUser &&
	streql(it->itUserType, "mcn_portid_t") &&
	(((it->itInName == MCN_MSGTYPE_POLYMORPHIC) &&
	  (it->itOutName == MCN_MSGTYPE_POLYMORPHIC)) ||
	 MCN_MSGTYPE_IS_PORT(it->itInName) ||
	 MCN_MSGTYPE_IS_PORT(it->itOutName)))
	it->itUserType = "ipcport_t";
#endif

    if (it->itTransType == strNULL)
	it->itTransType = it->itServerType;
}

ipc_flags_t
itCheckFlags(ipc_flags_t flags, identifier_t name)
{
    /* only one of flLong and flNotLong */

    if ((flags&(flLong|flNotLong)) == (flLong|flNotLong)) {
	warn("%s: IsLong and IsNotLong cancel out", name);
	flags &= ~(flLong|flNotLong);
    }

    /* only one of flDealloc, flNotDealloc, flMaybeDealloc */

    if (flags & flMaybeDealloc) {
	if (flags & (flDealloc|flNotDealloc)) {
	    warn("%s: Dealloc and NotDealloc ignored with Dealloc[]", name);
	    flags &= ~(flDealloc|flNotDealloc);
	}
    }

    if ((flags&(flDealloc|flNotDealloc)) == (flDealloc|flNotDealloc)) {
	warn("%s: Dealloc and NotDealloc cancel out", name);
	flags &= ~(flDealloc|flNotDealloc);
    }

    return flags;
}

dealloc_t
itCheckDeallocate(const ipc_type_t *it, ipc_flags_t flags, dealloc_t dfault,
		  identifier_t name)
{
    dealloc_t dealloc = dfault;

    if (flags & flMaybeDealloc)
	dealloc = d_MAYBE;
    if (flags & flDealloc)
	dealloc = d_YES;
    if (flags & flNotDealloc)
	dealloc = d_NO;

    if (dealloc == dfault) {
	if (flags & flMaybeDealloc)
	    warn("%s: Dealloc[] is redundant", name);
	if (flags & flDealloc)
	    warn("%s: Dealloc is redundant", name);
	if (flags & flNotDealloc)
	    warn("%s: NotDealloc is redundant", name);
    }

    if (flags & (flMaybeDealloc|flDealloc|flNotDealloc)) {
	/* only give semantic warnings if the user specified something */

	if (dealloc != d_NO) {
	    if (it->itInLine && !it->itIndefinite)
		warn("%s: Dealloc will cause an IPC error", name);
	}
    }

    return dealloc;
}

static enum uselong { NotLong, CanBeLong, ShouldBeLong, MustBeLong, TooLong }
itUseLong(register const ipc_type_t *it)
{
    enum uselong uselong = NotLong;

    if ((it->itInName == MCN_MSGTYPE_POLYMORPHIC) ||
	(it->itOutName == MCN_MSGTYPE_POLYMORPHIC) ||
	it->itVarArray)
	uselong = CanBeLong;

    if ((it->itVarArray && !it->itInLine) || it->itIndefinite)
	uselong = ShouldBeLong;

    if (((it->itInName != MCN_MSGTYPE_POLYMORPHIC) &&
	 (it->itInName >= (1<<8))) ||
	((it->itOutName != MCN_MSGTYPE_POLYMORPHIC) &&
	 (it->itOutName >= (1<<8))) ||
	(it->itSize >= (1<<8)) ||
	(it->itNumber >= (1<<12)))
	uselong = MustBeLong;

    if (((it->itInName != MCN_MSGTYPE_POLYMORPHIC) &&
	 (it->itInName >= (1<<16))) ||
	((it->itOutName != MCN_MSGTYPE_POLYMORPHIC) &&
	 (it->itOutName >= (1<<16))) ||
	(it->itSize >= (1<<16)))
	uselong = TooLong;

    return uselong;
}

bool
itCheckIsLong(const ipc_type_t *it, ipc_flags_t flags, bool dfault,
	      identifier_t name)
{
    bool islong = dfault;

    if (flags & flLong)
	islong = TRUE;
    if (flags & flNotLong)
	islong = FALSE;

    if (islong == dfault) {
	if (flags & flLong)
	    warn("%s: IsLong is redundant", name);
	if (flags & flNotLong)
	    warn("%s: IsNotLong is redundant", name);
    }

    if (flags & (flLong|flNotLong)) {
	enum uselong uselong = itUseLong(it);

	/* only give semantic warnings if the user specified something */

	if (islong && ((int)uselong <= (int)NotLong))
	    warn("%s: doesn't need IsLong", name);
	if (!islong && ((int)uselong >= (int)MustBeLong))
	    warn("%s: too big for IsNotLong", name);
    }

    return islong;
}

/******************************************************
 *  Checks for non-implemented types, conflicting type
 *  flags and whether the long or short form of msg type
 *  descriptor is appropriate. Called after each type statement
 *  is parsed.
 ******************************************************/
static void
itCheckDecl(identifier_t name, register ipc_type_t *it)
{
    enum uselong uselong;

    it->itName = name;

    itCalculateNameInfo(it);

    /* do a bit of error checking, mostly necessary because of
       limitations in Mig */

    if (it->itVarArray) {
	if ((it->itInTrans != strNULL) || (it->itOutTrans != strNULL))
	    error("%s: can't translate variable-sized arrays", name);

	if (it->itDestructor != strNULL)
	    error("%s: can't destroy variable-sized array", name);
    }

    if (it->itVarArray && it->itInLine) {
	if ((it->itElement->itUserType == strNULL) ||
	    (it->itElement->itServerType == strNULL))
	    error("%s: variable-sized in-line arrays need a named base type",
		  name);
    }

    /* process the IPC flag specification */

    it->itFlags = itCheckFlags(it->itFlags, name);

    it->itDeallocate = itCheckDeallocate(it, it->itFlags, d_NO, name);

    uselong = itUseLong(it);
    if (uselong == TooLong)
	warn("%s: too big for mach_msg_type_long_t", name);
    it->itLongForm = itCheckIsLong(it, it->itFlags,
				   (int)uselong >= (int)ShouldBeLong, name);
}

/*
 *  Pretty-prints translation/destruction/type information.
 */
static void
itPrintTrans(const ipc_type_t *it)
{
    if (!streql(it->itName, it->itUserType))
	printf("\tCUserType:\t%s\n", it->itUserType);

    if (!streql(it->itName, it->itServerType))
	printf("\tCServerType:\t%s\n", it->itServerType);

    if (it->itInTrans != strNULL)
       printf("\tInTran:\t\t%s %s(%s)\n",
	      it->itTransType, it->itInTrans, it->itServerType);

    if (it->itOutTrans != strNULL)
       printf("\tOutTran:\t%s %s(%s)\n",
	      it->itServerType, it->itOutTrans, it->itTransType);

    if (it->itDestructor != strNULL)
	printf("\tDestructor:\t%s(%s)\n", it->itDestructor, it->itTransType);
}

/*
 *  Pretty-prints type declarations.
 */
static void
itPrintDecl(identifier_t name, const ipc_type_t *it)
{
    printf("Type %s = ", name);
    if (!it->itInLine)
	printf("^ ");
    if (it->itVarArray)
	if (it->itNumber == 0 || it->itIndefinite)
	    printf("array [] of ");
	else
	    printf("array [*:%d] of ", it->itNumber);
    else if (it->itStruct && ((it->itNumber != 1) ||
			      (it->itInName == MCN_MSGTYPE_CSTRING)))
	printf("struct [%d] of ", it->itNumber);
    else if (it->itNumber != 1)
	printf("array [%d] of ", it->itNumber);

    if (streql(it->itInNameStr, it->itOutNameStr))
	printf("(%s,", it->itInNameStr);
    else
	printf("(%s|%s", it->itInNameStr, it->itOutNameStr);

    printf(" %d:%d%s%s)\n", it->itSize, it->itAlignment,
    	   it->itLongForm ? ", IsLong" : "",
	   it->itDeallocate == d_YES ? ", Dealloc"
				     :it->itDeallocate == d_MAYBE ?", Dealloc[]"
								  : "");

    itPrintTrans(it);

    printf("\n");
}

/*
 *  Handles named type-specs, which can occur in type
 *  declarations or in argument lists.  For example,
 *	type foo = type-spec;	// itInsert will get called later
 *	routine foo(arg : bar = type-spec);	// itInsert won't get called
 */
void
itTypeDecl(identifier_t name, ipc_type_t *it)
{
    itCheckDecl(name, it);

    if (BeVerbose)
	itPrintDecl(name, it);
}

/*
 *  Handles declarations like
 *	type new = name;
 *	type new = inname|outname;
 */
ipc_type_t *
itShortDecl(u_int inname, const_string_t instr, u_int outname,
	    const_string_t outstr, u_int defsize, u_int alignment)
{
    if (defsize == 0)
	error("must use full IPC type decl");

    return itLongDecl(inname, instr, outname, outstr, defsize, defsize, alignment, flNone);
}

/*
 *  Handles declarations like
 *	type new = (name, size, flags...)
 *	type new = (inname|outname, size, flags...)
 */
ipc_type_t *
itLongDecl(u_int inname, const_string_t instr, u_int outname,
	   const_string_t outstr, u_int defsize, u_int size,
	   u_int alignment, ipc_flags_t flags)
{
    register ipc_type_t *it;

    if ((defsize != 0) && (defsize != size))
	warn("IPC type decl has strange size (%u instead of %u)",
	     size, defsize);

    it = itAlloc();
    it->itInName = inname;
    it->itInNameStr = instr;
    it->itOutName = outname;
    it->itOutNameStr = outstr;
    it->itSize = size;
    it->itAlignment = alignment;
    if (inname == MCN_MSGTYPE_CSTRING)
    {
	it->itStruct = FALSE;
	it->itString = TRUE;
    }
    it->itFlags = flags;

    itCalculateSizeInfo(it);
    return it;
}

static ipc_type_t *
itCopyType(const ipc_type_t *old)
{
    register ipc_type_t *new = itAlloc();

    *new = *old;
    new->itName = strNULL;
    new->itNext = itNULL;
    new->itElement = (ipc_type_t *) old;

    /* size info still valid */
    return new;
}

/*
 * A call to itCopyType is almost always followed with itResetType.
 * The exception is itPrevDecl.  Also called before adding any new
 * translation/destruction/type info (see parser.y).
 *
 *	type new = old;	// new doesn't get old's info
 *	type new = array[*:10] of old;
 *			// new doesn't get old's info, but new->itElement does
 *	type new = array[*:10] of struct[3] of old;
 *			// new and new->itElement don't get old's info
 */

ipc_type_t *
itResetType(ipc_type_t *old)
{
    /* reset all special translation/destruction/type info */

    old->itInTrans = strNULL;
    old->itOutTrans = strNULL;
    old->itDestructor = strNULL;
    old->itUserType = strNULL;
    old->itServerType = strNULL;
    old->itTransType = strNULL;
    return old;
}

/*
 *  Handles the declaration
 *	type new = old;
 */
ipc_type_t *
itPrevDecl(identifier_t name)
{
    register ipc_type_t *old;

    old = itLookUp(name);
    if (old == itNULL)
    {
	error("type '%s' not defined", name);
	return itAlloc();
    }
    else
        return itCopyType(old);
}

/*
 *  Handles the declarations
 *	type new = array[] of old;	// number is 0
 *	type new = array[*] of old;	// number is 0
 *	type new = array[*:number] of old;
 */
ipc_type_t *
itVarArrayDecl(u_int number, register const ipc_type_t *old)
{
    register ipc_type_t *it = itResetType(itCopyType(old));

    if (!it->itInLine || it->itVarArray)
	error("IPC type decl is too complicated");
    if (number != 0)
	it->itNumber *= number;
    else {
	/*
	 * Send at most 2048 bytes inline.
	 */
	u_int	bytes;

	bytes = (it->itNumber * it->itSize + 7) / 8;
	it->itNumber = (2048 / bytes) * it->itNumber;
	it->itIndefinite = TRUE;
    }
    it->itVarArray = TRUE;
    it->itStruct = FALSE;
    it->itString = FALSE;

    itCalculateSizeInfo(it);
    return it;
}

/*
 *  Handles the declaration
 *	type new = array[number] of old;
 */
ipc_type_t *
itArrayDecl(u_int number, const ipc_type_t *old)
{
    register ipc_type_t *it = itResetType(itCopyType(old));

    if (!it->itInLine || it->itVarArray)
	error("IPC type decl is too complicated");
    it->itNumber *= number;
    it->itStruct = FALSE;
    it->itString = FALSE;

    itCalculateSizeInfo(it);
    return it;
}

/*
 *  Handles the declaration
 *	type new = ^ old;
 */
ipc_type_t *
itPtrDecl(ipc_type_t *it)
{
    if (!it->itInLine ||
	(it->itVarArray && !it->itIndefinite && (it->itNumber > 0)))
	error("IPC type decl is too complicated");
    it->itNumber = 0;
    it->itIndefinite = FALSE;
    it->itInLine = FALSE;
    it->itStruct = TRUE;
    it->itString = FALSE;

    itCalculateSizeInfo(it);
    return it;
}

/*
 *  Handles the declaration
 *	type new = struct[number] of old;
 */
ipc_type_t *
itStructDecl(u_int number, const ipc_type_t *old)
{
    register ipc_type_t *it = itResetType(itCopyType(old));

    if (!it->itInLine || it->itVarArray)
	error("IPC type decl is too complicated");
    it->itNumber *= number;
    it->itStruct = TRUE;
    it->itString = FALSE;

    itCalculateSizeInfo(it);
    return it;
}

/*
 * Treat 'c_string[n]' as
 * 'array[n] of (MSG_TYPE_STRING_C, 8)'
 */
ipc_type_t *
itCStringDecl(u_int count, bool varying)
{
    register ipc_type_t *it;
    register ipc_type_t *itElement;

    itElement = itShortDecl(MCN_MSGTYPE_CSTRING,
			    "MCN_MSGTYPE_CSTRING",
			    MCN_MSGTYPE_CSTRING,
			    "MCN_MSGTYPE_CSTRING",
			    0, 8);
    itCheckDecl("char", itElement);

    it = itResetType(itCopyType(itElement));
    it->itNumber = count;
    it->itVarArray = varying;
    it->itStruct = FALSE;
    it->itString = TRUE;

    itCalculateSizeInfo(it);
    return it;
}

ipc_type_t *
itMakeCountType(void)
{
    ipc_type_t *it = itAlloc();

    it->itName = "mcn_msgtype_number_t";
    it->itInName = word_size_name;
    it->itInNameStr = word_size_name_string;
    it->itOutName = word_size_name;
    it->itOutNameStr = word_size_name_string;
    it->itSize = word_size_in_bits;
    it->itAlignment = word_size_in_bits;

    itCalculateSizeInfo(it);
    itCalculateNameInfo(it);
    return it;
}

extern ipc_type_t *
itMakeIntType()
{
    ipc_type_t *it = itAlloc();

    it->itName = "int";
    it->itInName = MCN_MSGTYPE_INT32;
    it->itInNameStr = "MCN_MSGTYPE_INT32";
    it->itOutName = MCN_MSGTYPE_INT32;
    it->itOutNameStr = "MCN_MSGTYPE_INT32";
    it->itSize = 32;
    it->itAlignment = 32;

    itCalculateSizeInfo(it);
    itCalculateNameInfo(it);
    return it;
}

ipc_type_t *
itMakePolyType(void)
{
    ipc_type_t *it = itAlloc();

    it->itName = "mcn_msgtype_name_t";
    it->itInName = word_size_name;
    it->itInNameStr = word_size_name_string;
    it->itOutName = word_size_name;
    it->itOutNameStr = word_size_name_string;
    it->itSize = word_size_in_bits;
    it->itAlignment = word_size_in_bits;

    itCalculateSizeInfo(it);
    itCalculateNameInfo(it);
    return it;
}

ipc_type_t *
itMakeDeallocType(void)
{
    ipc_type_t *it = itAlloc();

    it->itName = "boolean_t";
    it->itInName = MCN_MSGTYPE_BOOLEAN;
    it->itInNameStr = "MCN_MSGTYPE_BOOLEAN";
    it->itOutName = MCN_MSGTYPE_BOOLEAN;
    it->itOutNameStr = "MCN_MSGTYPE_BOOLEAN";
    it->itSize = 32;
    it->itAlignment = 32;

    itCalculateSizeInfo(it);
    itCalculateNameInfo(it);
    return it;
}

/*
 * Alignment of predefined basic types
 */
u_int
itAlignmentOf(u_int /*mach_msg_type_name_t*/ typename)
{
    switch (typename) {
	case MCN_MSGTYPE_UNSTRUCTURED:
/*	case MCN_MSGTYPE_BIT:
	case MCN_MSGTYPE_BOOLEAN: */
	case MCN_MSGTYPE_CHAR:
	case MCN_MSGTYPE_BYTE:
/*	case MCN_MSGTYPE_INT8: */
	case MCN_MSGTYPE_REAL:
	case MCN_MSGTYPE_STRING:
/*	case MCN_MSGTYPE_CSTRING:*/ return (0);

	case MCN_MSGTYPE_INT16: return (16);
	case MCN_MSGTYPE_INT32: return (32);
	case MCN_MSGTYPE_INT64: return (64);

    	case MCN_MSGTYPE_MOVERECV:
	case MCN_MSGTYPE_MOVESEND:
	case MCN_MSGTYPE_MOVEONCE:
	case MCN_MSGTYPE_COPYSEND:
	case MCN_MSGTYPE_MAKESEND:
	case MCN_MSGTYPE_MAKEONCE:
	case MCN_MSGTYPE_PORTNAME:
/*	case MCN_MSGTYPE_PORT_RECEIVE:
	case MCN_MSGTYPE_PORT_SEND:
	case MCN_MSGTYPE_PORT_SEND_ONCE: */
	case MCN_MSGTYPE_POLYMORPHIC: return (word_size_in_bits);

	default:
	warn("Internal: alignment for type %d unknown, assuming 0\n",
			typename);
		return (0);
    }
}
/*
 *  Initializes the pre-defined types.
 */
void
init_type(void)
{
    itRetCodeType = itAlloc();
    itRetCodeType->itName = "mcn_return_t";
    itRetCodeType->itInName = MCN_MSGTYPE_INT32;
    itRetCodeType->itInNameStr = "MCN_MSGTYPE_INT32";
    itRetCodeType->itOutName = MCN_MSGTYPE_INT32;
    itRetCodeType->itOutNameStr = "MCN_MSGTYPE_INT32";
    itRetCodeType->itSize = 32;
    itRetCodeType->itAlignment = 32;
    itCalculateSizeInfo(itRetCodeType);
    itCalculateNameInfo(itRetCodeType);

    itDummyType = itAlloc();
    itDummyType->itName = "char *";
    itDummyType->itInName = MCN_MSGTYPE_UNSTRUCTURED;
    itDummyType->itInNameStr = "MCN_MSGTYPE_UNSTRUCTURED";
    itDummyType->itOutName = MCN_MSGTYPE_UNSTRUCTURED;
    itDummyType->itOutNameStr = "MCN_MSGTYPE_UNSTRUCTURED";
    itDummyType->itSize = word_size_in_bits;
    itCalculateSizeInfo(itDummyType);
    itCalculateNameInfo(itDummyType);

    itRequestPortType = itAlloc();
    itRequestPortType->itName = "mcn_portid_t";
    itRequestPortType->itInName = MCN_MSGTYPE_COPYSEND;
    itRequestPortType->itInNameStr = "MCN_MSGTYPE_COPYSEND";
    itRequestPortType->itOutName = MCN_MSGTYPE_PORTSEND;
    itRequestPortType->itOutNameStr = "MCN_MSGTYPE_PORTSEND";
    itRequestPortType->itSize = word_size_in_bits;
    itRequestPortType->itAlignment = word_size_in_bits;
    itCalculateSizeInfo(itRequestPortType);
    itCalculateNameInfo(itRequestPortType);

    itZeroReplyPortType = itAlloc();
    itZeroReplyPortType->itName = "mcn_portid_t";
    itZeroReplyPortType->itInName = 0;
    itZeroReplyPortType->itInNameStr = "0";
    itZeroReplyPortType->itOutName = 0;
    itZeroReplyPortType->itOutNameStr = "0";
    itZeroReplyPortType->itSize = word_size_in_bits;
    itZeroReplyPortType->itAlignment = word_size_in_bits;
    itCalculateSizeInfo(itZeroReplyPortType);
    itCalculateNameInfo(itZeroReplyPortType);

    itRealReplyPortType = itAlloc();
    itRealReplyPortType->itName = "mcn_portid_t";
    itRealReplyPortType->itInName = MCN_MSGTYPE_MAKEONCE;
    itRealReplyPortType->itInNameStr = "MCN_MSGTYPE_MAKEONCE";
    itRealReplyPortType->itOutName = MCN_MSGTYPE_PORTONCE;
    itRealReplyPortType->itOutNameStr = "MCN_MSGTYPE_PORTONCE";
    itRealReplyPortType->itSize = word_size_in_bits;
    itRealReplyPortType->itAlignment = word_size_in_bits;
    itCalculateSizeInfo(itRealReplyPortType);
    itCalculateNameInfo(itRealReplyPortType);

    itWaitTimeType = itMakeIntType();
    itMsgOptionType = itMakeIntType();
}

/******************************************************
 *  Make sure return values of functions are assignable.
 ******************************************************/
void
itCheckReturnType(identifier_t name, const ipc_type_t *it)
{
    if (!it->itStruct)
	error("type of %s is too complicated", name);
    if ((it->itInName == MCN_MSGTYPE_POLYMORPHIC) ||
	(it->itOutName == MCN_MSGTYPE_POLYMORPHIC))
	error("type of %s can't be polymorphic", name);
}


/******************************************************
 *  Called by routine.c to check that request ports are
 *  simple and correct ports with send rights.
 ******************************************************/
void
itCheckRequestPortType(identifier_t name, const ipc_type_t *it)
{
    if (((it->itOutName != MCN_MSGTYPE_PORTSEND) &&
	 (it->itOutName != MCN_MSGTYPE_PORTONCE) &&
	 (it->itOutName != MCN_MSGTYPE_POLYMORPHIC)) ||
	(it->itNumber != 1) ||
	(it->itSize != word_size_in_bits) ||
	(it->itAlignment != word_size_in_bits) ||
	!it->itInLine ||
	it->itDeallocate != d_NO ||
	!it->itStruct ||
	it->itVarArray)
	error("argument %s isn't a proper request port", name);
}


/******************************************************
 *  Called by routine.c to check that reply ports are
 *  simple and correct ports with send rights.
 ******************************************************/
void
itCheckReplyPortType(identifier_t name, const ipc_type_t *it)
{
    if (((it->itOutName != MCN_MSGTYPE_PORTSEND) &&
	 (it->itOutName != MCN_MSGTYPE_PORTONCE) &&
	 (it->itOutName != MCN_MSGTYPE_POLYMORPHIC) &&
	 (it->itOutName != 0)) ||
	(it->itNumber != 1) ||
	(it->itSize != word_size_in_bits) ||
	(it->itAlignment != word_size_in_bits) ||
	!it->itInLine ||
	it->itDeallocate != d_NO ||
	!it->itStruct ||
	it->itVarArray)
	error("argument %s isn't a proper reply port", name);
}


/******************************************************
 *  Used by routine.c to check that WaitTime is a
 *  simple bit 32 integer.
 ******************************************************/
void
itCheckIntType(identifier_t name, const ipc_type_t *it)
{
    if ((it->itInName != MCN_MSGTYPE_INT32) ||
	(it->itOutName != MCN_MSGTYPE_INT32) ||
	(it->itNumber != 1) ||
	(it->itSize != 32) ||
	(it->itAlignment != 32) ||
	!it->itInLine ||
	it->itDeallocate != d_NO ||
	!it->itStruct ||
	it->itVarArray)
	error("argument %s isn't a proper integer", name);
}
void
itCheckNaturalType(name, it)
    identifier_t name;
    ipc_type_t *it;
{
    if ((it->itInName != word_size_name) ||
	(it->itOutName != word_size_name) ||
	(it->itNumber != 1) ||
	(it->itSize != word_size_in_bits) ||
	(it->itAlignment != word_size_in_bits) ||
	!it->itInLine ||
	it->itDeallocate != d_NO ||
	!it->itStruct ||
	it->itVarArray)
	error("argument %s should have been a %s", name, word_size_name_string);
}
