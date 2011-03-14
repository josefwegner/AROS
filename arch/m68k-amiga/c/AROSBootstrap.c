/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

/* This loads relocatable AROS ROM ELF images, and gzipped
 * image. Use either the aros-amiga-m68k-reloc.elf or
 * aros.elf.gz images. The fully linked aros-amiga-m68k.elf
 * image cannot be loaded by this application.
 *
 * As you can probably guess, you will need at least 2MB of
 * MEMF_KICK RAM to get this to work.
 *
 * Also - no AROS specific code can go in here! We have to run
 * on AOS 1.3 and up.
 */
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#include <zlib.h>

#define NO_SYSBASE_REMAP
#include <rom/dos/internalloadseg_elf.c>

struct DosLibrary *DOSBase;

/* Define these here for zlib so that we don't
 * pull in arosc.library.
 */
void *malloc(int size)
{
    return AllocVec(size, MEMF_ANY);
}

void free(void *ptr)
{
    FreeVec(ptr);
}

int open(const char *name, int mode)
{
    return (int)Open(name, MODE_OLDFILE);
}

void close(int fd)
{
    Close((BPTR)fd);
}

int read(int fd, void *buff, size_t len)
{
    return Read((BPTR)fd, buff, (LONG)len);
}

off_t lseek(int fd, off_t offset, int whence)
{
    LONG mode = SEEK_SET;
    LONG err;

    switch (whence) {
    case SEEK_CUR: mode = OFFSET_CURRENT; break;
    case SEEK_SET: mode = OFFSET_BEGINNING; break;
    case SEEK_END: mode = OFFSET_END; break;
    default: return -1;
    }

    err = Seek((BPTR)fd, (LONG)offset, mode);
    if (err < 0)
    	return -1;

    return Seek((BPTR)fd, 0, OFFSET_CURRENT);       
}

/* Backcalls for InternalLoadSeg_ELF
 * using the gzip backend.
 */
static AROS_UFH4(LONG, elfRead,
	AROS_UFHA(BPTR,  file, D1),
	AROS_UFHA(void *, buf, D2),
	AROS_UFHA(LONG,  size, D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    return gzread((gzFile)file, buf, (unsigned)size);

    AROS_USERFUNC_EXIT
}

static AROS_UFH4(LONG, elfSeek,
	AROS_UFHA(BPTR,  file, D1),
	AROS_UFHA(LONG,   pos, D2),
	AROS_UFHA(LONG,  mode, D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT
    int whence;
    LONG ret;
    LONG oldpos;

    switch (mode) {
    case OFFSET_CURRENT  : whence = SEEK_CUR; break;
    case OFFSET_END      : whence = SEEK_END; break;
    case OFFSET_BEGINNING: whence = SEEK_SET; break;
    default: return -1;
    }

    oldpos = (LONG)gzseek((gzFile)file, 0, SEEK_CUR);

    ret = (LONG)gzseek((gzFile)file, (z_off_t)pos, whence);
    if (ret < 0)
    	return -1;

    return oldpos;

    AROS_USERFUNC_EXIT
}

static struct MinList mlist;

static AROS_UFH3(APTR, elfAlloc,
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(ULONG, flags, D1),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    struct MemList *ml;

    size += sizeof(*ml);

    ml = AllocMem(size, flags | MEMF_KICK | (SysBase->LibNode.lib_Version >= 36 ? MEMF_REVERSE : 0));

    ml->ml_NumEntries = 1;
    ml->ml_ME[0].me_Addr = (APTR)ml;
    ml->ml_ME[0].me_Length = size;

    AddTail(&mlist, ml);

    return &ml[1];

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, elfFree,
	AROS_UFHA(APTR, addr, A1),
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    struct MemList *ml;

    addr -= sizeof(*ml);
    size += sizeof(*ml);

    Remove(addr);

    FreeMem(addr, size);

    AROS_USERFUNC_EXIT
}

static BPTR ROMLoad(const char *filename)
{
    gzFile gzf;
    BPTR rom = BNULL;
    SIPTR funcarray[] = {
    	(SIPTR)elfRead,
    	(SIPTR)elfAlloc,
    	(SIPTR)elfFree,
    	(SIPTR)elfSeek,
    };

    if ((gzf = gzopen(filename, "rb"))) {
    	gzbuffer(gzf, 65536);

    	rom = InternalLoadSeg_ELF((BPTR)gzf, BNULL, funcarray, NULL, DOSBase);
    	if (rom == BNULL) {
    	    Printf("%s: Can't parse\n", filename);
    	}

    	gzclose_r(gzf);
    } else {
    	Printf("%s: Can't open\n", filename);
    }

    return rom;
}

#define FAKEBASE 0x200
#define FAKEBASESIZE 558
#define COLDCAPTURE (FAKEBASE + FAKEBASESIZE + 16)

/* Theory of operation:

- create fake sysbase in low chip memory (We can't use original because it is not binary compatible with AROS one)
- set correct checksum and point ColdCapture to our routine (also in low chip)
- reset the system (autoconfig devices disappear, including most expansion RAM and ROM overlay disables chip RAM)
- original ROM code now disables ROM overlay and checks KS checksum and jumps to our ColdCapture routine
- above step is needed because in worst case ALL RAM disappear at reset and there
  are also accelerators that reset the CPU completely when reset instruction is executed.
- now we have control again, autoconfig devices are still unconfigured but at least we have chip ram.
- jump to AROS ROM code entry point which detects romloader mode and automatically reserves RAM used by "ROM" code
- AROS ROM creates new proper execbase and copies ColdCapture
- normal boot starts

*/

static UWORD GetSysBaseChkSum(struct ExecBase *sysbase)
{
     UWORD sum = 0;
     UWORD *p = (UWORD*)&sysbase->SoftVer;
     while (p <= &sysbase->ChkSum)
	sum += *p++;
     return sum;
}


/* reset VBR and switch off MMU */
static void setcpu(void)
{
    asm(
	".chip 68040\n"
	"move.l	4,%a0\n"
	"move.w	296(%a0),%d1\n"
	"moveq	#0,%d0\n"
	"btst	#0,%d1\n"
	"beq.s	novbr\n"
	"movec	%d0,%vbr\n"
"novbr:	moveq	#1,%d0\n"
	"beq.s	cpudone\n"
	"movec	%d0,%cacr\n"
	"btst	#3,%d1\n"
	"beq.s	not040\n"
	"movec	%d0,%tc\n"
	"movec	%d0,%dtt0\n"
	"movec	%d0,%dtt1\n"
	"movec	%d0,%itt0\n"
	"movec	%d0,%itt1\n"
	"cpusha	%bc\n"
	"bra.s	cpudone\n"
"not040: btst	#2,%d1\n"
	"beq.s	cpudone\n"
	"lea	zero(%pc),%a0\n"
	".long	0xf0104000\n"
	".long	0xf0100c00\n"
	".long	0xf0100800\n"
	"bra.s cpudone\n"
"zero:	.long	0,0\n"
"cpudone:\n"
    );
}

static void reboot(void)
{
    asm(
	"lea 0xf80002,%a0\n"
	"reset\n"
	"jmp (%a0)\n"
    );
}

AROS_UFH2(void, putser,
	AROS_UFHA(UBYTE, chr,       D0),
	AROS_UFHA(APTR , PutChData, A3))
{
    AROS_USERFUNC_INIT
    RawPutChar(chr);
    AROS_USERFUNC_EXIT
}
    
APTR entry;

static void supercode(void)
{
    ULONG *fakesys;
    struct ExecBase *sysbase;
    ULONG *traps = 0;

    setcpu();
    fakesys = (ULONG*)FAKEBASE;

    *fakesys++ = traps[31]; // Level 7
    *fakesys++ = 0x4ef9 | ((IPTR)entry >> 16);
    *fakesys++ = ((IPTR)entry << 16) | 0x4e75;
    sysbase = (struct ExecBase*)fakesys; 
    traps[1] = (IPTR)sysbase;

    memset(sysbase, 0, FAKEBASESIZE);
    sysbase->ColdCapture = entry;
    sysbase->MaxLocMem = 512 * 1024;
    sysbase->KickMemPtr = (APTR)mlist.mlh_Head;
    sysbase->KickCheckSum = (APTR)SumKickData();
    sysbase->ChkBase=~(IPTR)sysbase;
    sysbase->ChkSum = 0;
    sysbase->ChkSum = GetSysBaseChkSum(sysbase) ^ 0xffff;

    RawDoFmt("KickMemPtr 0x%lx\n", &sysbase->KickMemPtr, putser, NULL);
    RawDoFmt("Ready to go!\nColdCapture is 0x%lx\n", &sysbase->ColdCapture, putser, NULL);

    SysBase = (struct ExecBase*)sysbase;
    CacheClearU();

    reboot();
}

void BootROM(BPTR romlist)
{
    APTR GfxBase;

    if (0 && (GfxBase = OpenLibrary("graphics.library", 0))) {
    	LoadView(NULL);
    	LoadView(NULL);
    	CloseLibrary(GfxBase);
    }

    entry = BADDR(romlist)+sizeof(ULONG);

    /* We're off in the weeds now. */
    Disable();

    /* Make list singly linked, and join
     * with the existing KickMem list
     */
    mlist.mlh_TailPred->mln_Succ = SysBase->KickMemPtr;

    Supervisor(supercode);
}

int __nocommandline;

int main(void)
{
    int err = 1;

    /* See if we're already running on AROS.
     */
    struct Library *sbl = (APTR)SysBase;
    if (sbl->lib_Version > 40)
    	return RETURN_OK;

    NEWLIST(&mlist);

    DOSBase = (APTR)OpenLibrary("dos.library", 0);
    if (DOSBase != NULL) {
    	BPTR ROMSegList;
    	IPTR args[1] = {
    	    (IPTR)"aros.elf.gz",
    	};
    	struct RDArgs *rda;

    	if ((rda = ReadArgs("FILE", args, NULL)) != NULL) {

    	    ROMSegList = ROMLoad((const char *)args[0]);
    	    if (ROMSegList != BNULL) {
    	    	Printf("Successfully loaded ROM\n");

    	    	BootROM(ROMSegList);

    	    	UnLoadSeg(ROMSegList);
    	    } else {
    	    	Printf("Can't load ROM ELF file\n");
    	    }
    	    FreeArgs(rda);
    	} else {
    	    Printf("Can't parse arguments\n");
    	}
    	/* Load ROM image */

    	CloseLibrary((APTR)DOSBase);
    }
    return err;
}
