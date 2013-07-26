/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  /* sanity check ! */
	STATWORD ps;
	disable(ps);
  if ( (virtpage < 4096) || ( source < 0 ) || ( source > MAX_ID) ||(npages < 1) || ( npages >256) || bs_tab[source].as_heap == 1){
	kprintf("xmmap call error: parameter error! \n");
	//restore(ps);
	return SYSERR;
  }

	if(proctab[currpid].map[source].pid == currpid)
	{
		//current process is already mapped 
		//kprintf("\n\nProcess %s is already mapped to bs %d",proctab[currpid].pname,source);
		return(OK);
	}
	//kprintf("\n\nCalling xmmap for vpage:%x(%d) bs:%d npages:%d",virtpage, virtpage, source, npages);
	bs_add_mapping(source, currpid, virtpage, npages);
	
	restore(ps);
	return(OK);
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage )
{
	STATWORD ps;
	disable(ps);
  /* sanity check ! */
  if ( (virtpage < 4096) ){ 
	kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
	//restore(ps);
	return SYSERR;
  }

	bs_del_mapping(currpid, virtpage);

	//its is possible that the page table of the process has become empty. So we need to delete the page table
	pte_remove(virtpage, currpid);

	/*reload CR3 to clean up the mapping*/
	write_cr3(proctab[currpid].pdbr);

	restore(ps);
	return(OK);

}
