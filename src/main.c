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

typedef struct __include_entry_t {
	char fname[8];
	void *next;
} include_entry_t;

#define SIZEOF_LABEL_T 13
typedef struct _label_{
	char *name;
	int value,offset,org;
	uint8_t bytes;
}label_t;

const char *TEMP_FILE = "_BASMtmp";
const char *TEMP_FILE_2 = "_BASMtm2";
const char *MemoryError = "Insufficient Memory";

extern uint8_t buffer[];
uint8_t ADDR_BYTES = 3;
uint8_t CURRENT_BYTES;
uint8_t *O_FILE_PTR;
define_entry_t *internal_define_pointers[26];
char *LAST_LINE;
unsigned int assembling_line = 0;
unsigned int ORIGIN = 0xD1A881; //usermem
#define MAX_STACK 2048
int DATA_STACK[MAX_STACK];
label_t *WORD_STACK;
int DATA_SP = 0;
int WORD_SP;
char *ErrorCode = 0;
char *ErrorWord = 0;
int O_FILE_ORG,I_FILE_ORG,O_FILE_TELL;

ti_var_t gfp;

include_entry_t *first_include;
include_entry_t *last_include;


void *readTokens(uint8_t *buffer,unsigned int amount,void *ptr);
void setLabelOffset(const char *name,int value);
void setGotoOffset(const char *name);
void setLabelValue(const char *name,const uint8_t *value_ptr);
void setLabelValueValue(const char *name,int value);
void setGotoValue(const char *name,const uint8_t *value_ptr);
int getLabelValue(label_t *lbl);
int getLabelOffset(const char *name);
label_t *findLabel(const char *name);
label_t *findGoto(const char *name);
void defineLabel(uint8_t *name,int val);
void defineGoto(uint8_t *name,int val);
void UpdateWordStack(void);
int assemble(const char *inFile, char *outFile);
uint8_t *getEmitData(const char *name);
uint8_t *checkIncludes(const char *name);
uint8_t *searchIncludeFile(const char *fname, const char *cname);
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
		printAt("Input file:",0,0);
		os_PutStrFull(&inFile);
		outFile[0] = 0x41;
		outFile[1] = 0;
		if (fp2=ti_Open("BASMdata","r")){
			uint8_t l;
			uint8_t *org = ti_GetDataPtr(fp2);
			if (strcmp(org,"BASM3-OPCODES")){
				printAt("Malformed BASMdata.8xv",0,0);
				ti_Close(fp2);
			} else {
				uint16_t *ptr = (uint16_t*)(org+32);
				for (l=0;l<26;l++){
					internal_define_pointers[l] = (define_entry_t*)(org + *ptr++);
				}
				ti_Close(fp2);
				if (!outFile[0]){
					error("No output program!","");
				} else {
					if (assemble(&inFile,&outFile)){
						fp2 = ti_OpenVar(TEMP_FILE,"r",TI_TPRGM_TYPE);
						fp = ti_OpenVar(&outFile,"w",TI_PPRGM_TYPE);
						write(ti_GetDataPtr(fp2),ti_GetSize(fp2),fp);
						ti_Close(fp);
						ti_Close(fp2);
						printAt("Assembled Successfuly!",0,4);
						printAt("Output file:",0,5);
						printAt(&outFile,12,5);
					} else {
						printAt("Aborted.",0,8);
					}
				}
			}
		} else {
			printAt("Missing BASMdata.8xv",0,0);
		}
	} else {
		printAt("Error: Could not open Ans",0,0);
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
	while (ptr<max){
		ErrorCode = 0;
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
			if (*ptr==0x3F){
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
							char c;
							int i = 0;
							while ((c=buf[i++])>0x40 && c<=0x5A);
							if (c==':'){
								buf[i-1]=0;
								if (buf[i]=='='){
									setLabelValue(&buf,buf+i+1);
								} else if (buf[i]=='+'){
									setLabelValueValue(&buf,ORIGIN+ti_Tell(fp)+getNumberWrapper(&ptr));
								} else {
									setLabelValueValue(&buf,ORIGIN+ti_Tell(fp));
								}
							} else {
								if (!ErrorCode) ErrorCode = "Undefined word";
							}
						}
					} else {
						if (!ErrorCode) ErrorCode = "Syntax";
					}
				} else {
					ptr++;
				}
				sprintf(&buffer,"line %d",assembling_line);
				printAt(&buffer,14,9);
				if (ErrorCode){
					if (ErrorWord){
						error(ErrorCode,ErrorWord);
					} else {
						error(ErrorCode,&buf);
					}
					break;
				}
				assembling_line++;
			}
		}
		if (!ErrorCode){
			assembling_line = 0;
			printAt("Filling addresses...     ",0,9);
			i = WORD_SP;
			do {
				label_t *lbl;
				label_t *gt = &WORD_STACK[i];
				if (gt->bytes&3){ //set address for this item.
					uint8_t c;
					uint8_t *ptr;
					int val;
					lbl = findLabel(gt->name);
					val = getLabelValue(lbl);
					ptr = (uint8_t*)gt->offset-gt->org;
					if (c==0xCB && *ptr>=0x40){
						uint8_t bitn;
						void *numptr = (void*)gt->value;
						if ((bitn=getNumber(&numptr))>=8){
							ErrorCode = "Argument out of range";
							break;
						}
						*ptr = (*ptr&0xC7)|(bitn<<3);
					} else {
						if (!((c=*ptr++)&0xC7) && c&0x38){
							val = gt->offset - val;
						} else if (c==0xED||c==0xDD||c==0xFD){
							c = *ptr++;
						}
						ti_Seek((int)ptr,SEEK_SET,fp);
						ti_Write(&val,gt->bytes&3,1,fp);
					}
				}
			} while (i--);
			if (ErrorCode){
				error(ErrorCode,ErrorWord);
			}
		}
		ti_Close(fp);
	}
	ti_Close(gfp);

	return !(int)ErrorCode;
}

void *readTokens(uint8_t *buffer,unsigned int amount,void *ptr){
	unsigned int str_len,i;
	char *str;
	uint8_t c;
	i=0;
	memset(buffer,0,amount+1);
	while (i<amount && (*(uint8_t*)ptr)!=0x3F){
		if (str = ti_GetTokenString(&ptr,0,&str_len)){
			if (str_len){
				if ((i+str_len)>=amount){
					str_len = amount-i;
				}
				if (str_len){
					memcpy(buffer+i,str,str_len);
				}
				i+=str_len;
			}
		} else {
			break;
		}
	}
	return ptr;
}

void setGotoOffset(const char *name){ //set the output offset of an address that needs to be filled in later.
	label_t *data;
	if (data=findGoto(name)){
		int ptr = ORIGIN + O_FILE_TELL;
		data->bytes=CURRENT_BYTES; //number of bytes needing to be filled
		data->offset = ptr; //offset in output file
	}
}

void setGotoValue(const char *name,const uint8_t *value_ptr){
	label_t *data;
	if (data=findGoto(name)){
		int val;
		data->bytes=CURRENT_BYTES; //number of bytes needing to be filled
		val=getNumberWrapper(&value_ptr);
		data->value = val; //value of address
		memcpy((void*)O_FILE_ORG+data->offset,&val,CURRENT_BYTES); //set the address in the output file
	}
}

label_t *findLabel(const char *name){
	label_t *data;
	int i = WORD_SP;
	do {
		data = &WORD_STACK[i];
		if (!data->bytes){
			if (!strcmp(data->name,name)){
				return data;
			}
		}
	} while (i--);
	return 0;
}

label_t *findGoto(const char *name){
	label_t *data;
	int i = WORD_SP;
	do {
		data = &WORD_STACK[i];
		if (data->bytes){
			if (!strcmp(data->name,name)){
				return data;
			}
		}
	} while (i--);
	return 0;
}

int getLabelValue(label_t *lbl){ //get the value of a label
	if (lbl->bytes&0x80){
		void *ptr = (void*)lbl->value;
		return getNumberWrapper(&ptr); //return the computed value
	} else {
		return lbl->value; //return the value
	}
	return 0;
}

int getLabelOffset(const char *name){ //get the offset of a label
	label_t *data;
	if (data=findLabel(name)){
		return data->offset; //return the value
	}
	return 0;
}

void setLabelOffset(const uint8_t *name,int value){ //set a label's offset from the file origin.
	label_t *data;
	if (data=findLabel(name)){
		memcpy(&data->offset,&value,3); //set its offset (from the file origin, hopefuly)
	}
}

void setLabelValue(const uint8_t *name,const uint8_t *value_ptr){
	label_t *data;
	if (data=findLabel(name)){
		memcpy(&data->value,&value_ptr,3); //set the pointer to it's value's expression
		data->bytes|=0x80;
	}
}

void setLabelValueValue(const uint8_t *name,int value){
	label_t *data;
	if (data=findLabel(name)){
		memcpy(&data->value,&value,3);
		data->bytes&=0x7F;
	}
}

void defineLabel(const char *name,int val){ //write a new label
	ti_Seek(0,SEEK_END,gfp);
	ti_Write(&name,3,1,gfp); //write a pointer to the name of the label.
	ti_Write(&val,3,1,gfp); //write the value of the label.
	ti_Write("\xFF\xFF\xFF",3,1,gfp); //write 0xFFFFFF for the file offset.
	ti_Write(&ORIGIN,3,1,gfp); //write the label's origin pointer
	ti_PutC(0,gfp); //write 0 to denote that this is a label.
	UpdateWordStack();
}

void defineGoto(const char *name,int val){ //write a new goto
	ti_Seek(0,SEEK_END,gfp);
	ti_Write(&name,3,1,gfp); //write a pointer to the name of the label to goto.
	ti_Write(&val,3,1,gfp); //write the value of the label to goto.
	ti_Write(&O_FILE_TELL,3,1,gfp); //write file offset in output file
	ti_Write(&ORIGIN,3,1,gfp); //write the goto's origin pointer
	ti_PutC(0x7F,gfp); //and 0x7F for the length. (to be modified later)
	UpdateWordStack();
}

uint8_t *getEmitData(const char *name){ //opcodes and built-in words
	uint8_t *data;
	define_entry_t *ptr;
	if (data=checkInternal(name,&ptr)){
		return data;
	} else if (data=processOpcodeLine(name)){
		while (ptr->bytes){
			if (!strncmp(data,&ptr->opcode,12)){
				uint8_t *buf4;
				if (buf4=malloc(8)){
					int i = (ptr->flags&7) + 1;
					memcpy(buf4,&ptr->bytes,ptr->bytes+1);
					emitArgument(buf4+i,name,ptr->flags);
					if (ptr->flags&F_LONG_ARG&&ADDR_BYTES!=2) buf4[0]++;
					free(data);
					return buf4;
				} else {
					ErrorCode = MemoryError;
				}
			}
			ptr++;
		}
		ErrorWord = data;
	} else {
		ErrorCode = MemoryError;
	}
	return 0;
}

uint8_t *searchIncludeFile(const char *fname, const char *cname){
	ti_var_t fp;
	uint8_t *ptr;
	int i,elen;
	if (fp = ti_Open(fname,"r")){
		ptr = ti_GetDataPtr(fp);
		ti_Close(fp);
		if (strcmp(ptr,"BASM3.0 INC")) return 0;
	} else return 0;
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

void UpdateWordStack(void){
	ti_Rewind(gfp);
	WORD_STACK = ti_GetDataPtr(gfp);
	WORD_SP = ti_GetSize(gfp) / SIZEOF_LABEL_T;
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
	printAt(str,0,1);
	if (*word){
		printAt(" \"",0,2);
		os_PutStrFull(word);
		os_PutStrFull("\"");
	}
	if (assembling_line){
		sprintf(&sbuf,"Error on line %d",assembling_line);
		printAt(&sbuf,0,3);
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
