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
void setLabelOffset(const uint8_t *name,int value);
void setGotoOffset(const char *name);
void setLabelValue(const uint8_t *name,int val);
int getLabelValue(const char *name);
int getLabelOffset(const char *name);
void defineLabel(uint8_t *name,int val);
void defineGoto(uint8_t *name,int val);
void UpdateWordStack(void);
int assemble(const char *inFile, char *outFile);
uint8_t *getEmitData(const char *name);
uint8_t *checkIncludes(const char *name);
uint8_t *searchIncludeFile(const char *fname, const char *cname);
uint8_t *checkWordStack(const char *name);
int includeFile(const char *fname);

void error(const char *str,const char *word);
void print(const char *str);
void printX(const char *str,int amt);
void printAt(const char *str,uint8_t x,uint8_t y);
void printXAt(const char *str,int amt,uint8_t x,uint8_t y);
char *trimWord(char *str);
char *upperCaseStr(char *str);
sk_key_t pause(void);

extern int isNumber(const char *line);


typedef struct __include_entry_t {
	char fname[8];
	void *next;
} include_entry_t;


const char *TEMP_FILE = "_BASMtmp";
const char *TEMP_FILE_2 = "_BASMtm2";
const char *MemoryError = "Insufficient Memory";

extern uint8_t buffer[];
uint8_t ADDR_BYTES = 3;
uint8_t CURRENT_BYTES;
uint8_t *O_FILE_PTR;
uint8_t ALLOWLABELS = 1;
char *LAST_LINE;
unsigned int assembling_line = 0;
unsigned int ORIGIN = 0xD1A881; //usermem
#define MAX_STACK 2048
int DATA_STACK[MAX_STACK];
uint8_t *WORD_STACK;
int DATA_SP = 0;
int WORD_SP;
char *ErrorCode = 0;
int O_FILE_ORG,I_FILE_ORG,O_FILE_TELL;

ti_var_t gfp;

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
				printAt("Assembled Successfuly!",0,4);
				printAt("Output file:",0,5);
				printAt(&outFile,12,5);
			}
		} else {
			printAt("Aborted.",0,8);
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
	uint8_t buf[512];
	int i;

	if (!(fp = ti_OpenVar(inFile,"r",TI_PRGM_TYPE))){
		return 0;
	}
	I_FILE_ORG = (int)(ptr = ti_GetDataPtr(fp));
	max = ptr+ti_GetSize(fp);
	ti_Close(fp);


	printAt("Prescanning...",0,9);
	gfp = ti_OpenVar(TEMP_FILE_2,"w",TI_TPRGM_TYPE);
	ErrorCode = 0;
	while (ptr<max){
		if (os_GetCSC()==sk_Clear){
			ErrorCode = "UserBreakError";
			break;
		} else if (*((uint8_t*)ptr)==0x3F){
			ptr++;
		} else {
			uint8_t c;
			int i=0;
			ptr=readTokens(&buf,min(511,max-ptr),ptr);
			upperCaseStr(&buf);
			i = strlen(&buf)-1;
			if (buf[i]==':'){
				uint8_t *data;
				if (data = malloc(i+1)){
					memcpy(data,&buf,i);
					data[i]=0;
					defineLabel(data,0);
				} else {
					ErrorCode = MemoryError;
					break;
				}
			}
		}
	}
	if (!ErrorCode){
		UpdateWordStack();
		printAt("Assembling...  ",0,9);

		fp = ti_OpenVar(TEMP_FILE,"w",TI_TPRGM_TYPE);
		ptr = (uint8_t*)I_FILE_ORG;
		assembling_line = 1;
		while (ptr<max) {
			uint8_t c;
			uint8_t *edata;
			O_FILE_ORG = (int)(O_FILE_PTR=ti_GetDataPtr(fp))-(O_FILE_TELL=ti_Tell(fp));


			if (os_GetCSC()==sk_Clear){
				ErrorCode = "UserBreakError";
				break;
			}
			ErrorCode = 0;
			if (((uint8_t*)ptr)[0]==0x3F){
				ptr++;
			} else {
				ptr=readTokens(&buf,min(511,max-ptr),ptr);
				upperCaseStr(&buf);

				if (c = *buf){
					if (c>=0x41 && c<=0x5A){
						if (edata = getEmitData(&buf)){
							write(edata+1,edata[0],fp);
						} else if (edata = checkIncludes(&buf)) {
							write(edata+1,edata[0],fp);
						} else {
							if (ErrorCode){
								error(ErrorCode,"");
								break;
							} else if (edata = checkWordStack(&buf)){
								write(edata+1,edata[0],fp);
							} else {
								int i = 0;
								i = strlen(&buf)-1;
								if (buf[i]==':'){
									buf[i]=0; i++;
									if (buf[i]=='='){
										uint8_t *ptr = &buf[i+1];
										setLabelValue(&buf,getNumberWrapper(&ptr));
									} else if (buf[i]=='+'){
										uint8_t *ptr = &buf[i+1];
										setLabelOffset(&buf,ORIGIN+ti_Tell(fp)+getNumberWrapper(&ptr));
									} else {
										setLabelOffset(&buf,ORIGIN+ti_Tell(fp));
									}
								} else {
									ErrorCode = "Undefined word";
									break;
								}
							}
						}
					} else {
						error("Syntax",trimWord(&buf));
						break;
					}
				} else {
					ptr++;
				}
			}
			sprintf(&buffer,"line %d",assembling_line);
			printAt(&buffer,14,9);
			assembling_line++;
		}
		if (!ErrorCode){
			printAt("Filling addresses...     ",0,9);
			i = WORD_SP;
			do {
				if (WORD_STACK[i+8]){ //set address for this item.
					uint8_t *ptr;
					uint8_t c;
					uint16_t num;
					int val = getLabelOffset((uint8_t*)WORD_STACK+i); //get the offset of the label with the same name
					if (WORD_STACK[i+8] && (num = *(uint16_t*)(&WORD_STACK[i+6]))==0xFFFF){
						ErrorCode = "Undefined Label";
						break;
					}
					ptr = (uint8_t*)O_FILE_ORG + num;
					if ((c=*ptr++)==0xED||c==0xCB||c==0xDD||c==0xFD){
						ptr++;
					} else if (c==0x10||c==0x18||c==0x20||c==0x28||c==0x30||c==0x38){
						int i2 = WORD_SP;
						do {
							if (WORD_STACK[i2+8]){
								if (!strcmp((uint8_t*)WORD_STACK+i,(uint8_t*)WORD_STACK+i2)){
									val = *((uint16_t*)WORD_STACK+i2+3) - val;
									break;
								}
							}
						} while (i2-=9);
					} else {
						val = getLabelValue((uint8_t*)WORD_STACK+i);
					}
					memcpy(ptr,&val,WORD_STACK[i+8]);
				}
			} while (i-=9);
		}
	}
	ti_Close(fp);
	ti_Close(gfp);

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

void setGotoOffset(const char *name){ //set the output offset of an address that needs to be filled in later.
	uint8_t *data;
	int i = WORD_SP;
	do {
		data = WORD_STACK+i;
		if (data[8]){ //mark an address pointer that needs to be filled in later
			if (!strcmp(((char*)data),name)){
				data[8]=CURRENT_BYTES; //number of bytes needing to be filled
				memcpy(&data+6,&O_FILE_TELL,2); //offset in output file
			}
		}
	} while (i-=9);
}

int getLabelValue(const char *name){ //get the value of a label
	uint8_t *data;
	int i = WORD_SP;
	do {
		data = WORD_STACK+i;
		if (!data[8]){ //this is a label.
			if (!strcmp(((char*)data),name)){ //same name
				return *(int*)(&data[3]); //return the value
			}
		}
	} while (i-=9);
	return 0;
}

int getLabelOffset(const char *name){ //get the offset of a label
	uint8_t *data;
	int i = WORD_SP;
	do {
		data = WORD_STACK+i;
		if (!data[8]){ //this is a label.
			if (!strcmp(((char*)data),name)){ //same name
				return *(uint16_t*)(&data[6]); //return the value
			}
		}
	} while (i-=9);
	return 0;
}

void setLabelOffset(const uint8_t *name,int value){ //set a label's offset from the file origin.
	uint8_t *data;
	int i = WORD_SP;
	do {
		data = WORD_STACK+i;
		if (!data[8]){ //this is a label.
			if (!strcmp(((char*)data),name)){ //same name
				memcpy(&data+6,&value,2); //set its offset (from the file origin, hopefuly)
			}
		}
	} while (i-=9);
}

void setLabelValue(const uint8_t *name,int value){
	uint8_t *data;
	int i = WORD_SP;
	do {
		data = WORD_STACK+i;
		if (!data[8]){ //this is a label.
			if (!strcmp(((char*)data),name)){ //same name
				memcpy(&data+3,&value,3); //set it's value
			}
		}
	} while (i-=9);
}

void defineLabel(uint8_t *name,int val){ //write a new label
	ti_Seek(0,SEEK_END,gfp);
	ti_Write(&name,3,1,gfp); //write a pointer to the name of the label.
	ti_Write(&val,3,1,gfp); //write the value of the label.
	ti_Write("\xFF\xFF\0",3,1,gfp); //write 0xFFFF for the offset, and 0 for the length.
	UpdateWordStack();
}

void defineGoto(uint8_t *name,int val){ //write a new goto
	ti_Seek(0,SEEK_END,gfp);
	ti_Write(&name,3,1,gfp); //write a pointer to the name of the label to goto.
	ti_Write(&val,3,1,gfp); //write the value of the label to goto.
	ti_Write("\xFF\xFF\xFF",3,1,gfp); //write 0xFFFF for my offset, and 0xFF for the length. (to be modified in post-comp)
	UpdateWordStack();
}


uint8_t *getEmitData(const char *name){ //opcodes and built-in words
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

uint8_t *searchIncludeFile(const char *fname, const char *cname){
	ti_var_t fp;
	uint8_t *ptr;
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
	ptr+=((uint16_t)ptr[i*2+35]);
	while (*ptr){
		if (!strncmp(cname,ptr,elen)){
			return ptr+strnlen(ptr,elen);
		} else {
			ptr+=strnlen(ptr,elen)+4;
		}
	}
	return 0;
}

uint8_t *checkWordStack(const char *name){
	int i = WORD_SP;
	do {
		if (WORD_STACK[i+8]){
			if (!strcmp((uint8_t*)(WORD_STACK+i),name)){
				uint8_t buf[4] = {3};
				memcpy(&buf[1],WORD_STACK+i+3,3);
				return &buf;
			}
		}
	} while (i-=9);
	return 0;
}

void UpdateWordStack(void){
	ti_Rewind(gfp);
	WORD_STACK = ti_GetDataPtr(gfp);
	WORD_SP = ti_GetSize(gfp);
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
