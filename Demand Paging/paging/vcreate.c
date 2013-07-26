/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	//kprintf("To be implemented!\n");

	STATWORD ps;
	disable(ps);
	//get a free backing store that will act as a heap
	bsd_t bs_vc = get_free_bs();

	//kprintf("\nGot backing store %d\n",bs_vc);
	//get pid
	int pid = create(procaddr, ssize, priority, name, nargs, args);
	//kprintf("\nGot pid %d\n",pid);

	//map the backing store 
	bs_add_mapping(bs_vc, pid, 4096, hsize);
	bs_tab[bs_vc].as_heap = 1;
	//kprintf("\nBS mappings added\n");

	//update proctab fields
	proctab[pid].store = bs_vc;
	proctab[pid].vhpno = bs_tab[bs_vc].base_page;
	proctab[pid].vhpnpages = hsize;

	//vmblock *mptr = (vmblock);
	//kprintf("\ncheck 1\n");

	//struct mblock *mptr = (struct mblock *)(4096*NBPG);

	//mptr.vaddr = 4096*NBPG;
	//kprintf("\ncheck 2\n");
	//mptr.mnext = NULL;
	//proctab[pid].vmemlist->mnext = (struct mblock *)(4096*NBPG);
	//kprintf("\ncheck 3\n");
	//mptr.vlen = hsize*NBPG;
	//proctab[pid].vmemlist->mlen = hsize*NBPG;
	//kprintf("\ncheck 4\n");
	//proctab[pid].vmemlist->mnext = mptr;
	//proctab[pid].vmemlist->mnext->mnext = 0;

	//vmblock mptr;
	//mptr.vaddr = 4096*NBPG;
	//mptr.vlen = hsize*NBPG;
	//mptr.next = NULL;
	//kprintf("\nin vcreate mptr %lu\n",mptr.vaddr);
	proctab[pid].freememlist->vaddr = 4096*NBPG;
	proctab[pid].freememlist->vlen = hsize*NBPG;
	proctab[pid].freememlist->next = NULL;
	proctab[pid].freememlist->num = 1;
	//kprintf("\nin vreate proctab %d\n",proctab[pid].freememlist->vlen);
	/*proctab[pid].freememlist->vaddr = (unsigned long *)(4096*NBPG);
	proctab[pid].freememlist->vlen = hsize*NBPG;
	proctab[pid].freememlist->next = NULL;*/
	//kprintf("\nWhat the hell\n");
	//proctab[pid]. = ;

	restore(ps);
	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
