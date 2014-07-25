#include <syslog.h>
#include <sys/param.h>

#include "cleaner.h"

extern int debug;
extern int seg_size();

unsigned long cksum(void *a, size_t b)
{
	printf("cksum called\n");
	return 0;
}

/*
 * Print out a summary block; return number of blocks in segment; 0
 * for empty segment or corrupt segment.
 * Returns a pointer to the array of inode addresses.
 */

void dump_summary(struct summary *sump, unsigned long flags)
{
	int i;
	struct segsum *ssp;
	struct finfo *fip;

	ssp = &sump->segsum;
	if (flags & DUMP_SUM_HEADER) {
		syslog(LOG_DEBUG, "nfi = %d, nino = %d, create = %s", 
			ssp->ss_nfinfo, ssp->ss_nino, 
			ctime((time_t *)&ssp->ss_create));
	}

	if (flags & DUMP_INODE_ADDRS) {
		syslog(LOG_DEBUG, "Inode blocks: ");
		for(i = 0;i < ssp->ss_nino; ++i)
			syslog(LOG_DEBUG, "%d ", sump->inos[i]);
		syslog(LOG_DEBUG, "\n");
	}

	for(i = 0;i < ssp->ss_nfinfo; ++i) {
		int j;
		if (flags & DUMP_FINFOS)
			syslog(LOG_DEBUG, "nblocks=%d,ino=%d,", 
				sump->fis[i].fi_nblocks, sump->fis[i].fi_ino);
		if(flags & DUMP_FINFOS) {
			syslog(LOG_DEBUG, "fi_blocks[%d] = ", i);
			for(j = 0;j < sump->fis[i].fi_nblocks; ++j)
				syslog(LOG_DEBUG, "%d ",sump->fis[i].fi_blocks[j]);
			syslog(LOG_DEBUG, "\n");
		}
	}
}

void dump_cleaner_info(CLEANERINFO *cip)
{
        if(debug <= 1)
            return;

	syslog(LOG_DEBUG,"segments clean\t%d\tsegments dirty\t%d\n\n",
	    cip->clean, cip->dirty);
}

void dump_super(struct lfs_super_block *lfsp)
{
	int i;

        if(debug < 2)
            return;

	syslog(LOG_DEBUG,"%s0x%X\t%s0x%X\n",
		"magic    ", lfsp->s_magic,
		"version  ", lfsp->s_version);
	
	syslog(LOG_DEBUG, "%s%d\n",
		"nseg     ", lfsp->s_nseg);

}

void dump_segusetbl(FSINFO *fsp)
{	int i;

	if(debug <= 1)
		return;
	for(i = 0;i < fsp->nseguse; ++i) {
		struct segusage *ps;
		
		ps = fsp->fi_segusep + i;
		printf("segment = %d, live bytes = %d \n", i + 1, ps->su_nbytes);
	}
}

void dump_ifile(FSINFO *fsp)
{	int i;

	if(debug <= 1)
		return;
	for(i = 0;i < fsp->fi_ifile.n_ife; ++i) {
		struct ifile_entry *pife;
		
		pife = fsp->fi_ifile.ifes + i;
		printf("%d inode daddr %d\n", i + 1, pife->ife_daddr);
	}
}
