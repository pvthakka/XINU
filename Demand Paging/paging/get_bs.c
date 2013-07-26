#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */

	STATWORD ps; 

	disable(ps);

	if(npages == 0 || npages > 256 || bs_id < 0 || bs_id > MAX_ID || bs_tab[bs_id].as_heap == 1) {
		restore(ps);
		return(SYSERR);
	}

	if(bs_tab[bs_id].status == BSM_MAPPED)
	{
		restore(ps);
		return(bs_tab[bs_id].npages);
	}
	//kprintf("\nCalling get_bs for bs: %d npages: %d", bs_id, npages);
	bs_tab[bs_id].owners = NULL;
	bs_tab[bs_id].status = BSM_MAPPED;
	bs_tab[bs_id].npages = npages;
	bs_tab[bs_id].as_heap = 0;
   
	//kprintf("\n%d pages of %d bs allocated",npages, bs_id);
	return npages;

}


