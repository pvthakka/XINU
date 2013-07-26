/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
		STATWORD ps;
	disable(ps);
  unsigned long int eip = read_cr2();

	//kprintf("\n\n#PF in %s, cr2:%x",proctab[currpid].pname, eip);
  unsigned long int pd_offset = eip >> 22;
  unsigned long int pt_offset = eip >>12;
  pt_offset = pt_offset & 1023;
  unsigned long offset = eip;
  offset = offset & 4095;

	//kprintf("\nEIP read is %lu\n",eip);
  eip = eip >> 12; //vpno
  //kprintf("\nEIP shifted is %lu\n",eip);

  int i = 0, flag = 0;
  int backing_store_page, bs_id;

  for(i = 0 ; i < NBS; i++)
  {
		if(proctab[currpid].map[i].pid == currpid)
		{
			//kprintf("\nif\n");
			if(proctab[currpid].map[i].next == NULL)
			{
				//kprintf("\nif if\n");
				if(proctab[currpid].map[i].vpno == eip) //we found the exact page
				{
					backing_store_page = proctab[currpid].map[i].base_page;
					flag = 1;
					//kprintf("\nif if if\n");
				}
				else if(proctab[currpid].map[i].vpno < eip && (proctab[currpid].map[i].vpno+proctab[currpid].map[i].npages) >= eip) //we found the page in the range of base_page - npages
				{
					backing_store_page = proctab[currpid].map[i].base_page + eip - proctab[currpid].map[i].vpno;
					flag = 1;
					//kprintf("\nif if else if\n");
				}
			}
			else
			{
				//kprintf("\nelse\n");
				bs_map_t *jump = &(proctab[currpid].map[i]);
				while(jump != NULL)
				{
					//kprintf("\nwhile\n");
					if(jump->vpno == eip) //we found the exact page
					{
						backing_store_page = jump->base_page;
						flag = 1;
						//kprintf("\nif else if\n");
						break;
					}
					else if(jump->vpno < eip && (jump->npages+jump->vpno) >= eip) //we found the page in the range of base_page - npages
					{
						backing_store_page = jump->base_page + eip - jump->vpno;
						flag = 1;
						//kprintf("\nif else else if\n");
						break;
					}
					jump = jump->next;
				}
			}
		}
		if(flag)
		{
			bs_id = i;
			break;
		}

  }
  //kprintf("\nin pfint bs %d, bs_page %d",bs_id,backing_store_page);

  unsigned long *bs_addr = (unsigned long *)(backing_store_page*NBPG);

	//populate page table
	//checking if page dir is empty
	
	pd_t *ptr1 = (pd_t *)(proctab[currpid].pdbr);
	//kprintf("\nbefore %d, %lu\n",ptr1,pd_offset);
	ptr1 += pd_offset; 
	pt_t *ptr;
	if(ptr1->pd_pres == 1) //page table exists hence add entry to that
	{
		ptr = (pt_t *)((ptr1->pd_base)*NBPG);
		frm_tab[(ptr1->pd_base)-FRAME0].refcnt++;
		frm_map[(ptr1->pd_base)-FRAME0].fr_refcnt++;
	}
	else //we need to create a page table, add our free_frm entry to it and add the page table entry to the directory
	{
		//kprintf("\nin else %d\n",ptr1);

		int pt_frame = get_frm();
		frm_tab[pt_frame-FRAME0].status = FRM_PGT;
		frm_tab[pt_frame-FRAME0].refcnt++;

		frm_map[pt_frame-FRAME0].fr_status = FRM_MAPPED;
		frm_map[pt_frame-FRAME0].fr_pid = currpid;
		frm_map[pt_frame-FRAME0].fr_refcnt++;
		frm_map[pt_frame-FRAME0].fr_type = FR_TBL;

		ptr = (pt_t*)(pt_frame*NBPG);

		//add the above table to page directory
		ptr1->pd_pres = 1;
		ptr1->pd_write = 1;
		ptr1->pd_user = 0;
		ptr1->pd_pwt = 0;
		ptr1->pd_pcd = 0;
		ptr1->pd_acc = 0;
		ptr1->pd_mbz = 0;
		ptr1->pd_fmb = 0;
		ptr1->pd_global = 0;
		ptr1->pd_avail = 0;
		ptr1->pd_base = pt_frame;

		//kprintf("\nget_frm return frame %d. To be page table for process %s",pt_frame, proctab[currpid].pname);

	}
	//add entry to page table
	ptr += pt_offset;
	ptr->pt_pres = 1;
	ptr->pt_write = 1;
	ptr->pt_user = 0;
	ptr->pt_pwt = 0;
	ptr->pt_pcd = 0;
	ptr->pt_acc = 0;
	ptr->pt_dirty = 0;
	ptr->pt_mbz = 0;
	ptr->pt_global = 0;
	ptr->pt_avail = 0;

	  //getting a free frame an setting the frame mappings
  int free_frm;
  if(ni_page_table[backing_store_page-total_bs_left] != -1) // if backing store is already mapped then we share the frame
	{
		free_frm = ni_page_table[backing_store_page-total_bs_left];
		//kprintf("\nIN NI frm = %d, ni = %d\n",free_frm, ni_page_table[backing_store_page-total_bs_left]);
		frm_map[free_frm-FRAME0].fr_refcnt++;
		frm_tab[free_frm-FRAME0].refcnt++;

		//adding this vpno and pid to the shared frame map list
		fr_map_t *map = (fr_map_t *)getmem(sizeof(fr_map_t));
		map->fr_pid = currpid;
		map->fr_vpno = eip;
		map->shared = NULL;

		fr_map_t *next = (fr_map_t *)getmem(sizeof(fr_map_t));
		next = &(frm_map[free_frm-FRAME0]);

		while(next->shared != NULL)
				next = next->shared;

		next->shared = map;

		//printing the list
		/*kprintf("\nSHARED VPNO: ");
		next = &(frm_map[free_frm-FRAME0]);
		while(next != NULL)
		{
			kprintf("%d -> ",next->fr_vpno);
			next = next->shared;
		}*/
	}
	else
	{
		free_frm = get_frm();
		
		frm_tab[free_frm-FRAME0].status = FRM_BS;
		frm_tab[free_frm-FRAME0].refcnt++;
		frm_tab[free_frm-FRAME0].bs = bs_id;
		frm_tab[free_frm-FRAME0].bs_page = backing_store_page;
		frm_tab[free_frm-FRAME0].bs_next = NULL;
		frm_tab[free_frm-FRAME0].fifo = NULL;
		frm_tab[free_frm-FRAME0].age = 128;
 		frm_map[free_frm-FRAME0].fr_status = FRM_MAPPED;
		frm_map[free_frm-FRAME0].fr_pid = currpid;
		frm_map[free_frm-FRAME0].fr_vpno = eip;
		frm_map[free_frm-FRAME0].fr_refcnt++;
		frm_map[free_frm-FRAME0].fr_type = FR_PAGE;
		frm_map[free_frm-FRAME0].bs_page_num = backing_store_page;

		ni_page_table[backing_store_page-total_bs_left] = free_frm;
		
		//set bs mappings
		if(bs_tab[bs_id].frm == NULL)
				  bs_tab[bs_id].frm = &frm_tab[free_frm-FRAME0];
		else
		{
			//frame_t *jump = &frm_tab[free_frm-FRAME0];
			frame_t *jump = (frame_t *)getmem(sizeof(frame_t)); 
			jump = bs_tab[bs_id].frm;
			while(jump->bs_next != NULL)
			{
				jump = jump->bs_next;
				//kprintf("\njumping\n");
			}

			jump->bs_next = &frm_tab[free_frm-FRAME0];
		}

		//adding this frame to the fifo queue
		if(fifo_head == NULL)
		{
			//queue is empty
			fifo_head = &frm_tab[free_frm-FRAME0];
			fifo_tail = fifo_head;
		}
		else
		{
			fifo_tail->fifo = &frm_tab[free_frm-FRAME0];
			fifo_tail = &frm_tab[free_frm-FRAME0];
		}
	}
	//kprintf("\nget_frm return frame %d.", free_frm);
	
  unsigned long *dst_addr = (unsigned long *)(free_frm*NBPG);

  //kprintf("\n\n Virtual page %d mapped to bs page %d, bs id %d, mapped to frame %d, %lu",eip, backing_store_page, bs_id, free_frm,dst_addr);

	/*frame_t *next = fifo_head;
	kprintf("\nFIFO : ");
	while(next != NULL)
	{
		kprintf("%d-> ",next->frame_num);
		next = next->fifo;
	}
	kprintf("\n");*/

	
   //copy page from bs to phy
   for(i = 0; i < NBPG/sizeof(unsigned long); i++)
   {
		*dst_addr = *bs_addr;
		dst_addr++;
		bs_addr++;

   }
   
	ptr->pt_base = free_frm;

	restore(ps);

	//kprintf("\nmap bs%d/page: %d to frame %d",bs_id, (backing_store_page-proctab[currpid].map[bs_id].base_page), free_frm);
  return OK;
}


