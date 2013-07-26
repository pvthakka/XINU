/* paging.h */
#ifndef _PAGING_H_
#define _PAGING_H_

typedef unsigned int	 bsd_t;
#define FP2FN(frm)  (((frm) - frm_map) + FRAME0)
#define FN2ID(fn)   ((fn) - FRAME0)
#define FP2PA(frm)  ((void*)(FP2FN(frm) * NBPG))
#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame		*/

#define NFRAMES 	1024	/* number of frames		*/

#define BSM_UNMAPPED	0
#define BSM_MAPPED	1

#define FRM_UNMAPPED	0
#define FRM_MAPPED	1

#define FR_PAGE		0
#define FR_TBL		1
#define FR_DIR		2

#define FRM_FREE 0
#define FRM_PGD 1
#define FRM_PGT 2
#define FRM_BS 3

#define FIFO		3
#define AGING		4

#define MAX_ID          7              /* You get 8 mappings, 0 - 8 */

#define BACKING_STORE_BASE	0x00800000
#define BACKING_STORE_UNIT_SIZE 0x00100000

#define NBS 8
#define BS_PAGES 2048

extern int total_bs_left;
extern int page_replace_policy;
extern int bs_size;
/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

typedef struct _frame_t {
	int status; /* FRM_FREE, FRM_PGD, FRM_PGT, FRM_BS*/

	/*If the frame is a FRM_PGT, refcnt is the number of mappings install 
	 in this PGT. release it when refcnt is zero. When the frame is
	 a FRM_BS, how many times this frame is mapped by processes. If refcnt
	 is zero, time to release the page*/
	int refcnt;

	/*Data used only if FRM_BS. The backstore pages this frame is mapping
	 to and the list of all the frames for this backstore*/
	bsd_t bs;
	int bs_page;
	struct _frame_t *bs_next;

	struct _frame_t *fifo; /* when the page is loaded, in ticks*/
	unsigned int age; /* Used for page replacement policy AGING */
	int frame_num;
} frame_t; //kernel

//typedef struct{
 // int bs_status;			/* MAPPED or UNMAPPED		*/
 // int bs_pid;				/* process id using this slot   */
  //int bs_vpno;				/* starting virtual page number */
  //int bs_npages;			/* number of pages in the store */
  //int bs_sem;				/* semaphore mechanism ?	*/
//} bs_map_t;

/*Each back store may be mapped into several address space.
 bs_map_t is used both by bs_t and process*/
typedef struct _bs_map_t {
	struct _bs_map_t *next;
	bsd_t bs; /* which bs is mapped*/
	int pid; /* process mapped this backstore*/
	int vpno; /* first virtual page the bs mapped to*/
	int npages;
	int base_page;
} bs_map_t; //kernel

typedef struct {
	int status;
	int as_heap; /* is this bs used by heap?*/
	int npages; /* number of pages in the store */
	bs_map_t *owners; /* where it is mapped*/
	frame_t *frm; /* the list of frames that maps this bs*/
	int base_page;
} bs_t; //kernel

typedef struct _fr_map_t {
  int fr_status;			/* MAPPED or UNMAPPED		*/
  int fr_pid;				/* process id using this frame  */
  int fr_vpno;				/* corresponding virtual page no*/
  int fr_refcnt;			/* reference count number of things pointing to your page*/
  int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
  int fr_dirty;
  struct _fr_map_t *shared;				/* shared frames	*/
  int frame_num;
  int bs_page_num;
}fr_map_t;

typedef struct _vmblock{
	unsigned long vaddr;
	unsigned long vlen;
	unsigned int num;
	struct _vmblock *next;
}vmblock;

extern bs_map_t bs_map[NBS];
extern fr_map_t frm_map[NFRAMES];
extern bs_t bs_tab[NBS];
extern frame_t frm_tab[NFRAMES];
extern frame_t *fifo_head, *fifo_tail;
extern int ni_page_table[((unsigned long) BACKING_STORE_UNIT_SIZE)*NBS/NBPG];

/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xmunmap(int);

/* given calls for dealing with backing store */

int get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(int src_frame, int dst_page);


/*creating common 4 page tables and 1 page directory
pt_t shared_page_table[4][1024];
pd_t shared_page_directory[4];*/

#endif