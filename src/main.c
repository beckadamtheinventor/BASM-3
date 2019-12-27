/*
 *--------------------------------------
 * Program Name: BASM 3
 * Author: Adam "beckadamtheinventor" Beckingham
 * License: GPL3
 * Description: An assembler for the eZ80 microprocessor, for the TI-84+CE line of graphing calculators
 *--------------------------------------
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fileioc.h>

#include "opcodes.h"

#define min(a,b) a<b?a:b
#define max(a,b) a>b?a:b
#define write(a,b,c) if (b){ti_Write(a,b,1,c);}


void *readTokens(char *buffer,unsigned int amount,void *ptr);
void markUndefLabel(const uint8_t *data);
void markDefLabel(const uint8_t *data,int org);
int assemble(const char *inFile, char *outFile);
uint8_t *getEmitData(const char *name);
uint8_t *checkIncludes(const char *name);
uint8_t *searchIncludeFile(const char *fname, const char *cname);
uint8_t *emitNumber(const char *name,uint8_t bytes);
int includeFile(const char *fname);

void error(const char *str,const char *word);
void print(const char *str);
void printX(const char *str,int amt);
void printAt(const char *str,uint8_t x,uint8_t y);
void printXAt(const char *str,int amt,uint8_t x,uint8_t y);
char *trimWord(char *str);
char *upperCaseStr(char *str);
sk_key_t pause(void);


typedef struct __include_entry_t {
	char fname[8];
	void *next;
} include_entry_t;


const char *TEMP_FILE = "_BASMtmp";

uint8_t ADDR_BYTES = 3;
unsigned int assembling_line = 0;
unsigned int ORIGIN = 0xD1A881; //usermem
#define MAX_STACK 2048
int *DATA_STACK[MAX_STACK];
int DATA_SP = 0;
char *ErrorCode = 0;
int O_FILE_ORG;

include_entry_t *first_include;
include_entry_t *last_include;


void main(void){
	ti_var_t fp,fp2;
	char inFile[9];
	char outFile[9];
	string_t *ptr;
	uint8_t vt;

	ti_CloseAll();
	os_ClrHomeFull();
	if ((ptr = os_RclAns(&vt))&&vt==4){
		readTokens(&inFile,min(8,ptr->len),ptr->data);
		os_PutStrFull("Input file:");
		os_PutStrFull(&inFile);
		outFile[0] = 0x41;
		outFile[1] = 0;
		if (assemble(&inFile,&outFile)){
			if (!outFile[0]){
				error("No output program!","");
			} else {
				fp2 = ti_OpenVar(TEMP_FILE,"r",TI_TPRGM_TYPE);
				fp = ti_OpenVar(&outFile,"w",TI_PPRGM_TYPE);
				write(ti_GetDataPtr(fp2),ti_GetSize(fp2),fp);
				ti_Close(fp);
				ti_Close(fp2);
				print("Assembled Successfuly!");
				print("Output file:");
				print(&outFile);
			}
		} else {
			print("Abort.");
		}
	} else {
		print("Error: Could not open Ans");
	}
	pause();
	ti_CloseAll();
}

int assemble(const char *inFile, char *outFile){
	ti_var_t fp;
	uint8_t *ptr;
	uint8_t *max;
	uint8_t buffer[512];

	if (!(fp = ti_OpenVar(inFile,"r",TI_PRGM_TYPE))){
		return 0;
	}
	ptr = ti_GetDataPtr(fp);
	max = ptr+ti_GetSize(fp);
	ti_Close(fp);

	fp = ti_OpenVar(TEMP_FILE,"w",TI_TPRGM_TYPE);

	assembling_line = 1;
	while (ptr<max) {
		uint8_t c;
		uint8_t *edata;
		O_FILE_ORG = (int)ti_GetDataPtr(fp)-ti_Tell(fp);


		if (os_GetCSC()==sk_Clear){
			ErrorCode = "UserBreakError";
			break;
		}
		ErrorCode = 0;
		if (((uint8_t*)ptr)[0]==0x3F){
			ptr++;
		} else {
			ptr=readTokens(&buffer,min(511,max-ptr),ptr);
			upperCaseStr(&buffer);

			if (c = *buffer){
				if (c>=0x41 && c<=0x5A){
					if (edata = getEmitData(&buffer)){
						write(edata+1,edata[0],fp);
					} else if (edata = checkIncludes(&buffer)) {
						write(edata+1,edata[0],fp);
					} else {
						if (ErrorCode){
							error(ErrorCode,"");
						} else {
							error("Undefined word",trimWord(&buffer));
						}
						break;
					}
				} else if (c>=0x30 && c<=0x39) {
					if (edata=emitNumber(&buffer,1)){
						write(edata+1,edata[0],fp);
					}
				} else {
					error("Syntax",trimWord(&buffer));
					break;
				}
			} else {
				ptr++;
			}
		}
		assembling_line++;
	}
	if (!ErrorCode){
		ti_Rewind(fp);
		while (DATA_SP){
			markDefLabel((void*)(DATA_STACK[DATA_SP]),(int)ti_GetDataPtr(fp));
		}
	}

	ti_Close(fp);

	return !(int)ErrorCode;
}

void *readTokens(char *buffer,unsigned int amount,void *ptr){
	unsigned int str_len,i;
	char *str;
	uint8_t c;
	i=0;
	while (i<amount && ((uint8_t*)ptr)[0]!=0x3F){
		str = ti_GetTokenString(&ptr,0,&str_len);
		if (str_len){
			if ((i+str_len)>=amount){
				str_len = amount-i;
			}
			if (str_len){
				memcpy(buffer+i,str,str_len);
			}
			i+=str_len;
		}
	}
	buffer[i] = 0;
	return ptr;
}


/* byte (data+0): V&0x03 is number of bytes to emit. 
 *                V&0x80 marks value defined, otherwise the assembler needs to calculate it. 
 *                If that fails, then throw an undef'd word error.
 * word (data+1): offset in output file, to put value once computed.
 * long (data+3): stores computed value.
 * word (data+6): offset in source file, expression of value to compute.
*/

void markUndefLabel(const uint8_t *data){
	memcpy((DATA_STACK[DATA_SP++] = malloc(10)),data,10);
}

void markDefLabel(const uint8_t *data,int org){
	void *ptr = 0;
	if (data[0]&0x80){
		if (data[0]&0x03){
			memcpy(ptr,data+1,2);
			memcpy(ptr+O_FILE_ORG,data+3,data[0]&0x03);
			free(data);
		}
	} else {
		int val;
		memcpy(ptr,data+6,2);
		val = getNumber(&ptr);
		if (!ErrorCode){
			ptr=0;
			memcpy(ptr,data+1,2);
			memcpy(ptr+O_FILE_ORG,&val,data[0]&0x03);
			free(data);
		}
	}
}


uint8_t *getEmitData(const char *name){
	uint8_t c = name[0]-0x41;
	switch (c){
	case 0:
		return OpcodesA(name);
	case 1:
		return OpcodesB(name);
	case 2:
		return OpcodesC(name);
	case 3:
		return OpcodesD(name);
	case 4:
		return OpcodesE(name);
	case 5:
		return OpcodesF(name);
	case 8:
		return OpcodesI(name);
	case 9:
		return OpcodesJ(name);
	case 11:
		return OpcodesL(name);
	case 12:
		return OpcodesM(name);
	case 13:
		return OpcodesN(name);
	case 14:
		return OpcodesO(name);
	case 15:
		return OpcodesP(name);
	case 17:
		return OpcodesR(name);
	case 18:
		return OpcodesS(name);
	case 19:
		return OpcodesT(name);
	case 23:
		return OpcodesX(name);
	default:
		return 0;
	}
}

uint8_t *emitNumber(const char *name,uint8_t bytes){
	uint8_t buf[4];
	int number = getNumber(&name);
	memcpy(&buf[1],&number,3);
	buf[0] = bytes;
	return &buf;
}

uint8_t *searchIncludeFile(const char *fname, const char *cname){
	ti_var_t fp;
	uint8_t *ptr;
	char *nptr;
	int i,elen;
	if (fp = ti_Open(fname,"r")){
		ptr = ti_GetDataPtr(fp);
		ti_Close(fp);
		if (strcmp(ptr,"BASM3.0 INC")){
			return 0;
		}
	} else {
		return 0;
	}
	elen = (int)ptr[32];
	i = min(cname[0]-0x41,cname[0]-0x61);
	nptr = (char*)((int)ptr[i*3+35]);
	while (*nptr){
		
		nptr++;
	}
	return 0;
}

uint8_t *checkIncludes(const char *name){
	include_entry_t *ent;
	
	ent = first_include;
	do {
		uint8_t *rv;
		if (rv = searchIncludeFile(ent->fname,name)){
			return rv;
		}
	} while (ent = ent->next);
	return 0;
}

int includeFile(const char *fname){
	include_entry_t *ent;
	
	if (!(ent=malloc(11))){
		return 0;
	}

	memcpy(ent->fname,fname,8);
	last_include->next = ent;
	last_include = ent;
	ent->next = NULL;
	return (int)ent;
}

void error(const char *str,const char *word){
	char sbuf[80];
	ErrorCode = str;
	print(str);
	os_PutStrFull(" \"");
	os_PutStrFull(word);
	os_PutStrFull("\"");
	if (assembling_line){
		sprintf(&sbuf,"Error on line %d",assembling_line);
		print(&sbuf);
	}
}

char *trimWord(char *str){
	uint8_t *i;
	i = str;
	while (*i!=' ' && *i) i++;
	*i = 0;
	return str;
}

void print(const char *str){
	unsigned int row,col;
	os_NewLine();
	os_GetCursorPos(&row,&col);
	if (row>=9){
		os_SetCursorPos(0,9);
		os_PutStrFull("                          ");
		os_SetCursorPos(0,9);
	}
	os_PutStrFull(str);
}


void printX(const char *str,int amt){
	char *s = (char*)malloc(amt+1);
	memcpy(s,str,amt);
	s[amt] = 0;
	print(s);
	free(s);
}

void printAt(const char *str,uint8_t x,uint8_t y){
	os_SetCursorPos(y,x);
	os_PutStrFull(str);
}

void printXAt(const char *str,int amt,uint8_t x,uint8_t y){
	char *s = (char*)malloc(amt+1);
	memcpy(s,str,amt);
	s[amt] = 0;
	printAt(s,x,y);
	free(s);
}

char *upperCaseStr(char *str){
	char *ptr;
	unsigned char c;
	uint8_t a;
	ptr = str;
	while (c=*ptr++){
		a = min(c-0x41,c-0x61);
		if (a<26){
			*ptr = 0x41+a;
		}
	}
	return str;
}

sk_key_t pause(void){
	sk_key_t k;
	while (!(k=os_GetCSC()));
	return k;
}
