#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <sys/statvfs.h>
#include <sys/mman.h>

#include <linux/types.h>

#include "lfs.h"

#include "cleaner.h"

int	 bi_compare(const void *, const void *);
int	 bi_toss(const void *, const void *, const void *);
void	 get_ifile(FS_INFO *, int);
int	 get_superblock(FS_INFO *, struct lfs_super_block *);
int	 pseg_valid(FS_INFO *, caddr_t, daddr_t, struct summary *);
int      pseg_size(daddr_t, FS_INFO *, SEGSUM *);
void 	 log_exit(int gooderr, int pri, char *fmt, ...);

extern int debug;
extern u_long cksum(void *, size_t);	/* XXX */

static int ifile_fd = -1;
static int seguse_fd = -1;
static int dev_fd = -1;

extern int debug;

void get(int fd, off_t off, void *p, size_t len)
{
        int rbytes;

        if (lseek(fd, off, SEEK_SET) < 0) {
            syslog(LOG_ERR, "Exiting: %s: lseek: %m", LFS_IDENT);
            exit(1);
        }
        if ((rbytes = read(fd, p, len)) < 0) {
            syslog(LOG_ERR, "Exiting: %s: read: %m", LFS_IDENT);
            exit(1);
        }
        if (rbytes != len) {
            syslog(LOG_ERR, "Exiting: %s: short read (%d, not %ld)",
                   LFS_IDENT, rbytes, (long)len);
            exit(1);
        }
}


char *fs_getmntinfo(char *name, const char *type)
{	FILE *fp = NULL;
	char buf[4096];
	char tempname[MNAMELEN];
	char tempmnt[MNAMELEN];
	char temptype[16];
	char *mntname = NULL;

	fp = fopen("/proc/mounts", "r");
	if(!fp) {
		perror("");
		exit(1);
	}

	while(fscanf(fp, "%s %s %s %*s %*s %*s\n", tempname, tempmnt, temptype) > 0) {
	//printf("name=%s,mnt=%s,type=%s\n", tempname, tempmnt, temptype);
		if(strcmp(name, tempname) == 0 && strcmp(type, temptype) == 0) {
			mntname = (char *)calloc(MNAMELEN, sizeof(char));
			mntname[0] = '\0';
			strncpy(mntname, tempmnt, MNAMELEN);
			break;
		}
	}

	return mntname;
}

int get_superblock(struct fs_info *fsp, struct lfs_super_block *sbp)
{
	if(dev_fd == -1) {
		dev_fd = open(fsp->fs_name, O_RDONLY);
		if(dev_fd < 0)
			return -1;
	}

	get(dev_fd, 0, sbp, sizeof(*sbp));
		
	return (0);
}

int get_ifile_fd(struct fs_info *fsp)
{
	char name[MNAMELEN];

	if(ifile_fd == -1) {
		sprintf(name, "%s/%s", fsp->mntname, LFS_IFILE_NAME);
		ifile_fd = open(name, O_RDONLY);
		if(ifile_fd < 0)
			log_exit(0, LOG_ERR, 
				 "get_ifile: cannot open ifile from %s: %m",
				 fsp->mntname);
	}
	return ifile_fd;
}

void get_ifile(struct fs_info *fsp, int use_mmap)
{
	__u8 *ifp;
	int count, i;
	struct stat file_stat;

	if(ifile_fd == -1)
		get_ifile_fd(fsp);
	else 
		lseek(ifile_fd, (off_t)0, SEEK_SET);
	if (fstat(ifile_fd, &file_stat) == -1)
		log_exit(EBADF, LOG_ERR, "get_ifile: fstat failed: %m");
	printf("ifile size = %d\n", file_stat.st_size);
	fsp->fi_fs_tstamp = file_stat.st_mtime;
	if (use_mmap && file_stat.st_size == fsp->fi_ifile_length) {
		/* IFILE hasn't changed in size */
		return;
	}
	if(use_mmap) {
		log_exit(0, LOG_ERR, "get_ifile: mmap is not implemented in LFS");
	}
	else {
		/* free the old ifes */
		if(fsp->fi_ifile.ifes)
			free(fsp->fi_ifile.ifes);
		if ((ifp = (__u8 *)malloc(file_stat.st_size)) == NULL)
			log_exit(0, LOG_ERR, "get_ifile: malloc failed (%m)");
redo_read:
		count = read(ifile_fd, ifp, (size_t)file_stat.st_size);

		if (count < 0)
			log_exit(EIO, LOG_ERR, "get_ifile: bad read (%m)");
		else if (count < file_stat.st_size) {
			syslog(LOG_WARNING, "get_ifile (%m)");
			if (lseek(ifile_fd, 0, SEEK_SET) < 0)
                                log_exit(0, LOG_ERR,
					 "get_ifile: bad ifile lseek (%m)");
			goto redo_read;
		}
	}
	fsp->fi_ifile_length = file_stat.st_size;

	/* fill the fi_ifile contents */
	fsp->fi_ifile.n_ife = fsp->fi_lfs.s_nino;
	fsp->fi_ifile.ifes = (struct ifile_entry *)ifp;

	dump_ifile(fsp);
}

void get_seguse_table(struct fs_info *fsp, int use_mmap)
{	__u8 *ifp;
	int count, i;
	struct stat file_stat;

	/* zero out the old cleaner info */
	memset(&fsp->fi_cip, 0, sizeof(struct cleanerinfo));
	if(seguse_fd == -1) {

		char name[MNAMELEN];

		sprintf(name, "%s/%s", fsp->mntname, LFS_SEGUSE_TABLE_NAME);
		seguse_fd = open(name, O_RDONLY);
		if(seguse_fd < 0)
			log_exit(0, LOG_ERR,
				 "get_seguse: cannot open seguse table file from %s: %m",
				 fsp->mntname);
	}
	else 
		lseek(seguse_fd, (off_t)0, SEEK_SET);
	
	if (fstat(seguse_fd, &file_stat) == -1)
		log_exit(EBADF, LOG_ERR, "get_seguse: fstat failed: %m");
	printf("seguse table size = %d\n", file_stat.st_size);
	fsp->fi_fs_tstamp = file_stat.st_mtime;
	
	if(use_mmap) {
		log_exit(0, LOG_ERR, "get_seguse: mmap is not implemented in LFS");
	}
	else {
		/* free the old segusages */
		if(fsp->fi_segusep) {
			/* FIXME: yuck, not intuitive */
			fsp->fi_segusep -= sizeof(struct cleanerinfo);
			free(fsp->fi_segusep);
		}
		if ((ifp = (__u8 *)malloc(file_stat.st_size)) == NULL)
			log_exit(0, LOG_ERR, "get_seguse: malloc failed (%m)");
redo_read:
		count = read(seguse_fd, ifp, (size_t)file_stat.st_size);

		if (count < 0)
			log_exit(EIO, LOG_ERR, "get_seguse: bad read (%m)");
		else if (count < file_stat.st_size) {
			syslog(LOG_WARNING, "get_seguse (%m)");
			if (lseek(seguse_fd, 0, SEEK_SET) < 0)
                                log_exit(0, LOG_ERR,
					 "get_seguse : bad ifile lseek (%m)");
			goto redo_read;
		}
	}

	memcpy(&fsp->fi_cip, ifp, sizeof(struct cleanerinfo));
	fsp->fi_segusep = (struct segusage *)(ifp + sizeof(struct cleanerinfo));
	fsp->nseguse = (count - sizeof(struct cleanerinfo)) / sizeof(struct segusage);
	dump_cleaner_info(&fsp->fi_cip);
	dump_segusetbl(fsp);
}

struct fs_info *get_fs_info(char *fs_name, char *mntname, int use_mmap)
{
	struct fs_info *fsp;

	if ((fsp = malloc(sizeof(*fsp))) == NULL)
		return NULL;
	memset(fsp, 0, sizeof(*fsp));

	fsp->fs_name = fs_name;
	strncpy(fsp->mntname, mntname, MNAMELEN);
	if (get_superblock(fsp, &fsp->fi_lfs)) {
		log_exit(0, LOG_ERR, "get_fs_info: get_superblock failed (%m)");
        }
	get_ifile(fsp, use_mmap);
	get_seguse_table(fsp, use_mmap);
	return (fsp);
}

void reread_fs_info(struct fs_info *fsp, int use_mmap)
{
	get_ifile(fsp, use_mmap);
	get_seguse_table(fsp, use_mmap);
}

int mmap_segment(struct fs_info *fsp, int segment, caddr_t *segbuf, int use_mmap)
{
extern int seg_size();

	struct lfs_super_block *lfsp;
	daddr_t seg_daddr;	/* base disk address of segment */
	size_t ssize;

	lfsp = &fsp->fi_lfs;

	/* get the disk address of the beginning of the segment */
	seg_daddr = LFS_SEGSTART * LFS_BSIZE + seg_size() * segment;
	ssize = seg_size();

	lseek(dev_fd, 0, SEEK_SET);

	if (use_mmap) {
		*segbuf = mmap((caddr_t)0, ssize, PROT_READ,
		    MAP_FILE|MAP_SHARED, dev_fd, seg_daddr);
		if (*(long *)segbuf < 0) {
                        syslog(LOG_WARNING,"mmap_segment: mmap failed: %m");
			return (0);
		}
	} else {
                if(debug > 1)
                        syslog(LOG_DEBUG, "mmap_segment\tseg_daddr: %lu\tseg_size: %lu",
                               (u_long)seg_daddr, (u_long)ssize);
           
		/* malloc the space for the buffer */
		*segbuf = malloc(ssize);
		if (!*segbuf) {
			syslog(LOG_WARNING,"mmap_segment: malloc failed: %m");
			return (0);
		}

		/* read the segment data into the buffer */
		if (lseek(dev_fd, seg_daddr, SEEK_SET) != seg_daddr) {
			syslog(LOG_WARNING,"mmap_segment: bad lseek: %m");
			free(*segbuf);
			return (-1);
		}
		if (read(dev_fd, *segbuf, ssize) != ssize) {
			syslog(LOG_WARNING,"mmap_segment: bad read: %m");
			free(*segbuf);
			return (-1);
		}
	}
	/* close (fid); */

	return (0);
}

void munmap_segment(struct fs_info *fsp, caddr_t seg_buf, int use_mmap)
{
	if (use_mmap)
		munmap(seg_buf, seg_size(&fsp->fi_lfs));
	else
		free(seg_buf);
}



static int getdevfd(FS_INFO *fsp)
{
	if (dev_fd != -1)
		return dev_fd;
		
	if ((dev_fd = open(fsp->fs_name, O_RDONLY)) == -1)
		log_exit(0, LOG_ERR, "Cannot open `%s' (%m)", fsp->fs_name);
	return dev_fd;
}


/*
 * Read a block from disk.
 */
int
get_rawblock(FS_INFO *fsp, char *buf, size_t size, daddr_t daddr)
{
	return pread(getdevfd(fsp), buf, size, fsbtob(&fsp->fi_lfs,
	    (off_t)daddr));
}

/*
 * Read an inode from disk.
 */
struct lfs_inode *get_dinode(FS_INFO *fsp, ino_t ino)
{
        static struct lfs_inode dino;
        struct lfs_inode *dip, *dib;
        struct lfs_super_block *lfsp;
	BLOCK_INFO bi;

        lfsp = &fsp->fi_lfs;
#if 0
	/*
	 * Locate the inode block and find the inode.
	 * Use this to know how large the file is.
	 */
	memset(&bi, 0, sizeof(bi));
	bi.bi_inode = ino;
	bi.bi_lbn = LFS_UNUSED_LBN; /* We want the inode */
	if (lfs_bmapv_emul(ifile_fd, &bi, 1) < 0) {
		syslog(LOG_WARNING, "LIOCBMAPV: %m");
		return NULL;
	}
	if (bi.bi_daddr <= 0)
		return NULL;

	lseek(getdevfd(fsp), (off_t)0, SEEK_SET);
	if ((dib = malloc(LFS_BSIZE)) == NULL) {
		syslog(LOG_WARNING, "get_dinode: malloc: %m");
		return NULL;
	}

	pread(dev_fd, dib, LFS_BSIZE, fsbtob(lfsp, (off_t)bi.bi_daddr));
	for (dip = dib; dip != dib + INOPB; ++dip)
		if (dip->i_ino == ino)
			break;
	if (dip == dib + INOPB) {
		free(dib);
		syslog(LOG_WARNING, "dinode %d not found at fsb 0x%x",
		    ino, bi.bi_daddr);
		return NULL;
	}
        dino = *dip; /* structure copy */

        free(dib);
#endif
        return &dino;
}

/*
 * Return the size of the partial segment, in bytes.
 */
int
pseg_size(daddr_t pseg_addr, FS_INFO *fsp, SEGSUM *sp)
{
	int i, ssize = 0;
	struct lfs_super_block *lfsp;
	FINFO *fp;

	lfsp = &fsp->fi_lfs;
	ssize = seg_size();

#if 0
	fp = (FINFO *)(sp + 1);
	for (i = 0; i < sp->ss_nfinfo; ++i) {
		ssize += (fp->fi_nblocks-1) * lfsp->lfs_bsize
			+ fp->fi_lastlength;
		fp = (FINFO *)(&fp->fi_blocks[fp->fi_nblocks]);
	}
#endif
	return ssize;
}

/*
 * This will parse a partial segment and fill in BLOCK_INFO structures
 * for each block described in the segment summary.  It will not include
 * blocks or inodes from files with new version numbers.
 */
void add_blocks(FS_INFO *fsp, BLOCK_INFO *bip, int *countp, 
		struct summary *sump, caddr_t seg_buf, daddr_t seg_addr)
{
	struct ifile_entry *ifp;
	FINFO	*fip;
	SEGSUM *sp;
	struct lfs_super_block *lfsp;
	int i, j;

        if(debug > 1)
            syslog(LOG_DEBUG, "FILE INFOS");

	lfsp = &fsp->fi_lfs;
	bip += *countp;
	sp = &sump->segsum;
	for (i = 0; i < sp->ss_nfinfo; ++i) {
		fip = &sump->fis[i];
		ifp = IFILE_ENTRY(fsp, fip->fi_ino);
		PRINT_FINFO(fip, ifp);
		for (j = 0; j < fip->fi_nblocks; j++) {
			bip->bi_inode = fip->fi_ino;
			bip->bi_lbn = fip->fi_lbn[j];
			bip->bi_daddr = fip->fi_blocks[j] * LFS_BSIZE;
			bip->bi_segcreate = (time_t)(sp->ss_create);
			bip->bi_bp = seg_buf + (bip->bi_daddr - seg_addr);
			bip->bi_version = ifp->ife_version;
			bip->bi_size = LFS_BSIZE; /* read from kernel side ?? */
			syslog(LOG_DEBUG, "ino=%d lbn=%d daddr=0x%x bp=0x%p",
				bip->bi_inode, bip->bi_lbn,
				bip->bi_daddr, bip->bi_bp); 
			if (ifp->ife_version == fip->fi_version) {
				++bip;
				++(*countp);
			}
		}
	}
}

/*
 * For a particular segment summary, reads the inode blocks and adds
 * INODE_INFO structures to the array.  Returns the number of inodes
 * actually added.
 */
void add_inodes(FS_INFO *fsp, BLOCK_INFO *bip, int *countp, 	
		struct summary *sump, caddr_t seg_buf, daddr_t seg_addr)
{
	struct lfs_inode *di = NULL;
	struct lfs_super_block *lfsp;
	struct ifile_entry *ifp;
	BLOCK_INFO *bp;
	SEGSUM *sp;
	int i;

	lfsp = &fsp->fi_lfs;
	sp = &sump->segsum;
	if (sp->ss_nino <= 0)
		return;

	bp = bip + *countp;

        if(debug > 1)
            syslog(LOG_DEBUG, "INODES:");

	for (i = 0; i < sp->ss_nino; ++i) {
		di = (struct lfs_inode *)
		     (seg_buf + (sump->inos[i] * LFS_BSIZE - seg_addr));
		
		//syslog(LOG_DEBUG, "offset for ino %d is = %d\n", di->i_ino,
		//	sump->inos[i] * LFS_BSIZE - seg_addr);

		bp->bi_lbn = LFS_UNUSED_LBN;
		bp->bi_inode = di->i_ino;
		bp->bi_daddr = sump->inos[i] * LFS_BSIZE;
		bp->bi_bp = di;
		bp->bi_segcreate = sp->ss_create;
		bp->bi_size = LFS_BSIZE;

		if (bp->bi_inode == LFS_IFILE_INODE) {
			PRINT_INODE(1, bp);
			bp->bi_version = 1;	/* Ifile version should be 1 */
			bp++;
			++(*countp);
		} else {
			ifp = IFILE_ENTRY(fsp, bp->bi_inode);
			syslog(LOG_DEBUG, "ifp->daddr=%d,sump->inos.daddr=%d\n",
			ifp->ife_daddr, sump->inos[i]);

			PRINT_INODE(ifp->ife_daddr == sump->inos[i], bp);
			bp->bi_version = ifp->ife_version;
			if (ifp->ife_daddr == bp->bi_daddr) {
				bp++;
				++(*countp);
			}
		}
	}
}

/*
 * Checks the summary checksum and the data checksum to determine if the
 * segment is valid or not.  Returns the size of the partial segment if it
 * is valid, and 0 otherwise.  Use dump_summary to figure out size of the
 * the partial as well as whether or not the checksum is valid.
 */
int pseg_valid(FS_INFO *fsp, caddr_t seg_buf, daddr_t addr, 
	       struct summary *sump)
{
	int nblocks;
	struct segsum *ssp;
#if 0
	caddr_t	p;
	int i;
	u_long *datap;
#endif

	ssp = &sump->segsum;

	if (ssp->ss_magic != SS_MAGIC) {
                syslog(LOG_WARNING, "Bad magic number: 0x%x instead of 0x%x",
		ssp->ss_magic, SS_MAGIC);
		return(0);
        }

	nblocks = sump->nblocks;
	if (nblocks <= 0 || nblocks > LFS_MAX_SEG_BLOCKS)
		return(0);

#if 0
	/* check data/inode block(s) checksum too */
	datap = (u_long *)malloc(nblocks * sizeof(u_long));
	p = (caddr_t)ssp + sizeof(SEGSUM);
	for (i = 0; i < nblocks; ++i) {
		datap[i] = *((u_long *)p);
		p += fsp->fi_lfs.lfs_bsize;
	}
	if (cksum ((void *)datap, nblocks * sizeof(u_long)) != ssp->ss_datasum) {
                syslog(LOG_WARNING, "Bad data checksum");
		free(datap);
		return 0;
        }
#endif
	return (nblocks);
}


int bi_compare(const void *a, const void *b)
{
	const BLOCK_INFO *ba, *bb;
	int diff;

	ba = a;
	bb = b;

	if ((diff = (int)(ba->bi_inode - bb->bi_inode)))
		return (diff);
	if ((diff = (int)(ba->bi_lbn - bb->bi_lbn))) {
		if (ba->bi_lbn == LFS_UNUSED_LBN)
			return(-1);
		else if (bb->bi_lbn == LFS_UNUSED_LBN)
			return(1);
		else if (ba->bi_lbn < 0 && bb->bi_lbn >= 0)
			return(1);
		else if (bb->bi_lbn < 0 && ba->bi_lbn >= 0)
			return(-1);
		else
			return (diff);
	}
	if ((diff = (int)(ba->bi_daddr - bb->bi_daddr)))
		return (diff);
	if(ba->bi_inode != LFS_IFILE_INODE && debug)
		syslog(LOG_DEBUG,"bi_compare: using kludge on ino %d!", ba->bi_inode);
	diff = ba->bi_size - bb->bi_size;
	return diff;
}

int
bi_toss(const void *dummy, const void *a, const void *b)
{
	const BLOCK_INFO *ba, *bb;

	ba = a;
	bb = b;

	return(ba->bi_inode == bb->bi_inode && ba->bi_lbn == bb->bi_lbn);
}

void
toss(void *p, int *nump, size_t size, int (*dotoss)(const void *, const void *, const void *), void *client)
{
	int i;
	char *p0, *p1;

	if (*nump == 0)
		return;

	p0 = p;
	for (i = *nump; --i > 0;) {
		p1 = p0 + size;
		if (dotoss(client, p0, p1)) {
			memmove(p0, p1, i * size);
			--(*nump);
		} else
			p0 += size;
	}
}

void
log_exit(int gooderr, int pri, char *fmt, ...)
{
	va_list ap;
	int err;

	err = errno;
	if (err == gooderr)
		pri = LOG_DEBUG;
	va_start(ap, fmt);
	vsyslog(pri, fmt, ap);
	va_end(ap);

	exit(err != gooderr);
}

/* reads summary, fills ssp, and returns number of blocks in the segment */
void read_summary(caddr_t seg_buf, struct summary *sump)
{
	int i;
	caddr_t tempbuf;
	int numblocks;

	tempbuf = seg_buf;
	memcpy(&sump->segsum, tempbuf, SEGSUM_SIZE);
	tempbuf += SEGSUM_SIZE;

	sump->inos = (daddr_t *)malloc(sump->segsum.ss_nino * DADDRT_SIZE);
	for(i = 0;i < sump->segsum.ss_nino; ++i) {
		sump->inos[i] = *(daddr_t *)tempbuf;
		tempbuf += DADDRT_SIZE;
	}
	numblocks = sump->segsum.ss_nino;

	sump->fis = (struct finfo *)malloc(sump->segsum.ss_nfinfo *
					  sizeof(struct finfo));
	for(i = 0;i < sump->segsum.ss_nfinfo; ++i) {
		int size, j;
				
		memcpy(&sump->fis[i], tempbuf, FINFOSIZE);
		tempbuf += FINFOSIZE;
		size = sump->fis[i].fi_nblocks * DADDRT_SIZE;
		sump->fis[i].fi_lbn= (daddr_t *)malloc(size);
		sump->fis[i].fi_blocks = (daddr_t *)malloc(size);
		memcpy(sump->fis[i].fi_lbn, tempbuf, size);
		tempbuf += size;
		memcpy(sump->fis[i].fi_blocks, tempbuf, size);
		tempbuf += size;
		numblocks += sump->fis[i].fi_nblocks;
	}
	sump->nblocks = numblocks;
}

/*
 * This function will scan a segment and return a list of
 * <inode, blocknum> pairs which indicate which blocks were
 * contained as live data within the segment when the segment
 * summary was read (it may have "died" since then).  Any given
 * pair will be listed at most once.
 */
int lfs_segmapv(FS_INFO *fsp, int segnum, caddr_t seg_buf, 
		BLOCK_INFO **blocks, int *bcount)
{
	BLOCK_INFO *bip, *_bip, *nbip;
	SEGSUM *ssp;
	SEGUSE *sup;
	FINFO *fip;
	struct lfs_super_block *lfsp;
	caddr_t s;
	daddr_t seg_addr, pseg_addr;
	int nelem, nblocks, nsegs, sumsize, i, ssize;
	struct summary sum;

	i = 0;
	bip = NULL;
	lfsp = &fsp->fi_lfs;
	nelem = LFS_MAX_SEG_BLOCKS;
	if (!(bip = malloc(nelem * sizeof(BLOCK_INFO)))) {
		syslog(LOG_DEBUG, "couldn't allocate %ld bytes in lfs_segmapv",
			(long)(nelem * sizeof(BLOCK_INFO)));
		goto err0;
	}

	sup = fsp->fi_segusep + segnum;
	pseg_addr = seg_addr = sntod(lfsp, segnum);
        
	if(debug > 1)
            syslog(LOG_DEBUG, "\tsegment buffer at: %p\tseg_addr 0x%llx",
	    seg_buf, (long long)seg_addr);

	read_summary(seg_buf, &sum);
	dump_summary(&sum, 0);

	*bcount = 0;
	sum.segnum = segnum;
	for (nsegs = 0; nsegs < sup->su_nsums; nsegs++) {
		ssp = &sum.segsum;

		nblocks = pseg_valid(fsp, seg_buf, pseg_addr, &sum);
		if (nblocks <= 0) {
                        syslog(LOG_DEBUG, "Warning: invalid segment summary at 0x%llx",
			    (long long)pseg_addr);
			goto err0;
		}
		
		if (*bcount + nblocks + ssp->ss_nino > nelem) {
			nbip = (BLOCK_INFO *)realloc(bip, (*bcount + nblocks + ssp->ss_nino) *
			    sizeof(BLOCK_INFO));
			if (!nbip)
				goto err0;
			bip = nbip;
			nelem = *bcount + nblocks + ssp->ss_nino;
		}
		add_blocks(fsp, bip, bcount, &sum, seg_buf, seg_addr);
		add_inodes(fsp, bip, bcount, &sum, seg_buf, seg_addr);

		ssize = pseg_size(pseg_addr, fsp, ssp);
		/* pseg_addr += */ /* when we have more than 1 su_nsums */
	}
	if(nsegs < sup->su_nsums) {
		syslog(LOG_WARNING,"only %d segment summaries in seg %d (expected %d)",
		       nsegs, segnum, sup->su_nsums);
		goto err0;
	}
	qsort(bip, *bcount, sizeof(BLOCK_INFO), bi_compare);
	toss(bip, bcount, sizeof(BLOCK_INFO), bi_toss, NULL);

        if(debug > 1) {
            syslog(LOG_DEBUG, "BLOCK INFOS");
            for (_bip = bip, i=0; i < *bcount; ++_bip, ++i)
                PRINT_BINFO(_bip);
        }

	*blocks = bip;
	return (0);

    err0:
	if (bip)
		free(bip);
	*bcount = 0;
	return (-1);

}


