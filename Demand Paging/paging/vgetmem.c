/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{

	STATWORD ps;    

	vmblock *currblock = (vmblock *)getmem(sizeof(vmblock));
	vmblock *prev = (vmblock *)getmem(sizeof(vmblock));

	disable(ps);
	if (nbytes == 0 || proctab[currpid].freememlist->num == 0) {
		restore(ps);
		kprintf("\nNo memory left\n");
		return NULL;
	}
	nbytes = (unsigned int) roundmb(nbytes);
	
	currblock = proctab[currpid].freememlist;
	prev = currblock;
	int i = 0;
	while(i < proctab[currpid].freememlist->num)
	{
		if(currblock->vlen == nbytes)
		{
			if(i == 0)
			{
				//we need to give the first block;
				unsigned long ret = currblock->vaddr;
				if(proctab[currpid].freememlist->num == 1)
				{
					proctab[currpid].freememlist->vaddr = 0;
					proctab[currpid].freememlist->num = 0;
					proctab[currpid].freememlist->vlen = 0;
					//kprintf("\ncase if\n");
				}
				else
				{
					proctab[currpid].freememlist->vaddr = currblock->next->vaddr;
					proctab[currpid].freememlist->num = (currblock->num)-1;
					proctab[currpid].freememlist->vlen = currblock->next->vlen;
					//kprintf("\ncase else\n");
				}
				//kprintf("\nGEtmem: while: num is %d\n",proctab[currpid].freememlist->num);
				return((WORD *)ret);
			}
			else
			{
				//the to be returned block is in the middle

				prev->next = currblock->next;
				proctab[currpid].freememlist->num--;
				proctab[currpid].freememlist->vlen -= nbytes;
				return((WORD *)currblock->vaddr);
			}
				//break;

		}
		else if(currblock->vlen > nbytes)
		{
			//partition the block
			unsigned long ret = currblock->vaddr;

			currblock->vaddr += nbytes;

			//if(proctab[currpid].freememlist->num > 1)
				//kprintf("\nLinked logic in getmem: %lu -> %lu\n", proctab[currpid].freememlist->vaddr, proctab[currpid].freememlist->next->vaddr);
			currblock->vlen -= nbytes;

			//kprintf("\nRemaining size is %d",proctab[currpid].freememlist->vlen);
			return((WORD *)ret);
		}
		prev = currblock;
		currblock = currblock->next;
		i++;
	}

	restore(ps);
	return NULL;
}


