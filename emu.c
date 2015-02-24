
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define LENGTHOFMCCODE 9
#define NUMOFINSTRUCTION 21
#define MAXMNEMONICCHAR 10
#define BITS32 32
#define BITS24 24
#define MEMSIZE 4096

struct Instruction{
	char *name;
	int opcode;
	int HasOpcode;
	int NumOfOperand;
	int IsValue;
	int IsOffset;
}; typedef struct Instruction ins;

struct Machinecode{
	char total[LENGTHOFMCCODE];
	char opcode[LENGTHOFMCCODE];
	char operand[LENGTHOFMCCODE];
}; typedef struct Machinecode mmc;

void ReadCode(char *FileName, ins *instruction);
void Execute(mmc *machinecode, ins *instruction, int instructioncount);
FILE *OpenReadFile (char *FileName);
char *AllocateMemory(int n);
char *TwosComReverse(long int DecimalNumber, int NumOfBits);
int ConvertOpcode (char *str);
int ConvertOperand (char *str, int NumOfBits);
int Searchinst(int opcode, ins *instruction);
void SplitMccode(mmc *machinecode, int addr);
void ProgramHelp(void);
void Storeinstruction(ins *instruction);

int flagT = 0;	//Flag for printing trace of instruction execution
int flagI = 0;	//Flag for printing instruction set
int flagS = 0;	//Flag for printing memory status before execution starts
int flagE = 0;	//Flag for printing memory status at the end of execution

int main(int argc, char *argv[])
{
	if (argc < 3) {
		ProgramHelp();
	}

	ins *instruction;
	instruction = (ins *)calloc (NUMOFINSTRUCTION, sizeof(ins));
	if(instruction == NULL) {
		exit(1); }
	
	if (strcmp(argv[1], "-trace") == 0) {	flagT = 1;	}
	else if (strcmp(argv[1], "-isa") == 0) {	flagI = 1;	}
	else if (strcmp(argv[1], "-start") == 0) {	flagS = 1;	}
	else if (strcmp(argv[1], "-end") == 0) {	flagE = 1;	}
	else {	ProgramHelp();	}

	Storeinstruction(instruction);
	ReadCode(argv[2], instruction); 

	return 0;
}

void ReadCode(char *FileName, ins *instruction)  //Function split machine code into operand and opcode and stores them in strcuture
{
	int addr = 0;
	int instructioncount = 0;
	mmc *machinecode;
	machinecode = (mmc *)calloc (MEMSIZE, sizeof(mmc));
	if(machinecode == NULL) {
		exit(1); }

	FILE *fp;
	int c, i =0, count=0;
	fp = OpenReadFile(FileName);
	
	while (c != EOF) {
		c = fgetc (fp);
		
		if ((count>7) || c=='\n') {
			count = 0, i =0;
			instructioncount++;
			addr++;	
		}
		else {
			machinecode[addr].total[i] = c; i++; count++; //Reads character into structure in groups of 8 or after every newline
		}		
	} 
	fclose (fp);
	Execute(machinecode, instruction, instructioncount);
}

void Execute( mmc *machinecode, ins *instruction, int instructioncount)
{ 				
	int A = 0, B = 0, PC = 0, SP;
	int i, operand, opcode, addr = 0;
	int instructionsexecuted = 0;
	int memory[MEMSIZE] = {0}; 
	SP = MEMSIZE-1; 
	
	for (i =0; i<instructioncount; i++) {
		memory[i] = ConvertOperand(machinecode[i].total, BITS32); //uses the whole machinecode as operand
	}

	if (flagS) {		
		printf("This is the initial memory status before execution(Only filled memory are displayed):\n");
		printf("A=%i,B=%i,PC=%i,SP=%08X",A,B,PC,SP);
		for (i =0; i<instructioncount; i++) {
			if (i%3 == 0) {
				printf("\n");
			}
			printf("mem[%08X]=%08X, ", i, memory[i]);
		}
		printf("\n\n");
		exit(1);
	}

	for (addr = 0; addr<instructioncount;) {
		instructionsexecuted++;
		SplitMccode(machinecode,addr); //Splits code into operand and opcode

		operand = ConvertOperand (machinecode[addr].operand, BITS24);
		opcode = ConvertOpcode (machinecode[addr].opcode);
		i = Searchinst(opcode, instruction); //to get the instruction name
		switch (opcode) {
			case 0 : //ldc
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++; 		B = A;				A = operand;
			break;

			case 1 : //adc
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;		A = A + operand;
			break;

			case 2 : //ldl
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;		B = A;				A = memory[SP + operand];
			break;

			case 3 : //stl
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;		memory[SP + operand] = A;	A = B;
			break;

			case 4 : //ldnl
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;		A = memory[A + operand];
			break;

			case 5 : //stnl
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;		memory[A + operand] = B;
			break;

			case 6 : //add
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s \n",PC,SP,A,B,instruction[i].name); }
				PC++;		A = B + A;
			break;

			case 7 : //sub
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s \n",PC,SP,A,B,instruction[i].name); }
				PC++;		A = B - A;
			break;

			case 8 : //shl
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s \n",PC,SP,A,B,instruction[i].name); }
				PC++;		A = B << A;
			break;

			case 9 : //shr
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s \n",PC,SP,A,B,instruction[i].name); }
				PC++;		A = B >> A;
			break;

			case 10 : //adj
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;		SP = SP + operand;
			break;

			case 11 : //a2sp
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s \n",PC,SP,A,B,instruction[i].name); }
				PC++;		SP = A;		A = B;
			break;

			case 12 : //sp2a
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s \n",PC,SP,A,B,instruction[i].name); }
				PC++;		B = A;		A = SP;				
			break;

			case 13 : //call
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;		B = A;		A = PC;		PC = PC + operand;
			break;
		
			case 14 : //return
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s \n",PC,SP,A,B,instruction[i].name); }
				PC = A;		A = B;
			break;

			case 15 : //brz
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;			
				if (A == 0) {
					PC = PC + operand;
				}
			break;

			case 16 : //brlz
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;
				if (A  < 0) {
					PC = PC + operand;
				}
			break;

			case 17 : //br
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s %08X\n",PC,SP,A,B,instruction[i].name, operand); }
				PC++;		PC = PC + operand;
			break;

			case 18 : //HALT
				if (flagT) { printf("PC:%08X,SP:%08X,A:%08X,B:%08X,%s \n",PC,SP,A,B,instruction[i].name); printf ("Execution TRACE shown above\n");}
				printf("%i instrcutions were executed\n\n",instructionsexecuted);
				if (flagE) {
					printf("Final Memory status after execution:");
					for (i =0; i<MEMSIZE; i++) {
						if (i%3 == 0) {
							printf("\n");
						}
						printf("mem[%08X]=%08X, ", i, memory[i]);
					}
					printf("\n\n");
				}
				exit(1);
			break; 
		}
		addr = PC;
	}
}

FILE *OpenReadFile (char *FileName)
{
	FILE *fp;
	if ((fp = fopen(FileName,"rb"))== NULL) {
		printf("Cannot open Binary file.\n");
		exit(1);
	}
	return fp;
}

char *AllocateMemory(int n)
{
	char *str;
	str = (char *)calloc (n, sizeof(char));
	if (str == NULL){
		printf("Cannot Allocate Memoery");
		exit(2);
	}
	return str;
}


char *TwosComReverse(long int DecimalNumber, int NumOfBits) 
{
	int temp, i,j;
	char *binary;
	char *negativebinary;
	binary = AllocateMemory(NumOfBits+1);
	negativebinary = AllocateMemory(NumOfBits+1);
	temp = --NumOfBits;

	while (DecimalNumber > 0) { 
		binary[NumOfBits--] = DecimalNumber%2;
		DecimalNumber=DecimalNumber/2; 
	}
	if (binary[0] == 1) {
		for(i=temp; i>=0; ) {
			if (binary[i]== 1) {
				for (j=(i-1); j>=0; j--) {
					binary[j] = ~(binary[j])+2;
				}
				for(i=0; i<=temp; i++) {
					if (binary[i] == 1) {
						binary[i] = '1';}
					else { binary[i] = '0';}
				}
				sprintf(negativebinary, "-%s", binary);			
				return negativebinary;
			}
			i--;
		}
	}
	for(i=0; i<=temp; i++) {
		if (binary[i] == 1) {
			binary[i] = '1';}
		else { binary[i] = '0';}
	}
	return binary;
}


int ConvertOpcode (char *str)
{
	int number;
	number = strtol(str, NULL, 16);
	return number;
}

int ConvertOperand (char *str, int NumOfBits)
{
	long int number;
	number = strtol(str, NULL, 16);
	str = TwosComReverse(number, NumOfBits);
	number = strtol(str, NULL, 2);
	return number;
}


int Searchinst(int opcode, ins *instruction)
{
	int i;	
	for (i=0; i<NUMOFINSTRUCTION; i++) {
		if (instruction[i].name!=NULL && instruction[i].HasOpcode ==1 && instruction[i].opcode == opcode) {
			return i;
		}
	}
	return -1;
}

void SplitMccode(mmc *machinecode, int addr)
{
	int k, j;
	
	strncpy(machinecode[addr].operand, machinecode[addr].total, 6);
	machinecode[addr].operand[6] = '\0';
	k =0;
	for (j=6; j<8;) { 
		machinecode[addr].opcode[k++] = machinecode[addr].total[j++]; 
	} 
}

void ProgramHelp()
{
	printf("\nusage, <emulator> <option> <file.o>\n\n");
	printf("<option> can be any of the following:\n");
	printf("-trace	show instruction execution trace\n");
	printf("-isa	show instruction set\n");
	printf("-start	show memory before execution\n");
	printf("-end	show memory after execution\n\n");
	exit(1);
}

void Storeinstruction(ins *instruction)
{
	int i;
	for (i=0; i<NUMOFINSTRUCTION; i++) {
		instruction[i].name = AllocateMemory(MAXMNEMONICCHAR);
	}
strcpy(instruction[0].name,"data");  instruction[0].opcode = -1;  instruction[0].HasOpcode = 0;  instruction[0].NumOfOperand = 1;  instruction[0].IsOffset = 0;  instruction[0].IsValue = 1;
strcpy(instruction[1].name,"ldc");  instruction[1].opcode = 0;  instruction[1].HasOpcode = 1;  instruction[1].NumOfOperand = 1;  instruction[1].IsOffset = 0;  instruction[1].IsValue = 1;
strcpy(instruction[2].name,"adc");  instruction[2].opcode = 1;  instruction[2].HasOpcode = 1;  instruction[2].NumOfOperand = 1;  instruction[2].IsOffset = 0;  instruction[2].IsValue = 1;
strcpy(instruction[3].name,"ldl");  instruction[3].opcode = 2;	 instruction[3].HasOpcode = 1;  instruction[3].NumOfOperand = 1; instruction[3].IsOffset = 1;  instruction[3].IsValue =	0;
strcpy(instruction[4].name,"stl");  instruction[4].opcode = 3;  instruction[4].HasOpcode = 1;  instruction[4].NumOfOperand = 1;  instruction[4].IsOffset = 1;  instruction[4].IsValue = 0;
strcpy(instruction[5].name,"ldnl");  instruction[5].opcode = 4;  instruction[5].HasOpcode = 1;  instruction[5].NumOfOperand = 1;  instruction[5].IsOffset = 1;  instruction[5].IsValue = 0;
strcpy(instruction[6].name,"stnl");  instruction[6].opcode = 5;  instruction[6].HasOpcode = 1;  instruction[6].NumOfOperand = 1;  instruction[6].IsOffset = 1;  instruction[6].IsValue = 0;
strcpy(instruction[7].name,"add");  instruction[7].opcode = 6;  instruction[7].HasOpcode = 1;  instruction[7].NumOfOperand = 0;  instruction[7].IsOffset = 0;  instruction[7].IsValue = 0;
strcpy(instruction[8].name,"sub");  instruction[8].opcode = 7;  instruction[8].HasOpcode = 1;  instruction[8].NumOfOperand = 0;  instruction[8].IsOffset = 0;  instruction[8].IsValue = 0;
strcpy(instruction[9].name,"shl");  instruction[9].opcode = 8;  instruction[9].HasOpcode = 1;  instruction[9].NumOfOperand = 0;  instruction[9].IsOffset = 0;  instruction[9].IsValue = 0;
strcpy(instruction[10].name,"shr");  instruction[10].opcode = 9;  instruction[10].HasOpcode = 1;  instruction[10].NumOfOperand	=  0;  instruction[10].IsOffset	= 0;  instruction[10].IsValue = 0;
strcpy(instruction[11].name,"adj");  instruction[11].opcode = 10;  instruction[11].HasOpcode = 1;  instruction[11].NumOfOperand	= 1;  instruction[11].IsOffset = 0;  instruction[11].IsValue = 1;
strcpy(instruction[12].name,"a2sp");  instruction[12].opcode = 11;  instruction[12].HasOpcode = 1;  instruction[12].NumOfOperand = 0;  instruction[12].IsOffset = 0;  instruction[12].IsValue = 0;
strcpy(instruction[13].name,"sp2a");  instruction[13].opcode = 12;  instruction[13].HasOpcode =	1;  instruction[13].NumOfOperand = 0;  instruction[13].IsOffset	= 0;  instruction[13].IsValue = 0;
strcpy(instruction[14].name,"call");  instruction[14].opcode = 13;  instruction[14].HasOpcode =	1;  instruction[14].NumOfOperand = 1;  instruction[14].IsOffset	= 1;  instruction[14].IsValue = 0;
strcpy(instruction[15].name,"return");  instruction[15].opcode = 14;  instruction[15].HasOpcode	= 1;  instruction[15].NumOfOperand = 0;  instruction[15].IsOffset = 0;  instruction[15].IsValue = 0;
strcpy(instruction[16].name,"brz");  instruction[16].opcode = 15;  instruction[16].HasOpcode = 1;  instruction[16].NumOfOperand = 1;  instruction[16].IsOffset	= 1;  instruction[16].IsValue = 0;
strcpy(instruction[17].name,"brlz");  instruction[17].opcode = 16;  instruction[17].HasOpcode = 1;  instruction[17].NumOfOperand = 1;  instruction[17].IsOffset = 1;  instruction[17].IsValue = 0;
strcpy(instruction[18].name,"br");  instruction[18].opcode = 17;  instruction[18].HasOpcode = 1;  instruction[18].NumOfOperand = 1;  instruction[18].IsOffset = 1;  instruction[18].IsValue = 0;
strcpy(instruction[19].name,"HALT");  instruction[19].opcode = 18;  instruction[19].HasOpcode =	1;  instruction[19].NumOfOperand = 0;  instruction[19].IsOffset	= 0;  instruction[19].IsValue = 0;
strcpy(instruction[20].name,"SET");  instruction[20].opcode = -1;  instruction[20].HasOpcode = 0;  instruction[20].NumOfOperand = 1;  instruction[20].IsOffset = 0;  instruction[20].IsValue = 1;
	if (flagI) {
		for (i=0; i<NUMOFINSTRUCTION; i++) {
			printf("\nMnemonic:%6s\topcode:%2i\tValue(1/O):%i\tOffset(1/O):%i", instruction[i].name,instruction[i].opcode,instruction[i].IsValue,instruction[i].IsOffset);
		}
		printf("\n");
		exit(1);
	}
} 
