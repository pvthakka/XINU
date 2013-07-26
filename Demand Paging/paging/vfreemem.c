/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	WORD *block;
	unsigned size;
{
	STATWORD ps;
	disable(ps);

	if(size <=0 || block == NULL)
	{
		restore(ps);
		return(SYSERR);
	}

	
	size = (unsigned)roundmb(size);

	vmblock *current = (vmblock *)getmem(sizeof(vmblock));
	current = proctab[currpid].freememlist;
	vmblock *prev = (vmblock *)getmem(sizeof(vmblock));
	//kprintf("\nIn freemem word = %lu, freemem = %lu, curr = %lu, pid = %d\n",(unsigned long)block, proctab[currpid].freememlist->vaddr, current->vaddr, currpid);

	prev->vaddr = (unsigned long)block;
	prev->vlen = size;
	prev->num = current->num+1;
	current->num = 0;
	prev->next = current;

	proctab[currpid].freememlist = prev;

	restore(ps);
	return(OK);
}
