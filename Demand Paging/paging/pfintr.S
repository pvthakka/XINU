/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
	popl pferrcode     /* store the error code */
	pushfl		   /* store flag reg		*/
        cli                /* interrupt disabled 	*/
	pushal             /* save all general regs     */
   	call pfint         /* call paging fault ISR	*/
        popal              /* restore all general regs  */
	popfl		   /* restore flag reg		*/
        iret

