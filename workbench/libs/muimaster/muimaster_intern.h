/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef MUIMASTER_INTERN_H
#define MUIMASTER_INTERN_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif

#ifdef __AROS__
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#ifndef CLIB_BOOPSISTUBS_H
#   include <clib/boopsistubs.h>
#endif

#else

#define AROS_LIBFUNC_INIT
#define AROS_LIBBASE_EXT_DECL(a,b) extern a b;
#define AROS_LIBFUNC_EXIT

#endif

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

/* Sometype defs in AROS */
#ifndef __AROS__
#ifndef _AROS_TYPES_DEFINED
typedef unsigned long IPTR;
typedef long STACKLONG;
typedef unsigned long STACKULONG;
#define _AROS_TYPES_DEFINED
#endif
#endif

/****************************************************************************************/

struct MUIMasterBase_intern
{
    struct Library		library;
    struct ExecBase		*sysbase;
    BPTR			seglist;

    struct DosLibrary  	    	*dosbase;
    struct UtilityBase		*utilitybase;
    struct Library  	    	*aslbase;
    struct GfxBase  	    	*gfxbase;
    struct Library  	    	*layersbase;
    struct IntuitionBase    	*intuibase;
    struct Library  	    	*cxbase;
    struct Library  	    	*keymapbase;
    struct Library		*gadtoolsbase;
    struct Library  	    	*iffparsebase;
    struct Library  	    	*diskfontbase;
    struct Library  	    	*iconbase;
    struct Library  	    	*cybergfxbase;
#ifdef HAVE_COOLIMAGES
    struct Library  	    	*coolimagesbase;
#endif
    
/*  struct Library  	    	*datatypesbase; */

    struct SignalSemaphore ZuneSemaphore; /* Used when accessing global data */

    struct IClass **Classes;
    int     ClassCount;
    int     ClassSpace;
};

/****************************************************************************************/

#define REDUCE_FLICKER_TEST 0

/****************************************************************************************/

#undef MUIMB
#define MUIMB(b)	((struct MUIMasterBase_intern *)b)

#ifdef __AROS__

#include <proto/muimaster.h>

#undef SysBase
#define SysBase     	(MUIMB(MUIMasterBase)->sysbase)

#undef DOSBase
#define DOSBase     	(MUIMB(MUIMasterBase)->dosbase)

#undef UtilityBase
#define UtilityBase	(MUIMB(MUIMasterBase)->utilitybase)

#undef AslBase
#define AslBase     	(MUIMB(MUIMasterBase)->aslbase)

#undef GfxBase
#define GfxBase     	(MUIMB(MUIMasterBase)->gfxbase)

#undef LayersBase
#define LayersBase     	(MUIMB(MUIMasterBase)->layersbase)

#undef IntuitionBase
#define IntuitionBase  	(MUIMB(MUIMasterBase)->intuibase)

#undef CxBase
#define CxBase	    	(MUIMB(MUIMasterBase)->cxbase)

#undef KeymapBase
#define KeymapBase  	(MUIMB(MUIMasterBase)->keymapbase)

#undef GadToolsBase
#define GadToolsBase  	(MUIMB(MUIMasterBase)->gadtoolsbase)

#undef IFFParseBase
#define IFFParseBase  	(MUIMB(MUIMasterBase)->iffparsebase)

#undef DiskfontBase
#define DiskfontBase  	(MUIMB(MUIMasterBase)->diskfontbase)

#undef IconBase
#define IconBase  	(MUIMB(MUIMasterBase)->iconbase)

#undef CyberGfxBase
#define CyberGfxBase  	(MUIMB(MUIMasterBase)->cybergfxbase)

#undef CoolImagesBase
#define CoolImagesBase	(MUIMB(MUIMasterBase)->coolimagesbase)

/*
#undef DataTypesBase
#define DataTypesBase	(MUIMB(MUIMasterBase)->datatypesbase)
*/
#else /* ! __AROS__ */

#undef SysBase
#define SysBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->sysbase)

#undef DOSBase
#define DOSBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->dosbase)

#undef UtilityBase
#define UtilityBase	(((struct MUIMasterBase_intern *)MUIMasterBase)->utilitybase)

#undef AslBase
#define AslBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->aslbase)

#undef GfxBase
#define GfxBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->gfxbase)

#undef LayersBase
#define LayersBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->layersbase)

#undef IntuitionBase
#define IntuitionBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->intuibase)

#undef CxBase
#define CxBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->cxbase)

#undef KeymapBase
#define KeymapBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->keymapbase)

#undef GadToolsBase
#define GadToolsBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->gadtoolsbase)

#undef IFFParseBase
#define IFFParseBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->iffparsebase)

#undef DiskfontBase
#define DiskfontBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->diskfontbase)

#undef CyberGfxBase
#define CyberGfxBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->cybergfxbase)

#undef CoolImagesBase
#define CoolImagesBase	(((struct MUIMasterBase_intern *)MUIMasterBase)->coolimagesbase)

/*
#undef DataTypesBase
#define DataTypesBase	(((struct MUIMasterBase_intern *)MUIMasterBase)->datatypesbase)
*/

#ifndef _COMPILER_H
#include "compiler.h"
#endif

#endif /* ! __AROS__ */

#endif /* MUIMASTER_INTERN_H */
