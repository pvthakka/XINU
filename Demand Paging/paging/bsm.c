/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bs_map[NBS];
bs_t bs_tab[NBS];
int total_bs_left = ((unsigned long) BACKING_STORE_UNIT_SIZE)*NBS/NBPG;
int ni_page_table[((unsigned long) BACKING_STORE_UNIT_SIZE)*NBS/NBPG];
int bs_size = ((unsigned long) BACKING_STORE_UNIT_SIZE)/NBPG;

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	STATWORD ps;
	disable(ps);
	int i = 0;
	for(i = 0;i < NBS; i++)
	{
		bs_tab[i].status = BSM_UNMAPPED;
		bs_tab[i].as_heap = -1;
		bs_tab[i].npages = -1;
		bs_tab[i].owners = NULL;
		bs_tab[i].frm = NULL;
		bs_tab[i].base_page = BS_PAGES+(i*bs_size);

		//kprintf("\ntotal bs %d\n",total_bs_left);
	}
	
	for(i = 0;i < total_bs_left; i++)
		 ni_page_table[i] = -1;

	restore(ps);
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	STATWORD ps;
	disable(ps);
	int i = 0;
	for(i = 0 ; i < NBS; i++)
		if(bs_tab[i].status == BSM_UNMAPPED)
		{
			//restore(ps);
			return i;
		}
	restore(ps);
	return(SYSERR);
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD ps;
	disable(ps);
	bs_tab[i].status = BSM_UNMAPPED;
	bs_tab[i].npages = 0;
	bs_tab[i].frm = NULL;
	bs_tab[i].as_heap = 0;
	bs_tab[i].owners->next = NULL;
	bs_tab[i].owners->bs = -1;
	bs_tab[i].owners->pid = -1;
	bs_tab[i].owners->vpno = -1;
	bs_tab[i].owners->npages = 0;

	restore(ps);
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
}

/*initialize the back store*/
int init_bs()
{
	//done already in init_bsm
	init_bsm();
}

/*allocate a backstore */
bs_t* alloc_bs(bsd_t id, int npages)
{
	//done in get_bs

}

/*allocate a free backstore*/
bsd_t get_free_bs()
{
	STATWORD ps;
	disable(ps);
	int i = 0;
	for(i = 0; i < NBS; i++)
	{
		if(bs_tab[i].status == BSM_UNMAPPED)
		{
			//restore(ps);
			return i;
		}
	}
	restore(ps);
	return(SYSERR);
}

void free_bs(bsd_t id)
{
	STATWORD ps;
	disable(ps);
	bs_tab[id].status = BSM_UNMAPPED;
	bs_tab[id].npages = 0;
	bs_tab[id].frm = NULL;
	bs_tab[id].as_heap = 0;
	bs_tab[id].owners->next = NULL;
	bs_tab[id].owners->bs = -1;
	bs_tab[id].owners->pid = -1;
	bs_tab[id].owners->vpno = -1;
	bs_tab[id].owners->npages = 0;

	restore(ps);

}

/*validate bs mapping */
int validate_bs_mapping()
{

}

/*add a mapping of the bs to (pid, vpno)*/
void bs_add_mapping(bsd_t id, int pid, int vpno, int npages)
{
	get_bs(id,npages);
	//bs_tab[id].as_heap = 1;
	//kprintf("\nMap bs%d into process %d:%d",id, pid, vpno);
	proctab[pid].map[id].vpno = vpno;
	proctab[pid].map[id].npages = npages;
	proctab[pid].map[id].bs = id;
	proctab[pid].map[id].pid = pid;
	proctab[pid].map[id].base_page = BS_PAGES+(id*bs_size);
	proctab[pid].map[id].next = NULL;

	//kprintf("\nAdd mapping bs%d to process %d map:%d",proctab[pid].map[id].bs,proctab[pid].map[id].pid,proctab[pid].map[id].vpno);

	if(bs_tab[id].owners == NULL)
	{
		bs_tab[id].owners = &(proctab[pid].map[id]);
		//kprintf("\nAdded mapping bs-%d to bs linked list. process %d map:%d",bs_tab[id].owners->bs, bs_tab[id].owners->pid, bs_tab[id].owners->vpno);
	 }
	else
	{
		bs_map_t *jump = (bs_map_t *)getmem(sizeof(bs_map_t));
		jump = bs_tab[id].owners;
		while(jump->next != NULL)
			jump = jump->next;

		jump->next = &(proctab[pid].map[id]);
		//kprintf("\nAdded mapping bs-%d to bs linked list. process %d map:%d",jump->next->bs, jump->next->pid, jump->next->vpno);
		//kprintf("\ninside oweners else \n");
	}
}


/*remove a mapping from both the backstore and process. the unmap function
 will take care of the mapped virtual pages*/
int bs_del_mapping(int pid, int vpno)
{

	//if the given vpno is not the base vpno, get the base vpno
	vpno = bs_get_vpno(pid,vpno);

	//find out the backing store that contains this vpno fromt the map in proctab
	int i = 0;
	for(i = 0; i < NBS; i++)
	{
		if(proctab[pid].map[i].pid == pid && proctab[pid].map[i].vpno == vpno)
		{
			//First of all we need to remove the current BS map from the BS map linked list
			bs_map_t *next = (bs_map_t *)getmem(sizeof(bs_map_t));
			next = bs_tab[i].owners;
			bs_map_t *prev = (bs_map_t *)getmem(sizeof(bs_map_t));
		
			if(next == &(proctab[pid].map[i]))
			{
				//there is only one mapping hence set the owners as null
				//kprintf("\n\nUnmap process %d:%x",pid,proctab[pid].map[i].vpno);
				bs_tab[i].owners = NULL;
			}
			else
			{
				while(next != NULL)
				{
					if(next == &(proctab[pid].map[i]))
					{
						//kprintf("\n\nUnmap process %d:%x",pid,proctab[pid].map[i].vpno);
						prev->next = next->next;
						break;
					}
					prev = next;
					next = next->next;
				}
			}
			//Now resetting the map entries in proctab
			proctab[pid].map[i].next = NULL;
			proctab[pid].map[i].bs = -1;
			proctab[pid].map[i].pid = -1;
			proctab[pid].map[i].vpno = -1;
			proctab[pid].map[i].npages = 0;
			//kprintf("\ndelete pid %d's mapping of bs%d@%x",pid, i, vpno);

			//Now we traverse the frame linked list in bs_tab and keep returning those frames
			frame_t *nextf = (frame_t *)getmem(sizeof(frame_t));
			nextf = bs_tab[i].frm;
			frame_t *fprev = (frame_t *)getmem(sizeof(frame_t));

			//frame_t *nextf = bs_tab[i].frm;
			/*printf("\nFRAME : ");
			while(nextf != NULL)
			{
				kprintf("%d(%d)-> ",nextf->frame_num, frm_map[nextf->frame_num-FRAME0].fr_pid);
				nextf = nextf->bs_next;
			}
			kprintf("\n");
			nextf = bs_tab[i].frm;*/
			fprev = nextf;
			while(nextf != NULL)
			{
				if(frm_map[(nextf->frame_num)-FRAME0].shared == NULL)
				{
					//this frame is not shared. Hence we can free it
					if(frm_map[(nextf->frame_num)-FRAME0].fr_pid == pid)
					{
						//write frame to backing store
						//write_bs(nextf->frame_num, nextf->bs_page);

						//kprintf("\nRemoving frame %d from bs %d",nextf->frame_num, i);
						//free the frame
						//kprintf("\nIn unmap. bs_next = %d, fr_pid =%d, currpid = %d, frame_num = %d\n",nextf->bs_next, frm_map[(nextf->frame_num)-FRAME0].fr_pid, pid, nextf->frame_num);
						free_frm((nextf->frame_num)-FRAME0);
					}
				}
				else
				{
					//this frame is shared. Hence we need to removed the shared entry
					//kprintf("\nInside shared loop first vpno = %d, arg vpno = %d",frm_map[(nextf->frame_num)-FRAME0].fr_vpno, vpno);
					//kprintf("\nFirst pid = %d, arg pid = %d",frm_map[(nextf->frame_num)-FRAME0].fr_pid, pid);
					frm_map[(nextf->frame_num)-FRAME0].fr_refcnt--;
					frm_tab[(nextf->frame_num)-FRAME0].refcnt--;
					if(frm_tab[(nextf->frame_num)-FRAME0].refcnt <= 0)
						free_frm((nextf->frame_num)-FRAME0);
					else
					{
						if(frm_map[(nextf->frame_num)-FRAME0].fr_pid == pid)
						{
							frm_map[(nextf->frame_num)-FRAME0].fr_pid = frm_map[(nextf->frame_num)-FRAME0].shared->fr_pid;
							frm_map[(nextf->frame_num)-FRAME0].fr_vpno = frm_map[(nextf->frame_num)-FRAME0].shared->fr_vpno;
							frm_map[(nextf->frame_num)-FRAME0].shared = frm_map[(nextf->frame_num)-FRAME0].shared->shared;
							//kprintf("\nNEW PROCESS = %s, VPNO = %d",proctab[frm_map[(nextf->frame_num)-FRAME0].fr_pid].pname,frm_map[(nextf->frame_num)-FRAME0].fr_vpno );
						}
						else
						{
							fr_map_t *nextmap = (fr_map_t *)getmem(sizeof(fr_map_t));
							fr_map_t *prevmap = (fr_map_t *)getmem(sizeof(fr_map_t));
							nextmap = &(frm_map[(nextf->frame_num)-FRAME0]);
							prevmap = nextmap;
							while(nextmap != NULL)
							{
								if(nextmap->fr_pid == pid)
									prevmap->shared = nextmap->shared;
								
								prevmap = nextmap;
								nextmap = nextmap->shared;
							}
						}
					}
					/*kprintf("\nDELETED VPNO: ");
					fr_map_t *nextmap = &(frm_map[(nextf->frame_num)-FRAME0]);
					while(nextmap != NULL)
					{
						kprintf("%d -> ",nextmap->fr_vpno);
						nextmap = nextmap->shared;
					}*/
					
				}

				fprev = nextf;
				nextf = nextf->bs_next;
				fprev->bs_next = NULL;
			}

		}
	}
}


/*given a pid and virtual addr, return the backstore id and the page number 
 mapped to this address*/
//int bs_lookup_mapping(int pid, long vpno, bsd_t *id, int* pageth)
int bs_lookup_mapping(int pid, long vpno)
{
	int i = 0;
	for(i = 0; i < NBS; i++)
		if(proctab[pid].map[i].pid == pid && proctab[pid].map[i].vpno == vpno)
			return i;

	return(SYSERR);
		
}


/*return the frame that mapping the pagth page of backstore*/
frame_t *bs_get_frame(bsd_t id, int pagth)
{

}

void bs_remove_frame(int bs_id, int frame_num)
{
	frame_t *next = (frame_t *)getmem(sizeof(frame_t));
	next = bs_tab[bs_id].frm;
	frame_t *prev = (frame_t *)getmem(sizeof(frame_t));

	prev = next;
		
	if(next->frame_num == frame_num)
		bs_tab[bs_id].frm = bs_tab[bs_id].frm->bs_next;
	else
	{
		while(next != NULL)
		{
			if(next->frame_num == frame_num)
			{
				prev->bs_next = next->bs_next;
			}
			prev = next;
			next = next->bs_next;
		}
	}
	
}


/*release a process's mapping of the backstore pagth page*/
void bs_put_frame(bsd_t id, int pagth)
{

}


/*purge a frame from the backstore */
void bs_purge_frame(frame_t *frm)
{
	//bs = &bs_tab[frm->bs];
	//map = bs->owners;

	/*rampage through the each process's page table to remove mappings  <-- Why owners is needed!!!*/
	/*while (map != NULL) { 
         vpno = map->vpno + frm->bs_page;
		...
	}*/
}

int bs_get_vpno(int pid, int vpno)
{
	int i = 0, flag = 0;
	int ret_vpno;
	for(i = 0 ; i < NBS; i++)
	{
		if(proctab[pid].map[i].pid == pid)
		{
			//kprintf("\nif\n");
			if(proctab[pid].map[i].next == NULL)
			{
				//kprintf("\nif if\n");
				if(proctab[pid].map[i].vpno == vpno) //we found the exact vpno
				{
					ret_vpno = proctab[pid].map[i].vpno;
					flag = 1;
					//kprintf("\nif if if\n");
				}
				else if(proctab[pid].map[i].vpno < vpno && (proctab[pid].map[i].vpno+proctab[pid].map[i].npages) >= vpno) //we found the page in the range of base_page - npages
				{
					ret_vpno = proctab[pid].map[i].vpno;
					flag = 1;
					//kprintf("\nif if else if\n");
				}
			}
			else
			{
				//kprintf("\nelse\n");
				bs_map_t *jump = &(proctab[pid].map[i]);
				while(jump != NULL)
				{
					//kprintf("\nwhile\n");
					if(jump->vpno == vpno) //we found the exact page
					{
						ret_vpno = jump->vpno;
						flag = 1;
						//kprintf("\nif else if\n");
						break;
					}
					else if(jump->vpno < vpno && (jump->npages+jump->vpno) >= vpno) //we found the page in the range of base_page - npages
					{
						ret_vpno = jump->vpno;
						flag = 1;
						//kprintf("\nif else else if\n");
						break;
					}
					jump = jump->next;
				}
			}
		}
		if(flag)
			break;
	}
	return(ret_vpno);
}

void bs_kill(int pid)
{
	int i = 0;
	for(i = 0; i < NBS; i++)
	{
		if(proctab[pid].map[i].pid == pid)
		{
			xmunmap(proctab[pid].map[i].vpno);
			release_bs(i);
		}
	}

	//deleting processes page directory
	int page_dir = proctab[pid].pdbr/NBPG;

	frm_tab[page_dir-FRAME0].status = FRM_FREE;
	frm_tab[page_dir-FRAME0].refcnt = 0;
	frm_tab[page_dir-FRAME0].bs = -1;
	frm_tab[page_dir-FRAME0].bs_page = -1;
	frm_tab[page_dir-FRAME0].bs_next = NULL;
	frm_tab[page_dir-FRAME0].fifo = NULL;
	frm_tab[page_dir-FRAME0].age = 0;
	
	frm_map[page_dir-FRAME0].fr_status = FRM_UNMAPPED;
	frm_map[page_dir-FRAME0].fr_pid = -1;
	frm_map[page_dir-FRAME0].fr_vpno = -1;
	frm_map[page_dir-FRAME0].fr_refcnt = 0;
	frm_map[page_dir-FRAME0].fr_type = FR_PAGE;
	frm_map[page_dir-FRAME0].fr_dirty = -1;
	frm_map[page_dir-FRAME0].shared = NULL;
	frm_map[page_dir-FRAME0].bs_page_num = -1;

	if(frm_tab[(proctab[pid].pdbr/NBPG)-FRAME0].status == FRM_FREE)
		kprintf("\nFreed process %s's page directory",proctab[pid].pname);

}