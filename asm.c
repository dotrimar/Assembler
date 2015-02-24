#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define BITS32 32
#define BITS24 24
#define NUMOFINSTRUCTION 21
#define MAXMNEMONICCHAR 10
#define LENGTHOFMCCODE 9

struct instructionset
{
	char *name;
	int opcode;
	int HasOpcode;
	int NumOfOperand;
	int IsValue;
	int IsOffset;
}; typedef struct instructionset ins;

struct Buffer  //Temporary buffer for storing parsed lines
{
	char *labelString;
	int labCount;

	char *MnemonicString;
	int MnemonicCount;	
	
	char *OperandString;
	int OperandCount;
}; typedef struct Buffer buff;

struct Label
{
	char *symbol;
	int addr;
	int hit;
	int errorline;
}; typedef struct Label lab;


char *DecTo2Com (long int DecimalNumber, int NumOfBits);
int IsNumeric (char *str);
int IsAlphaNumeric (char *str);
char *AllocateMemory (int n);
FILE *OpenWriteFileAppend (char *FileName);
FILE *OpenWriteFile (char *FileName);
FILE *OpenReadFile (char *FileName);
FILE *WriteBinaryFile (char *FileName);
int FindMaxWordLength (char *FileName);
int NumberOfLines (char *FileName, int n);
void Storeinstruction (ins *instruction); 
int SearchSymbolTable (lab *symtab, char *str);
void insertSymbolTable (lab *symtab, char *str, int addr, int errorline);
void TrackLabelUsage(lab *symtab, char *str);
int Searchinst (char *str, ins *instruction);
buff *TranslateLine (char *line);
void PassOne (char *FileName, ins *p, lab *symtab);
void PassTwo (char *FileName, ins *p, lab *symtab);
void PrintError (FILE *fl, int error, int errorline,  buff *s);

int ProgramLength;			//Number of lines of Assembly source code
int ProgramMaxLineLength;		//Maximum number of character on a line
int error = 0;
int errorline;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("You haven't specify a valid file name! Format is: \n <program name> <sourcefile*>  *The sourcefile name is without an extension\n");
		exit(1);
	}
	
	ProgramMaxLineLength = FindMaxWordLength(argv[1]);			
	ProgramLength = NumberOfLines(argv[1], ProgramMaxLineLength+1);
	
	ins *instruction = (ins *)calloc (NUMOFINSTRUCTION, sizeof(ins)); //initialize instruction set table
		if (instruction == NULL){
			printf("Cannot Allocate Memory");
			exit(2);
		}

	lab *symtab = (lab *)calloc (ProgramLength, sizeof(lab)); //initialize symbol table
		if (symtab == NULL){
			printf("Cannot Allocate Memory");
			exit(2);
		}

	Storeinstruction(instruction); 
	PassOne(argv[1], instruction, symtab);
	PassTwo(argv[1], instruction, symtab);
	
	return 0;
}


buff *TranslateLine(char *line)
{
	buff *s = (buff *)calloc (ProgramMaxLineLength, sizeof(buff)); //Temporary buffer that will hold Mnemonics, labels and operands
	if (s == NULL){
		printf("Cannot Allocate Memory");
		exit(2);
	}

	char *token;
	char *templine = AllocateMemory(ProgramMaxLineLength+1);
	char* str; 
	int i;
	for (i = strlen(line)-1; i >= 0; i--) { 
 		if (line[i] == '\n' || line[i] == ';') { //To ignore comment 
			line[i] = '\0';
		}
		if ((line[i] == '\t') || (line[i] == '\'') || (line[i] == '`')) {
			line[i] = ' ';
 		}
		if (line[i]==':' && line[i+1] != ' ') { //To cater for instruction that has no space between : and mnemonics
			strcpy(templine, line); 
			templine = strcat(strtok(templine, ":"), ": "); //Extract lab and add space after :
			line = strcat(templine, &line[i+1]); //return back to main line
		}
	}
	s->labCount = 0;
	s->MnemonicCount = 0;
	s->OperandCount = 0;
 	for(str = line; ((token = strtok(str," ")) != NULL); str = NULL) { 
		if(s->MnemonicCount == 0) {
			if(token[strlen(token) - 1] == ':') { //copy symbol
					token[strlen(token) - 1] = '\0';
					s->labelString = AllocateMemory(strlen(token)+1);
					strcpy(s->labelString, token);
					s->labCount++;
			}
			else { //copy mnemonic
				s->MnemonicString = AllocateMemory(strlen(token)+1);
				strcpy(s->MnemonicString, token);
				s->MnemonicCount++;
			}
		}
		else { //copy operands
			s->OperandString = AllocateMemory(strlen(token)+1);
			strcpy(s->OperandString, token);
			s->OperandCount++;
		} 
	}
	return s;	
}

void PassOne(char *FileName, ins *p, lab *symtab)
{	
	FILE *fs, *fl;
	char *line, *linetoprint;
	
	line = AllocateMemory(ProgramMaxLineLength+1);
	linetoprint = AllocateMemory(ProgramMaxLineLength+1);
	int i, pc=1, linecounter = 0; 
	errorline =1;
	char *LogFile  = AllocateMemory(strlen(FileName)*2);
	char *SourceFile  = AllocateMemory(strlen(FileName)*2);
	strcpy(LogFile, FileName);
	strcpy(SourceFile, FileName);

	fl = OpenWriteFile (strcat(LogFile,".log"));
	fs = OpenReadFile (strcat(SourceFile,".asm"));
 
	while (fgets(line, ProgramMaxLineLength+1, fs) != NULL) { printf("ln;%i,pc;%i,%s", linecounter,pc,line);
		buff *s = TranslateLine(line);	//read each line into a buffer structure 

		if ((s->labCount!=0) || (s->MnemonicCount != 0))  { 
			if ((s->labCount !=0) && (s->MnemonicCount != 0)) {
				if (!isalpha(s->labelString[0]) || !IsAlphaNumeric (s->labelString)) { 
					error = 1;    PrintError (fl, error, errorline, s); 
				}
				else if (SearchSymbolTable(symtab, s->labelString) >= 0) { //Duplicate label
					error = 2;    PrintError (fl, error, errorline, s); 
				}
				else {
					if ((s->MnemonicCount != 0)  && strcmp(s->MnemonicString, "SET") == 0) { //To implement the SET instruction		
						insertSymbolTable (symtab, s->labelString, atoi(s->OperandString), errorline);
					}
					else { printf("ln;%i,pc;%i, label;%s\n", linecounter,pc,s->labelString);
						insertSymbolTable (symtab, s->labelString, linecounter, errorline);
					}
				}
			}
			else if ((s->labCount !=0) && (s->MnemonicCount == 0)){ //When only label on line
				if (isalpha(s->labelString[0]) == 0) {
					error = 3;    PrintError (fl, error, errorline, s); 
				}
				else if (SearchSymbolTable(symtab, s->labelString) >= 0) {
					error = 4;    PrintError (fl, error, errorline, s); 
				}
				else { printf("ln;%i,pc;%i, label;%s\n", linecounter,pc,s->labelString);
					insertSymbolTable (symtab, s->labelString, linecounter, errorline);
				}
				linecounter--; pc--;
			}
			if (s->MnemonicCount != 0) { //check instruction code, operands and co..	
				if ((i = Searchinst(s->MnemonicString, p))<0) {
					error = 5;    PrintError (fl, error, errorline, s); 
				}
				else if (p[i].NumOfOperand > s->OperandCount) {
					error = 6;    PrintError (fl, error, errorline, s);
				}
				else if (p[i].NumOfOperand < s->OperandCount) { 
					error = 7;    PrintError (fl, error, errorline, s); 
				}
				else if ((p[i].NumOfOperand == s->OperandCount) && (s->OperandCount>0)) {
					if (isalpha(s->OperandString[0])) { //Label operand
						//undefined label are identified in pass 2
					}
					else if ( ((s->OperandString[0])=='0' && isdigit(s->OperandString[1])) || ((s->OperandString[0])=='0' && (s->OperandString[1])=='x')  ) { //Hex or Octal
						if (  (s->OperandString[0])=='0' && (s->OperandString[1])=='x' ) { //hex
							for (i=2; i<(int)strlen(s->OperandString); i++) {
								if (!isxdigit(s->OperandString[i])) {
									error = 8;    PrintError (fl, error, errorline, s); 
								}
							}
						}
						if ( (s->OperandString[0])=='0' && isdigit(s->OperandString[1]) ) { //Octal
							for (i=1; i<(int)strlen(s->OperandString); i++) {
								if (   ((s->OperandString[i]) > 7) || ( isdigit(s->OperandString[i]) )   ) {
									error = 9;    PrintError (fl, error, errorline, s); 
									break;
								}
							}
						}
					}
					else if (!IsNumeric(s->OperandString)) {
						error = 10;    PrintError (fl, error, errorline, s); 
					}
				}
			}
			linecounter++; pc++;
		}
		errorline++; 
	}
	fclose (fl);	fclose (fs);
}

void PassTwo(char *FileName, ins *p, lab *symtab)
{ 
	FILE *fo, *fl, *fs, *fb;
	char *line, *linetoprint;
	char Total[LENGTHOFMCCODE];
	
	line = AllocateMemory(ProgramMaxLineLength+1);
	linetoprint = AllocateMemory(ProgramMaxLineLength+1);
	
	int i, j, operand;
	int linecounter = 0, pc=1, errorline=1;
	char *ListFile = AllocateMemory(strlen(FileName)*2), *LogFile = AllocateMemory(strlen(FileName)*2), *SourceFile = AllocateMemory(strlen(FileName)*2), *BinaryFile = AllocateMemory(strlen(FileName)*2);
	
	strcpy(ListFile, FileName);
	strcpy(LogFile, FileName);
	strcpy(SourceFile, FileName);
	strcpy(BinaryFile, FileName);
	
	fo = OpenWriteFile (strcat(ListFile,".lst"));
	fl = OpenWriteFileAppend (strcat(LogFile,".log"));
	fs = OpenReadFile (strcat(SourceFile,".asm"));
	fb = WriteBinaryFile (strcat(BinaryFile,".o"));

	while (fgets(line, ProgramMaxLineLength+1, fs) != NULL) { printf("ln;%i,pc;%i,%s", linecounter,pc,line);
		strcpy(linetoprint, line);  
		buff *s = TranslateLine(line);		//read each line into a buffer structure

		if ((s->MnemonicCount != 0) && (s->OperandCount>0)) {  //Mnemonics and Operand present in instruction
				i = Searchinst(s->MnemonicString, p);
				if (isalpha(s->OperandString[0]) != 0) {  //If operand is label
					TrackLabelUsage(symtab, s->OperandString);
					if ((j = SearchSymbolTable(symtab, s->OperandString)) < 0) { //Undefined Label
						error = 11;    PrintError (fl, error, errorline, s); 
					}
					else if (((j = SearchSymbolTable(symtab, s->OperandString)) >= 0) && (p[i].IsValue == 1)) { // If operand is label and not offset (i.e its value)
						operand = symtab[j].addr;
						operand = strtol(DecTo2Com(operand, BITS24), NULL, 2);
						sprintf(Total, "%06X%02X",operand, p[i].opcode);
						fprintf(fb, "%8s\n",Total);
						fprintf(fo, "%04X | %8s | %s", linecounter, Total, linetoprint);
					}
					else { //If operand is label and offset
						operand = symtab[j].addr-pc;
						operand = strtol(DecTo2Com(operand, BITS24), NULL, 2);
						if (symtab[j].addr==linecounter) { //If address is same as location counter, an infinite loop
							error = 12;    PrintError (fl, error, errorline, s);
						}
						else {
							sprintf(Total, "%06X%02X",operand, p[i].opcode);
							fprintf(fb, "%8s\n",Total);
							fprintf(fo, "%04X | %8s | %s", linecounter, Total, linetoprint); 
						}
					}
				}
				else { //If Operand is not lab
					if (p[i].HasOpcode == 0) { //Mnemonics Without Opcode such as SET and data
						operand = strtol(s->OperandString, NULL, 0);
						operand = strtol(DecTo2Com(operand, BITS32), NULL, 2);
						
						sprintf(Total, "%08X",operand); 
						fprintf(fb, "%8s\n",Total);
						fprintf(fo, "%04X | %8s | %s", linecounter, Total, linetoprint);
					}
					else { //Mnemonics has opcode
						operand = strtol(s->OperandString, NULL, 0); 
						operand = strtol(DecTo2Com(operand, BITS24), NULL, 2);
						sprintf(Total, "%06X%02X",operand, p[i].opcode); 
						fprintf(fb, "%8s\n",Total); 
						fprintf(fo, "%04X | %8s | %s", linecounter, Total, linetoprint);
					}
				}
			//}
			linecounter++, pc++;
		}
		else if ((s->MnemonicCount == 0) && (s->OperandCount == 0) && (s->labelString ==0)) { //Lines without lab, Mnemonic & Operand
			fprintf(fo, "%s", linetoprint);
		}
		else if ((s->MnemonicCount == 0) && (s->OperandCount == 0) && (s->labelString !=0)) { //Only lab on line
			fprintf(fo, "%04X |          | %s", linecounter, linetoprint);
		}
		else if ((s->MnemonicCount != 0) && (s->OperandCount == 0)) { //Mnemonics without operand
			if ((i = Searchinst(s->MnemonicString, p)) >= 0) {
				sprintf(Total, "000000%02X", p[i].opcode);
				fprintf(fb, "%8s\n",Total);
				fprintf(fo, "%04X | %8s | %s", linecounter, Total, linetoprint);
				linecounter++, pc++;
			}
		}
	errorline++;
	}
	for (i=0; i<ProgramLength; i++) {
		if ((symtab[i].symbol!=NULL) && (symtab[i].hit == 0)){
			fprintf(fl, "%sLine:%i: '%s' %s\n", "WARNING: ", symtab[i].errorline, symtab[i].symbol, "is an unused label ");
		}
	} 

	if (error > 0) {
		//fclose (fo);
		//fo = freopen(BinaryFile,"w");
		//freopen(BinaryFile, "w", fo); 
		//remove (ListFile); remove (BinaryFile); //Deletes errornous output files
		fprintf (fl, "\n\n%s\"%s\"%s No output file was generated.", "FAILURE!!! .....Assembling of ", SourceFile, " did not complete accurately or was terminated abruptly! Issues encountered are listed above...");
	}
	else if (error == 0) {
		fprintf (fl, "\n\n%s\"%s\" %s \"%s\" and \"%s\" %s", "SUCCESS! .....Assembling of ", SourceFile, " completed accurately... See", ListFile, BinaryFile, "for output.");
	}
	/*for (i=0; i<ProgramLength; i++) {
		if (symtab[i].symbol!=NULL) {
			printf("%i,%s,%i, %i\n ", i, symtab[i].symbol, symtab[i].addr, symtab[i].hit );
		}
	}*/
	fclose (fo); fclose (fl); fclose (fs);
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

int NumberOfLines(char *FileName, int n)
{
	char *File = AllocateMemory(strlen(FileName)*2);
	strcpy(File, FileName);
	strcat(File,".asm");
	FILE *fp;
	int line;
	char *str;
	fp = OpenReadFile(File);
	line = 0;
	str = AllocateMemory(n);
	while (!feof(fp)){
	if (fgets(str, n, fp) != NULL){
		line++;
		printf("%05d %s", line, str);
		}
	}
	fclose (fp);
	return line;
}
	
int FindMaxWordLength(char *FileName)
{
	char *File = AllocateMemory(strlen(FileName)*2);
	strcpy(File, FileName);
	strcat(File,".asm");
	FILE *fp;
	char c; int count=0;
	int maxword =0; 
	fp = OpenReadFile (File);
	while((c = fgetc(fp)) != EOF){ 
		count++;		
		if (c == '\n'){
			if (count > maxword){
				maxword = count;
			}count = 0;
		}
	}
	fclose(fp);
	return (maxword); 
}

FILE *OpenReadFile (char *FileName)
{
	FILE *fp;
	if ((fp = fopen(FileName,"r"))== NULL) {
		printf("Cannot open file.\n");
		exit(1);
	}
	return fp;
}

FILE *OpenWriteFile (char *FileName)
{
	FILE *fp;
	if ((fp = fopen(FileName,"w"))== NULL) {
		printf("Cannot write to file.\n");
		exit(1);
	}
	return fp;
}

FILE *WriteBinaryFile (char *FileName)
{
	FILE *fp;
	if ((fp = fopen(FileName,"wb"))== NULL) {
		printf("Cannot write binary file.\n");
		exit(1);
	}
	return fp;
}

FILE *OpenWriteFileAppend (char *FileName)
{
	FILE *fp;
	if ((fp = fopen(FileName,"a"))== NULL) {
		printf("Cannot append to file.\n");
		exit(1);
	}
	return fp;
}

int Searchinst(char *str, ins *instruction)
{
	int i;	
	for (i=0; i<NUMOFINSTRUCTION; i++) {
		if (instruction[i].name!=NULL && (strcmp(instruction[i].name, str)==0)) {
			return i;
		}
	}
	return -1;
}

void insertSymbolTable(lab *symtab, char *str, int addr, int errorline) 
{
	static int j=0;
	symtab[j].symbol = AllocateMemory(strlen(str)+1);
	strcpy (symtab[j].symbol, str);
	symtab[j].addr = addr;
	symtab[j].errorline = errorline;
	j++;
}

int SearchSymbolTable(lab *symtab, char *str)
{
	int i; 
	for (i=0; i<ProgramLength; i++) {
		if (symtab[i].symbol!=NULL && strcmp(symtab[i].symbol, str) == 0) { 
				return i;
		}
	}
	return -1; 
}	

void TrackLabelUsage(lab *symtab, char *str)
{
	int j;
	if ((j = SearchSymbolTable(symtab, str)) >= 0) {
		symtab[j].hit = 1;
	}
}




int IsNumeric (char *str)
{
	int i;
	if ((str[0] == '-') || (str[0] == '+')) {
		for(i=1; i<(int)strlen(str); i++) {
			if (!isdigit(str[i])) {
		             return 0;
			}		
		}
	}
	else {
		for(i=0; i<(int)strlen(str); i++) {
			if (!isdigit(str[i])) {
		             return 0;
			}		
		}
	}
	return 1;
}

int IsAlphaNumeric (char *str)
{
	int i;
	for(i=0; i<(int)strlen(str); i++) {
		if (!isalnum(str[i])) {
			return 0;
		}		
	}
	return 1;
}

//converts decimal number to 2's complement in binary
char *DecTo2Com(long int DecimalNumber, int NumOfBits) 
{
	int temp, i,j;
	int flag = 0;
	char *binary;
	binary = AllocateMemory(NumOfBits+1);
	
	temp = --NumOfBits;
	if (DecimalNumber < 0) {
		flag = 1;
		DecimalNumber = -DecimalNumber;
	}
	while (DecimalNumber > 0) { 
		binary[NumOfBits--] = DecimalNumber%2;
		DecimalNumber=DecimalNumber/2; 
	}
	if (flag) {
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
				return binary;
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

void PrintError (FILE *fl, int error, int errorline, buff *s)
{
	switch (error) {
		case 1 : fprintf(fl, "%sLine:%i: '%s'%s\n", "ERROR: ", errorline, s->labelString, " is a bogus label name,name must start with an Alphabet and must contain only alphanumeric characters");	break;
		case 2 : fprintf(fl, "%sLine:%i: '%s'%s\n", "ERROR: ", errorline, s->labelString, " already exist in the symboltable");	break;
		case 3 : fprintf(fl, "%sLine:%i: '%s'%s\n", "ERROR: ", errorline, s->labelString, " is a bogus label name,name must start with an Alphabet");	break;
		case 4 : fprintf(fl, "%sLine:%i: '%s'%s\n", "ERROR: ", errorline, s->labelString, " already exist in the symboltable");	break;
		case 5 : fprintf(fl, "%sLine:%i: '%s',%s\n","ERROR: ", errorline, s->MnemonicString," is an unknown Mnemonic, cannot determine the right number of operand or operand type!");	break;
		case 6 : fprintf(fl, "%sLine:%i: %s'%s'\n", "ERROR: ", errorline, "Missing Operand(s) in Mnemonic ", s->MnemonicString);	break;
		case 7 : fprintf(fl, "%sLine:%i: %s'%s'\n", "ERROR: ", errorline, "Unexpected Operand(s) in Mnemonic ", s->MnemonicString);	break;
		case 8 : fprintf(fl, "%sLine:%i: '%s'%s\n", "ERROR: ", errorline, s->OperandString, " is not a valid Hexadecimal number\n");	break;
		case 9 : fprintf(fl, "%sLine:%i: '%s'%s\n", "ERROR: ", errorline, s->OperandString, " is not a valid Octal number");	break;
		case 10 : fprintf(fl, "%sLine:%i: %s\n", "ERROR: ", errorline, "Nnumeric Operand expected");	break;
		case 11 : fprintf(fl, "%sLine:%i: '%s',%s\n", "ERROR: ",errorline, s->OperandString, "No such label exist(Undefined label)"); break;
		case 12 : fprintf(fl, "%sLine:%i: %s\n", "ERROR: ", errorline, "An infinite loop"); break;
		
	}
}

void Storeinstruction(ins *instruction)
{
	int i;
	for (i=0; i<NUMOFINSTRUCTION; i++) {
		instruction[i].name = AllocateMemory(MAXMNEMONICCHAR);
	}

//	Mnemonic			OpCode							NumberofOperand				OperandType(Offset)		OperandType(Value)
//	--------			------------------------------------------		---------------				-------------------		------------------
strcpy(instruction[0].name,"data");  instruction[0].opcode = 0;  instruction[0].HasOpcode = 0;  instruction[0].NumOfOperand = 1;  instruction[0].IsOffset = 0;  instruction[0].IsValue = 1;
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
strcpy(instruction[20].name,"SET");  instruction[20].opcode = 0;  instruction[20].HasOpcode = 0;  instruction[20].NumOfOperand = 1;  instruction[20].IsOffset = 0;  instruction[20].IsValue = 1;

	for (i=0; i<NUMOFINSTRUCTION; i++) {
		printf("i;%i,nam;%s,opcode;%i,HasOpcode;%i,NumOfOperand;%i,IsValue;%i,IsOffset;%i,\n", i,instruction[i].name,instruction[i].opcode,instruction[i].HasOpcode,instruction[i].NumOfOperand,instruction[i].IsValue,instruction[i].IsOffset);
	}
}

