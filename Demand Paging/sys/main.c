/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

typedef struct{
	long one;
	long two;
	int three;
	char four;
} node;

void halt();

proc_vcreate(char c) {
	//kprintf("\nInside proc\n");
	/*char *count = (char *)vgetmem(sizeof(char));
	//char count = 'A';
	*count = 'A';
	kprintf("\ncount is %d %c\n",count, *count);

	vfreemem(count, sizeof(char));

	count = (char *)vgetmem(sizeof(char));
	*count = 'B';
	kprintf("\ncount is %d %c\n",count, *count);

	vfreemem(count, sizeof(char));

	long int *ptr = (long int *)vgetmem(sizeof(long));
	*ptr = 34943743;
	kprintf("\nptr is %d %ld\n",ptr, *ptr);

	vfreemem(count, sizeof(long int));*/
	
	int i = 0;
	node *pt = (node *)vgetmem(sizeof(node));
	node *temp;
	while(pt != NULL)
	{
		pt->one = 1234+i;
		pt->two = 6789+i;
		pt->three = 356+i;
		pt->four = 'A'+i;
		//kprintf("\npt is %d, %ld, %ld, %d, %c\n",pt, pt->one, pt->two, pt->three, pt->four);
		i++;
		temp = pt;
		pt = (node *)vgetmem(sizeof(node));
	}

	kprintf("\n\nTest result %d. Should be 12800.",i);

	vfreemem(temp, sizeof(node));

	char *count = (char *)vgetmem(sizeof(char));
	if(count != NULL)
		kprintf("\n\nVFREEMEM working properly");
	else
		kprintf("\n\nVFREEMEM failed");

	pt = (node *)vgetmem(sizeof(node));
	if(pt == NULL)
		kprintf("\n\nVGETMEM boundary check passed");
	else
		kprintf("\n\nVGETMEM boundary check failed");
	
	kill(currpid);
}

proc_vcreate_fifo(char c) {

	int i = 0;
	node *pt = (node *)vgetmem(sizeof(node));
	node *temp = pt;
	while(pt != NULL)
	{
		pt->one = 1234+i;
		pt->two = 6789+i;
		pt->three = 356+i;
		pt->four = 'A'+i;
		//kprintf("\npt is %d, %ld, %ld, %d, %c\n",pt, pt->one, pt->two, pt->three, pt->four);
		i++;
		//temp = pt;
		pt = (node *)vgetmem(sizeof(node));
	}

	sleep(5);

	//reading the first heap page which will result in PF
	kprintf("\n\nAccessing first heap page %d (should be 1234), %ld (should be 6789), %ld (should be 356), %c (should be A)\n\n",temp->one, temp->two, temp->three, temp->four);
	kill(currpid);
}

proc1(char c) {
	
	bsd_t bs = get_free_bs();
	get_bs(bs, 250);

	char *addr = (char*) 0x20000000;

	int i = ((unsigned long) addr) >> 12;
	if (xmmap(i, bs, 250) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
	
	}
	for (i = 0; i < 250; i++) {
		*addr = 500 + i;
		addr += NBPG;	//increment by one page each time
	}

	sleep(5);
	xmunmap(0x20000000 >> 12);
	release_bs(bs);
	kill(currpid);
}

proc2(char c) {
	
	bsd_t bs = get_free_bs();
	get_bs(bs, 250);

	char *addr = (char*) 0x30000000;

	int i = ((unsigned long) addr) >> 12;
	if (xmmap(i, bs, 250) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
	
	}
	for (i = 0; i < 250; i++) {
		*addr = 1000 + i;
		addr += NBPG;	//increment by one page each time
	}

	sleep(5);
	xmunmap(0x30000000 >> 12);
	release_bs(bs);
	kill(currpid);
}

void test_fifo()
{
	char *addr = (char*) 0x40000000; //1G
	bsd_t bs = get_free_bs();

	int i = ((unsigned long) addr) >> 12;	// the ith page

	kprintf("\n\nTesting FIFO %d\n\n", i);
	
	get_bs(bs, 250);

	if (xmmap(i, bs, 250) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
	
	}
	for (i = 0; i < 250; i++) {
		*addr = i+0;
		addr += NBPG;	//increment by one page each time
	}

	int prA = vcreate(proc1, 100, 250, 25, "proc A", 'd');

	resume(prA);

	int prB = vcreate(proc2, 100, 250, 25, "proc B", 'd');

	resume(prB);

	char *addr2 = (char*) 0x50000000;
	bsd_t bs2 = get_free_bs();
	int j = ((unsigned long) addr2) >> 12;
	if (xmmap(j, bs2, 250) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	for (i = 0; i < 250; i++) {
		*addr2 = i+1500;
		addr2 += NBPG;	//increment by one page each time
	}
	
	char *addr3 = (char*) 0x60000000;
	bsd_t bs3 = get_free_bs();
	int k = ((unsigned long) addr3) >> 12;
	if (xmmap(k, bs3, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	char *access = (char *) 0x40000000;
	*access = 'P';
	kprintf("\nAccessing the first memory location to avoid page replacement in aging %c %lu\n",*access, (unsigned long)access);

	for (i = 0; i < 100; i++) {
		*addr3 = i+2000;
		addr3 += NBPG;	//increment by one page each time
	}

	addr = (char*) 0x40000000;
	kprintf("\nAccessing first ever page here %d\n", *addr);

	xmunmap(0x40000000 >> 12);
	xmunmap(0x50000000 >> 12);
	xmunmap(0x60000000 >> 12);
	release_bs(bs);
	release_bs(bs2);
	release_bs(bs3);
	sleep(10);
}

proc_share1(char c)
{
		char *x;
        char temp;
        get_bs(4, 100);
        xmmap(70000, 4, 100);    /* This call simply creates an entry in the backing store mapping */
        x = (char *)(70000*NBPG);
        *x = 'Y';                            /* write into virtual memory, will create a fault and system should proceed as in the prev example */
        kprintf("\nReading from proc 1 %c\n", *x);                        /* read back and check */
		sleep(5);
        xmunmap(70000); 
}

proc_share2(char c)
{
	 char *x;
     char temp_b;
     xmmap(60000, 4, 100);
     x = (char *)(60000 * NBPG);

	 sleep(10);
     kprintf("\nReading from proc 2 %c (should be Y) and no page fault should be generated\n", *x);
	 xmunmap(60000);
}

void test_share()
{
	int prA = vcreate(proc_share1, 100, 250, 25, "share1", 'd');

	resume(prA);

	int prB = vcreate(proc_share2, 100, 250, 25, "share2", 'd');

	resume(prB);
	sleep(15);
}

void test_default()
{
	char *addr = (char*) 0x40000000; //1G
	bsd_t bs = get_free_bs();

	int i = ((unsigned long) addr) >> 12;	// the ith page

	kprintf("\n\nHello World, Xinu lives %d\n\n", i);

	get_bs(bs, 200);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
			
	}
	for (i = 0; i < 16; i++) {
		*addr = 'A' + i;
		addr += NBPG;	//increment by one page each time
	}

	addr = (char*) 0x40000000; //1G
	for (i = 0; i < 16; i++) {
		kprintf("0x%08x: %c\n", addr, *addr);
		addr += 4096;       //increment by one page each time
	}

	xmunmap(0x40003000 >> 12);
	release_bs(bs);
	//resume(create(main,INITSTK,INITPRIO,INITNAME,INITARGS));
}

void test_aging()
{
	bsd_t bs1 = get_free_bs();
	get_bs(bs1, 256);

	bsd_t bs2 = get_free_bs();
	get_bs(bs2, 256);

	bsd_t bs3 = get_free_bs();
	get_bs(bs3, 256);

	bsd_t bs4 = get_free_bs();
	get_bs(bs4, 249);

	if (xmmap(40000, bs1, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(50000, bs2, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(60000, bs3, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(70000, bs4, 249) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}

	int i = 0;
	char *addr = (char *)(40000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(50000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1000 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(60000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(70000*NBPG);
	for (i = 0; i < 245; i++) {
		*addr = 2000 + i;
		addr += NBPG;	//increment by one page each time
	}

	//create 1 page fault which will result in page replacement
	bsd_t bs5 = get_free_bs();
	get_bs(bs5, 1);
	if (xmmap(80000, bs5, 1) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	addr = (char *)(80000*NBPG);
	*addr = 'A';

	//accessing the next in queue page for replacement
	kprintf("\nbreak\n");
	addr = (char *)(40002*NBPG);
	*addr = 'P';
	kprintf("\nHopefully avoided page %lu data %c\n",(unsigned long)addr, *addr);

	//create another page fault
	bsd_t bs6 = get_free_bs();
	get_bs(bs6, 1);
	if (xmmap(90000, bs6, 1) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	addr = (char *)(90000*NBPG);
	*addr = 'B';

	kprintf("\nPAGE STATES: %d %d\n",frm_tab[1032-1024].status, frm_tab[1034-1024].status);
	xmunmap(40000);
	xmunmap(50000);
	xmunmap(60000);
	xmunmap(70000);
	xmunmap(80000);
	xmunmap(90000);
}

proc_new_share1(char c)
{
	char *addr = (char*) 0x40050000;
	bsd_t bs = 6;
	int i = ((unsigned long) addr) >> 12;
	get_bs(bs, 200);

	kprintf("\nPROC A: got bs");
	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
		kprintf("\nPROC A: xmmap success");
	
	}

	for (i = 0; i < 10; i++) {
		kprintf("\nPROC A: in for loop count %d",i);
		*addr = 'A' + i;
		//kprintf("\n\n in for %s",*addr);
		addr += NBPG;	//increment by one page each time
	}

	xmunmap(0x40050000 >> 12);
}

proc_new_share3(char c)
{
	char *addr = (char*) 0x60050000;
	bsd_t bs = 6;
	int i = ((unsigned long) addr) >> 12;
	get_bs(bs, 200);

	kprintf("\nPROC C: got bs");
	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
		kprintf("\nPROC C: xmmap success");
	
	}

	for (i = 0; i < 10; i++) {
		kprintf("\nPROC C: in for loop count %d",i);
		*addr = 'X' + i;
		//kprintf("\n\n in for %s",*addr);
		addr += NBPG;	//increment by one page each time
	}

	sleep(5);
	xmunmap(0x60050000 >> 12);
}

proc_new_share2(char c)
{
	char *addr = (char*) 0x50050000;
	bsd_t bs = 6;
	int i = ((unsigned long) addr) >> 12;
	get_bs(bs, 200);
	
	kprintf("\nPROC B: got bs");
	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
		kprintf("\nPROC B: xmmap success\n");
	
	}


	for (i = 0; i < 10; i++) {
		kprintf("\nPROC B: in for loop count %d",i);
		*addr = 'M' + i;
		//kprintf("\n\n in for %s",*addr);
		addr += NBPG;	//increment by one page each time
	}

	int prA = vcreate(proc_new_share1, 100, 100, 25, "procA", 'd');
	resume(prA);

	int prC = vcreate(proc_new_share3, 100, 100, 25, "procC", 'd');
	resume(prC);

	sleep(10);

	addr = (char*) 0x50050000; //1G
	for (i = 0; i < 10; i++) {
		kprintf("\nPROC B: 0x%08x: %c", addr, *addr);
		addr += 4096;       //increment by one page each time
	}

	xmunmap(0x50050000 >> 12);
}

void test_new_share()
{
	int prB = vcreate(proc_new_share2, 100, 250, 25, "procB", 'd');

	resume(prB);

}

proc_share_fifo1()
{
	char *addr = (char*) 0x80050000;
	bsd_t bs = 6;
	int i = ((unsigned long) addr) >> 12;
	get_bs(bs, 256);

	kprintf("\nPROC A: got bs");
	if (xmmap(i, bs, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
		kprintf("\nPROC A: xmmap success");
	
	}

	for (i = 0; i < 256; i++) {
		//kprintf("\nPROC A: in for loop count %d",i);
		*addr = 'A';
		//kprintf("\n\n in for %s",*addr);
		addr += NBPG;	//increment by one page each time
	}

	sleep(10);

	//accessing the first shared frame
	addr = (char*) 0x80050000;
	kprintf("\nAccessing the first vpaddr %lu, data %c\n",(unsigned long)addr, *addr);
	xmunmap(0x80050000 >> 12);
}

proc_share_fifo2()
{
	char *addr = (char*) 0x90050000;
	bsd_t bs = 6;
	int i = ((unsigned long) addr) >> 12;
	get_bs(bs, 256);

	kprintf("\nPROC B: got bs");
	if (xmmap(i, bs, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	else {
		kprintf("\nPROC B: xmmap success");
	
	}

	for (i = 0; i < 256; i++) {
		//kprintf("\nPROC B: in for loop count %d",i);
		*addr = 'B';
		//kprintf("\n\n in for %s",*addr);
		addr += NBPG;	//increment by one page each time
	}

	sleep(5);

	//trying to avoid frame 1034 from getting aged
	addr = (char*)0x90052000;
	*addr = 'Z';
	kprintf("\nAvoided eviction of frame 1034 if aging used. addr = %lu, data = %c\n",(unsigned long)addr,*addr);
	//accessing the first shared frame
	addr = (char*)0x90050000;
	kprintf("\nAccessing the first vpaddr %lu, data %c\n",(unsigned long)addr, *addr);

	sleep(5);
	xmunmap(0x90050000 >> 12);
}

void test_sharing_fifo()
{
	int prA = vcreate(proc_share_fifo1, 100, 250, 25, "procA", 'd');
	resume(prA);

	int prB = vcreate(proc_share_fifo2, 100, 250, 25, "procB", 'd');
	resume(prB);

	bsd_t bs1 = get_free_bs();
	get_bs(bs1, 256);

	bsd_t bs2 = get_free_bs();
	get_bs(bs2, 256);

	bsd_t bs3 = get_free_bs();
	get_bs(bs3, 242);

	if (xmmap(40000, bs1, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(50000, bs2, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(60000, bs3, 242) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}

	int i = 0;
	char *addr = (char *)(40000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(50000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1000 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(60000*NBPG);
	for (i = 0; i < 242; i++) {
		*addr = 1500 + i;
		addr += NBPG;	//increment by one page each time
	}

	//generating a page fault
	bsd_t bs5 = get_free_bs();
	get_bs(bs5, 1);
	if (xmmap(30000, bs5, 1) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	addr = (char *)(30000*NBPG);
	*addr = 'C';

	sleep(20);

	xmunmap(40000);
	xmunmap(50000);
	xmunmap(60000);
	xmunmap(30000);
}

void test_vcreate()
{
	int prA = vcreate(proc_vcreate, 100, 50, 25, "procA", 0);
	resume(prA);
}

void test_vcreate_fifo()
{
	int prA = vcreate(proc_vcreate_fifo, 100, 50, 25, "procA", 'C');
	resume(prA);

	bsd_t bs1 = get_free_bs();
	get_bs(bs1, 256);

	bsd_t bs2 = get_free_bs();
	get_bs(bs2, 256);

	bsd_t bs3 = get_free_bs();
	get_bs(bs3, 256);

	bsd_t bs4 = get_free_bs();
	get_bs(bs4, 193);

	if (xmmap(40000, bs1, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(50000, bs2, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(60000, bs3, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(70000, bs4, 193) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}

	int i = 0;
	char *addr = (char *)(40000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(50000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1000 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(60000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(70000*NBPG);
	for (i = 0; i < 193; i++) {
		*addr = 2000 + i;
		addr += NBPG;	//increment by one page each time
	}

	//create 1 page fault which will result in page replacement of the heap
	bsd_t bs5 = get_free_bs();
	get_bs(bs5, 1);
	if (xmmap(80000, bs5, 1) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	addr = (char *)(80000*NBPG);
	*addr = 'A';

	sleep(10);
}

void test_xmap_fifo()
{
	bsd_t bs1 = get_free_bs();
	get_bs(bs1, 256);

	bsd_t bs2 = get_free_bs();
	get_bs(bs2, 256);

	bsd_t bs3 = get_free_bs();
	get_bs(bs3, 256);

	bsd_t bs4 = get_free_bs();
	get_bs(bs4, 249);

	if (xmmap(40000, bs1, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(50000, bs2, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(60000, bs3, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(70000, bs4, 249) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}

	int i = 0;
	char *addr = (char *)(40000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(50000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1000 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(60000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(70000*NBPG);
	for (i = 0; i < 245; i++) {
		*addr = 2000 + i;
		addr += NBPG;	//increment by one page each time
	}

	//create 10 page faults which will result in page replacement
	bsd_t bs5 = get_free_bs();
	get_bs(bs5, 10);
	if (xmmap(80000, bs5, 10) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	addr = (char *)(80000*NBPG);
	for(i = 0; i < 10; i++) {
		*addr = 2500 + i;
		addr += NBPG;	//increment by one page each time

		//accessing the 8th page here to prevent replacement in aging
		char *temp = (char *)(40006*NBPG);
		*temp = 'P';
	}

	//unmapping first bs
	xmunmap(40000);

	//creating and mapping new bs. Now while accessing these there should not be a replacement call
	kprintf("\n\nTHERE SHOULD BE NO PAGE REPLACEMENT BEYOND THIS LINE");
	bsd_t bs6 = 6;
	get_bs(bs6, 10);
	if (xmmap(90000, bs6, 10) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	addr = (char *)(90000*NBPG);
	for (i = 0; i < 10; i++) {
		*addr = 'A' + i;
		addr += NBPG;	//increment by one page each time
	}
	//reading the new bs
	addr = (char *)(90000*NBPG);
	for (i = 0; i < 10; i++) {
		kprintf("\n0x%08x: %c", addr, *addr);
		addr += 4096;       //increment by one page each time
	}
	
}

void test_fifo_tlb()
{
	bsd_t bs1 = get_free_bs();
	get_bs(bs1, 256);

	bsd_t bs2 = get_free_bs();
	get_bs(bs2, 256);

	bsd_t bs3 = get_free_bs();
	get_bs(bs3, 256);

	bsd_t bs4 = get_free_bs();
	get_bs(bs4, 249);

	if (xmmap(40000, bs1, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(50000, bs2, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(60000, bs3, 256) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	
	if (xmmap(70000, bs4, 249) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}

	int i = 0;
	char *addr = (char *)(40000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(50000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1000 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(60000*NBPG);
	for (i = 0; i < 256; i++) {
		*addr = 1500 + i;
		addr += NBPG;	//increment by one page each time
	}
	addr = (char *)(70000*NBPG);
	for (i = 0; i < 245; i++) {
		*addr = 2000 + i;
		addr += NBPG;	//increment by one page each time
	}

	//create 10 page faults which will result in page replacement
	bsd_t bs5 = get_free_bs();
	get_bs(bs5, 10);
	if (xmmap(80000, bs5, 10) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}
	addr = (char *)(80000*NBPG);
	for(i = 0; i < 10; i++) {
		*addr = 2500 + i;
		addr += NBPG;	//increment by one page each time
	}

	//accessing the first 5 pages. There should be page replacement and the code should run
	addr = (char *)(40000*NBPG);
	for (i = 0; i < 5; i++) {
		kprintf("\n\nData at %08x location is %d",addr, *addr);
		addr += NBPG;	//increment by one page each time
	}

}

int main() {

	//srpolicy(AGING);
	//test_fifo();
	//test_share();
	//test_default();
	//test_new_share();
	//test_aging();
	test_sharing_fifo();
	//test_vcreate();
	//test_vcreate_fifo();
	//test_xmap_fifo();
	//test_fifo_tlb();
	return 0;
}



