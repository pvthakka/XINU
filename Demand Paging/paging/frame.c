/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_map[NFRAMES];
frame_t frm_tab[NFRAMES];
frame_t *fifo_head, *fifo_tail;
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
	STATWORD ps;
	disable(ps);
  int i = 0;
  for(i = 0; i < NFRAMES; i++) 
  {
	frm_tab[i].status = FRM_FREE;
	frm_tab[i].refcnt = 0;
	frm_tab[i].bs = -1;
	frm_tab[i].bs_page = -1;
	frm_tab[i].bs_next = NULL;
	frm_tab[i].fifo = NULL;
	frm_tab[i].age = 0;
	frm_tab[i].frame_num = i+FRAME0;
	
	frm_map[i].fr_status = FRM_UNMAPPED;
	frm_map[i].fr_pid = -1;
	frm_map[i].fr_vpno = -1;
	frm_map[i].fr_refcnt = 0;
	frm_map[i].fr_type = FR_PAGE;
	frm_map[i].fr_dirty = -1;
	frm_map[i].shared = NULL;
	frm_map[i].bs_page_num = -1;
	frm_map[i].frame_num = i+FRAME0;

  }

	fifo_head = NULL;
	fifo_tail = NULL;
	restore(ps);
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
/*SYSCALL get_frm(int* avail)
{
  kprintf("To be implemented!\n");
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int frame)
{
	//STATWORD ps;
	//disable(ps);
  if(frame < 4 || frame > NFRAMES || frm_tab[frame].status == FRM_PGD || frm_tab[frame].status == FRM_PGT || frm_tab[frame].status == FRM_FREE)
	{
		//restore(ps);
		return(SYSERR);
	}

	//write frame to backing store page
	write_bs(frm_tab[frame].frame_num, frm_tab[frame].bs_page);

	//remove the entry from the NI page table
	ni_page_table[frm_tab[frame].bs_page-total_bs_left] = -1;

    frm_tab[frame].status = FRM_FREE;
    frm_tab[frame].refcnt = 0;
	frm_tab[frame].bs = -1;
	frm_tab[frame].bs_page = -1;
	//frm_tab[frame].bs_next = NULL;
	frm_tab[frame].age = 0;

	//remove this frame from fifo list
	frame_t *next, *prev;
	next = fifo_head;
	if(next == &frm_tab[frame])
	{
		fifo_head = fifo_head->fifo;
		//kprintf("\nin if fifo\n");
	}
	else
	{
		//kprintf("\nin else fifo\n");
		while(next->fifo != NULL)
		{
			if(next == &frm_tab[frame])
			{
				prev->fifo = next->fifo;
			}
			prev = next;
			next = next->fifo;
		}
			
	}
	frm_tab[frame].fifo = NULL;
	
	//remove reference from page table. Now here we can have multiple vpnos. Hence need to loop
	int flag = 0;
	do
	{
		unsigned long int pd_offset = frm_map[frame].fr_vpno >>10;
		unsigned long int pt_offset = frm_map[frame].fr_vpno;
		//kprintf("\nIn unmap. Vpno = %d, pd_offset = %lu, fr_pid =%d, currpid = %d\n",frm_map[(frame->frame_num)-FRAME0].fr_vpno, pd_offset, frm_map[(frame->frame_num)-FRAME0].fr_pid, pid);
		pt_offset = pt_offset & 1023;

		pd_t *ptr1 = (pd_t *)(proctab[frm_map[frame].fr_pid].pdbr);
		ptr1 += pd_offset; 

		unsigned int pg_table = ptr1->pd_base;
					
		pt_t *ptr = (pt_t *)(pg_table*NBPG);
		ptr += pt_offset;

		ptr->pt_pres = 0;
		ptr->pt_write = 0;
		ptr->pt_user = 0;
		ptr->pt_pwt = 0;
		ptr->pt_pcd = 0;
		ptr->pt_acc = 0;
		ptr->pt_dirty = 0;
		ptr->pt_mbz = 0;
		ptr->pt_global = 0;
		ptr->pt_avail = 0;
		ptr->pt_base = 0;

		//decrement page table ref count
		frm_map[pg_table-FRAME0].fr_refcnt--;
		frm_tab[pg_table-FRAME0].refcnt--;
		//kprintf("\n%d page table ref count at %d\n",pg_table,frm_tab[pg_table-FRAME0].refcnt);

		frm_map[frame].fr_status = FRM_UNMAPPED;
		//frm_map[frame].fr_pid = -1;
		//frm_map[frame].fr_vpno = -1;
		frm_map[frame].fr_refcnt = 0;
		frm_map[frame].fr_type = FR_PAGE;
		frm_map[frame].fr_dirty = -1;
		//frm_map[frame].shared = NULL;
		frm_map[frame].bs_page_num = -1;

		if(frm_map[frame].shared == NULL)
		{
			frm_map[frame].fr_pid = -1;
			frm_map[frame].fr_vpno = -1;
			flag = 1;
		}
		else
		{
			frm_map[frame].fr_pid = frm_map[frame].shared->fr_pid;
			frm_map[frame].fr_vpno = frm_map[frame].shared->fr_vpno;
			frm_map[frame].shared = frm_map[frame].shared->shared;
		}
	} while(!flag);

	frm_map[frame].shared = NULL;
	//restore(ps);
	return OK;
}


int get_frm()
{
	int i = 0;

	for(i = 0; i < NFRAMES; i++)
	{
		if(frm_tab[i].status == FRM_FREE)
		{
			//kprintf("\nReturning from get_frm %d",i+FRAME0);
			return (i+FRAME0);
		}
	}

	//this means there is no free frame. Call page replacement algorithm

	//kprintf("\n\nKicking in replacement policy");

	return(page_replace());

	//return(SYSERR);

}

int page_replace()
{
	//STATWORD ps;
	//disable(ps);
	int tlb_flush = 0;
	if(page_replace_policy == FIFO)
	{
		//remove the first frame from the head of the FIFO queue
		frame_t *replace = (frame_t *)getmem(sizeof(frame_t)); 
		replace = fifo_head;
		
		//kprintf("\nHEAD : %d\n",replace->frame_num);
		//move head
		if(fifo_head != NULL)
		{
			fifo_head = fifo_head->fifo;

			//remove the frame from backing store frm list
			bs_remove_frame(frm_tab[(replace->frame_num)-FRAME0].bs, replace->frame_num);

			//if the replacement frame belongs to the current process, then we need to flush thr TLB. Hence set the flag.
			if(frm_map[(replace->frame_num)-FRAME0].fr_pid == currpid)
				tlb_flush = 1;

			//free the frame
			free_frm((replace->frame_num)-FRAME0);

			//if the TLB needs to be flushed
			if(tlb_flush)
				write_cr3(proctab[currpid].pdbr);

			//restore(ps);
			kprintf("\nReplacing frame %d using FIFO",replace->frame_num);
			return(replace->frame_num);
		}
		else
		{
			//restore(ps);
			return(SYSERR);
		}
	}
	else if(page_replace_policy == AGING)
	{
		frame_t *aging = fifo_head;
		int min_age = -1;
		int frame_num = SYSERR;
		//kprintf("\nFIFO head is %d\n",aging->frame_num);
		//pt_t *ptr;
		while(aging != NULL)
		{
			//reduce age by half
			//kprintf("\nSeeing frame %d, age %d",aging->frame_num, aging->age);
			aging->age = aging->age >> 1;
		
			//if page has been accessed, add 128 to its age and reset the access bit
			//this frame might me shared, hence we need to check access bits in each page table
			int flag = 0, shared_flag = 0;
			fr_map_t *next = &(frm_map[(aging->frame_num)-FRAME0]);
			int fr_vpno = next->fr_vpno;
			int fr_pid = next->fr_pid;
			do
			{
				//unsigned long int pd_offset = frm_map[(aging->frame_num)-FRAME0].fr_vpno >>10;
				//unsigned long int pt_offset = frm_map[(aging->frame_num)-FRAME0].fr_vpno;
				unsigned long int pd_offset = fr_vpno >>10;
				unsigned long int pt_offset = fr_vpno;
				pt_offset = pt_offset & 1023;

				//get the page table frame
				pd_t *ptr1 = (pd_t *)(proctab[fr_pid].pdbr);
				ptr1 += pd_offset; 

				unsigned int pg_table = ptr1->pd_base;
					
				pt_t *ptr = (pt_t *)(pg_table*NBPG);
				ptr += pt_offset;

				//now read the access bit
				if(ptr->pt_acc)
				{
					if(!shared_flag)
						aging->age |= 128;
					
					aging->age = aging->age & 255;
					
					ptr->pt_acc = 0;
					//break;
					shared_flag = 1;
					//kprintf("\nFrame %d has been accessed. New age %d",aging->frame_num, aging->age);
				}
				if(next->shared == NULL)
					flag = 1;
				else
				{
					next = next->shared;
					fr_vpno = next->fr_vpno;
					fr_pid = next->fr_pid;
					//kprintf("\nIN SHARED loop. vp = %d, pid = %d",fr_vpno, fr_pid);
				}
			} while(!flag);

			//if age is less then the minimum age seen till now, sotre that frame
			if(aging->age < min_age)
			{
				min_age = aging->age;
				frame_num = aging->frame_num;
				//kprintf("\nFound min age %d, frame %d\n",min_age, frame_num);
			}

			aging = aging->fifo;
			//kprintf("\njumping %d", aging->frame_num);
		}

		//remove the frame from backing store frm list
		bs_remove_frame(frm_tab[frame_num-FRAME0].bs, frame_num);

		//if the replacement frame belongs to the current process, then we need to flush thr TLB. Hence set the flag.
		if(frm_map[frame_num-FRAME0].fr_pid == currpid)
			tlb_flush = 1;

		//free the frame
		free_frm(frame_num-FRAME0);

		//if the TLB needs to be flushed
		if(tlb_flush)
			write_cr3(proctab[currpid].pdbr);

		//restore(ps);
		kprintf("\nReplacing frame %d using AGING",frame_num);
		return(frame_num);
	}
	//restore(ps);
	return(SYSERR);
}

void pte_remove(int vpno, int pid)
{
	unsigned long int pd_offset = vpno >>10;
	unsigned long int pt_offset = vpno;
	pt_offset = pt_offset & 1023;

	//get the page table frame
	pd_t *ptr1 = (pd_t *)(proctab[pid].pdbr);
	ptr1 += pd_offset; 

	unsigned int pg_table = ptr1->pd_base;
					
	//checking page table ref cnt
	if(frm_tab[pg_table-FRAME0].refcnt <= 0 && pg_table > FRAME0+5)
	{
		kprintf("\nFreeing page table number %d, vpno %d\n",pg_table,vpno);
		int i = pg_table-FRAME0;
		frm_tab[i].status = FRM_FREE;
		frm_tab[i].refcnt = 0;
		frm_tab[i].bs = -1;
		frm_tab[i].bs_page = -1;
		frm_tab[i].bs_next = NULL;
		frm_tab[i].fifo = NULL;
		frm_tab[i].age = 0;
			
		frm_map[i].fr_status = FRM_UNMAPPED;
		frm_map[i].fr_pid = -1;
		frm_map[i].fr_vpno = -1;
		frm_map[i].fr_refcnt = 0;
		frm_map[i].fr_type = FR_PAGE;
		frm_map[i].fr_dirty = -1;
		frm_map[i].shared = NULL;
		frm_map[i].bs_page_num = -1;
	}

	//removing from page directory
	ptr1->pd_pres = 0;
	ptr1->pd_write = 0;
	ptr1->pd_user = 0;
	ptr1->pd_pwt = 0;
	ptr1->pd_pcd = 0;
	ptr1->pd_acc = 0;
	ptr1->pd_mbz = 0;
	ptr1->pd_fmb = 0;
	ptr1->pd_global = 0;
	ptr1->pd_avail = 0;
	ptr1->pd_base = 0;
	
}
