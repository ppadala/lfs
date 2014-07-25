#ifndef _CLEANER_H
#define _CLEANER_H

#include <time.h>
#include <unistd.h>

#include "lfs.h"

#define DUMP_SUM_HEADER         0x0001
#define DUMP_INODE_ADDRS        0x0002
#define DUMP_FINFOS             0x0004 
#define DUMP_ALL                0xFFFF

#define BUSY_LIM        0.50
#define IDLE_LIM        0.90

#define	MIN_SEGS(lfsp)		(3)
#define	NUM_TO_CLEAN(fsp)	(1)

#define MNAMELEN 256

#define MAXLOADS	3
#define	ONE_MIN		0
#define	FIVE_MIN	1
#define	FIFTEEN_MIN	2

#define TIME_THRESHOLD	5	/* Time to tell looping from running */
#define LOOP_THRESHOLD	5	/* Number of looping respawns before exit */

typedef struct fs_info {
	char *fs_name;		/* partition name */
	char mntname[MNAMELEN];	/* where the particular lfs is mounted */
	struct	lfs_super_block	fi_lfs;	/* superblock */
	struct  ifile fi_ifile;		/* ifile */
	struct  cleanerinfo fi_cip;	/* cleaner info from segusetable file */
	struct  segusage *fi_segusep;	/* seguse array from segusetable file */
	int nseguse;			/* seguse array size */
	unsigned fi_ifile_length;       /* length of the ifile */
	time_t  fi_fs_tstamp;           /* last fs activity, per ifile */
}FS_INFO;

struct summary {
	struct segsum segsum;
	struct finfo *fis;
	daddr_t *inos;
	int nblocks; /* number of data+inode blocks in the segment */
	int segnum; /* segment number to which this summary belongs */
};

#define LFS_IDENT "lfs_cleanerd"

static inline struct ifile_entry *IFILE_ENTRY(FS_INFO *fsp, ino_t ino)
{
	return &fsp->fi_ifile.ifes[ino - 1];
}
/*
 * USEFUL DEBUGGING FUNCTIONS:
 */
#define PRINT_FINFO(fp, ip) if(debug > 1) { \
	syslog(LOG_DEBUG,"    %s %s%d version %d nblocks %d\n", \
	    (ip)->ife_version > (fp)->fi_version ? "TOSSING" : "KEEPING", \
	    "FINFO for inode: ", (fp)->fi_ino, \
	    (fp)->fi_version, (fp)->fi_nblocks); \
}

#define PRINT_INODE(b, bip) if(debug > 1) { \
	syslog(LOG_DEBUG,"\t%s inode: %d daddr: 0x%lx create: %s", \
	    b ? "KEEPING" : "TOSSING", (bip)->bi_inode, (long)(bip)->bi_daddr, \
	    ctime((time_t *)&(bip)->bi_segcreate)); \
}

#define PRINT_BINFO(bip) if(debug > 1 ) { \
	syslog(LOG_DEBUG,"inode: %d lbn: %d daddr: 0x%lx bp: 0x%lx create: %s", \
	    (bip)->bi_inode, (bip)->bi_lbn, (unsigned long)(bip)->bi_daddr, \
	    (bip)->bi_bp, ctime((time_t *)&(bip)->bi_segcreate)); \
}

#define PRINT_SEGUSE(sup, n) if(debug > 1) { \
	syslog(LOG_DEBUG,"Segment %d nbytes=%lu\tflags=%c%c nsums=%d lastmod: %s", \
			n, (unsigned long)(sup)->su_nbytes, \
			(sup)->su_flags & SEGUSE_DIRTY ? 'D' : 'C', \
			(sup)->su_flags & SEGUSE_ACTIVE ? 'A' : ' ', \
			(sup)->su_nsums, \
			ctime((time_t *)&(sup)->su_lastmod)); \
}

/* library */
char *fs_getmntinfo(char *name, const char *type);
void reread_fs_info(struct fs_info *fsp, int use_mmap);
struct fs_info *get_fs_info(char *fs_name, char *mntname, int use_mmap);

/* porting stuff from BSD */
/* I do not like typedefs, for the first version, let's bear the kludge */
typedef struct segusage SEGUSE;
typedef struct segsum SEGSUM;
typedef struct finfo FINFO;
typedef struct ifile IFILE;
typedef struct cleanerinfo CLEANERINFO;
typedef struct fs_info FSINFO;

/* Kludge */
#define INOPB 1

static inline int daddrtosn(daddr_t daddr)
{
	return dtosn(daddr / LFS_BSIZE);
}

#define segtod(a, b) (LFS_SEGSIZE * 4)
#define btofsb(a, b) (LFS_SEGSIZE * 4)
#define fsbtob(a, b) (LFS_SEGSIZE * 4)

#define MAXPHYS (64 * LFS_BSIZE)

#endif
