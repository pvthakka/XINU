#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

int write_bs(int src_frame, int dst_page) {

  /* write one page of data from src
     to the backing store bs_id, page
     page.
  */
  STATWORD ps;
	disable(ps);
  unsigned long *src_addr = (unsigned long *)(src_frame*NBPG);
  unsigned long *dst_addr = (unsigned long *)(dst_page*NBPG);

	int i = 0;
  for(i = 0; i < NBPG/sizeof(unsigned long); i++)
   {
		*dst_addr = *src_addr;
		dst_addr++;
		src_addr++;

   }
   restore(ps);
}

