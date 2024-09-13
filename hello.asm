#
# file: hello.asm
#

	SET R0 = hello_str
loop:	LOAD.B R1 <- *R0
 	TEST R1
	JMP_Z done
	OUT R1
	SET R2 = 1
	ADD R0+R2->R0
	JMP loop
done:	HALT

hello_str:
	STRING "Hello world!\n"
