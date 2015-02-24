ASSEMBLER

1.	Assembler (asm.c) compiles without errors and warnings using (gcc -std=c99 -pedantic) and (gcc -std=c99 -pedantic -W -Wall -Wpointer-arith -Wwrite-strings -Wstrict-prototypes) respectively. It is subdivided in several functions, TranslateLine, PassOne and PassTwo are the major functions.
	a.	PassOne -	Populates symbol table and does most of error detection.
	b.	PassTwo -	Does some error checking that could not be done in PassOne and does the encoding into machinecode
	c.	TranslateLine -	Act as the parser for each line, ignore comments, white spaces and co.

2.	It works perfectly identifying all possible error in the source code, it includes a table structure that contains all the Instruction names and their various relevant properties such as operand type, number of operand accepted e.t.c.

3.	It helps to diagnose all error by identifying the line number of the error in source file.

4.	My Assembler (asm.c) generates THREE output files from every assembly source code (filename.asm) namely:

	a.	filename.lst -	contains the linenumber, machinecode and instructions
	b.	filename.log -	contains the execution status and errors/warnings associated with the source code
	c.	filename.o   -	contains only the machinecode in hexadecimal format
	
The source code filename is specified in the command line alongside the Assembler name e.g        "ass triangle"	where triangle is the name of the source code, the .asm extension is automatically appended by the assembler

5.	The program uses a single routine called (TranslateLine) to scan through the lines for both passes

6.	I also implemented the SET pseudo instruction and demonstrated it using 'test3.asm'.

7.	My program makes use of 'calloc' for dynamic memory allocation as it initializes and free memory automatically'

Below are the effects of errors and/or warnings on the output file:

1.	A program that contains at least one error (as indicated in the log file) will have the content of the output files flushed out at the end of execution as any output produced would be inaccurate or incomplete.

2.	A program that contain only warning or no warning(s) or error(s) at all will generate the outputs file as expected.

***********************************************************************************************
EMULATOR

1.	My Emulator (emu.c) works fine according to specification, it loads object file, produces memroy dump and executes properly. it includes the following options 
	
	a.	It does a trace of the execution displaying the program counter, stack pointer, registers A and B at every instruction until it encounters the HALT instruction of an error occurs along the line.

	b.	It can display the memory state before and after execution as well as the instruction set.

2.	The Emulator accept .o files as input and the user have the option of outputting three different files along side the instruction set on the terminal if executed successfully.

	a.	filename_emu_trace.log -	contains the trace of the executed instruction including the total count of instructions executed.
	b.	filename_emu_startmem.log -	contains the initial memory status.
	c.	filename_emu_endmem.log -	contains the final memory status.

3.	It gives a successful status prompt if execution complete without issues.





