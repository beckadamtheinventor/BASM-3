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
#define isAlphaNumeric(c) ((unsigned)(c-0x41)<26||(unsigned)(c-0x30)<10||c=='.')

#define SIZEOF_INCLUDE_ENTRY 11
typedef struct __include_entry_t {
	char fname[8];
	void *next;
} include_entry_t;


const char *TEMP_FILE = "_BASMtmp";
const char *TEMP_FILE_2 = "_BASMtm2";
const char *MemoryError = "Insufficient Memory";
const char *UserBreakError = "Cancel. Abort.";
const char *UserBreakWord = "\\o/";

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
unsigned int O_FILE_ORG,I_FILE_ORG,O_FILE_TELL,O_FILE_SIZE;

ti_var_t gfp;

include_entry_t first_include;
include_entry_t *last_include;


void *readTokens(uint8_t *buffer,unsigned int amount,void *ptr,void *max);
void parseLabel(char *buf,ti_var_t fp);
void writeArgs(char *buf,int len,ti_var_t fp);
void setLabelOffset(const char *name,int value);
void setLabelValue(const char *name,const char *value_ptr);
void setLabelValueValue(const char *name,int value);
int getLabelValue(label_t *lbl);
int getLabelOffset(const char *name);
label_t *findLabel(const char *name);
label_t *findGoto(const char *name);
void defineLabel(const char *name,int val);
void defineGoto(void *val,int offset);
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
	char inFile[10];
	char outFile[11];
	string_t *ptr;
	uint8_t vt;

	ti_CloseAll();
	os_ClrHomeFull();
	if ((ptr = os_RclAns(&vt))&&vt==4){
		readTokens(&inFile,8,ptr->data,ptr->data+ptr->len);
		printAt("Input file:",0,0);
		os_PutStrFull(&inFile);
		memset(&outFile,0,11);
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
				if (assemble(&inFile,&outFile)){
					if (outFile[0]&&outFile[9]){
						fp2 = ti_OpenVar(TEMP_FILE,"r",TI_TPRGM_TYPE);
						fp = ti_OpenVar(&outFile,"w",outFile[9]);
						ti_Write(ti_GetDataPtr(fp2),O_FILE_SIZE,1,fp);
						if (outFile[10]&1){
							ti_SetArchiveStatus(1,fp);
						}
						ti_Close(fp);
						ti_Close(fp2);
						printAt("Assembled Successfuly!",0,4);
						printAt("Output file:",0,5);
						printAt(&outFile,12,5);
					} else {
						printAt("Output file not specified.",0,8);
					}
				} else {
					error(ErrorCode,ErrorWord);
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
		ErrorCode = "Input file not found";
		ErrorWord = inFile;
		return 0;
	}
	I_FILE_ORG = (int)(ptr = ti_GetDataPtr(fp));
	max = ptr+ti_GetSize(fp);
	ti_Close(fp);



	printAt("Prescanning...",0,9);
	gfp = ti_OpenVar(TEMP_FILE_2,"w",TI_TPRGM_TYPE);
	while (ptr<max){
		uint8_t c;
		ErrorCode = 0;
		if (os_GetCSC()==sk_Clear){
			ErrorCode = UserBreakError;
			ErrorWord = UserBreakWord;
			break;
		} else if ((c=*((uint8_t*)ptr))==tEnter){
			ptr++;
		} else {
			uint8_t c;
			uint8_t *line;
			int i=0;
			ptr=readTokens(&buf,511,ptr,max);
			upperCaseStr(&buf);
			line = &buf;
			if (!strncmp(line,"LBL ",4)){
				line+=4;
			}
			while ((c=line[++i])&&c!=':'); //skip until ':'
			if (c==':'){
				uint8_t *data;
				int len;
				ErrorWord=0;
				len = strlen(line);
				line[i++]=0;
				if (data = malloc(len)){
					int val;
					memcpy(data,line,len);
					defineLabel(data,0);
					if (data[i]=='='){
						setLabelValue(data,ErrorWord=data+i+1);
					}
				} else {
					ErrorCode = MemoryError;
					break;
				}
			}
		}
	}
	if (!ErrorCode){
		uint8_t c;
		UpdateWordStack();
		printAt("Assembling...  ",0,9);

		fp = ti_OpenVar(TEMP_FILE,"w",TI_TPRGM_TYPE);
		ti_Resize(0xF000,fp);
		ptr = (uint8_t*)I_FILE_ORG;
		assembling_line = 1;
		while (ptr<max) {
			uint8_t *edata;
			O_FILE_ORG = (int)(O_FILE_PTR=ti_GetDataPtr(fp))-(O_FILE_TELL=ti_Tell(fp));


			if (os_GetCSC()==sk_Clear){
				ErrorCode = UserBreakError;
				ErrorWord = UserBreakWord;
				break;
			}
			ErrorCode = 0;
			if (*ptr==tEnter){
				ptr++;
			} else {
				ptr=readTokens(&buf,511,ptr,max);
				if (*buf){
					if ((unsigned)(buf[0]-0x61)<26) buf[0]-=0x20;
					if ((unsigned)(buf[1]-0x61)<26) buf[1]-=0x20;
					if (!strncmp(&buf,"DB ",3)){
						writeArgs(&buf[3],1,fp);
					} else if (!strncmp(&buf,"DW ",3)){
						writeArgs(&buf[3],2,fp);
					} else if (!strncmp(&buf,"DL ",3)){
						writeArgs(&buf[3],3,fp);
					} else {
						uint8_t *line;
						upperCaseStr(&buf[2]);
						if ((unsigned)((c=*buf)-0x41)<26||c==tTheta){
							if (!strncmp(&buf,"FORMAT ",7)){
								line = &buf[7];
								do {
									if (!strncmp(line,"ASM",3)){
										ti_Write("\xEF\x7B",2,1,fp);
										ORIGIN = 0xD1A87F;
										outFile[9]=0x06;
										line+=3;
									} else if (!strncmp(line,"ARCHIVED",8)){
										outFile[10]|=1;
										line+=8;
									} else if (*line=='"'){
										int len = 0;
										line++;
										do c=line[len++]; while (c&&c!='"'); len--;
										if (!len){
											ErrorCode = "Output name can't be empty";
											ErrorWord = line-1;
											break;
										} else {
											if (len>8){
												ErrorCode = "Output name too long";
												ErrorWord = line-1;
												break;
											}
											memcpy(outFile,line,len);
											outFile[len]=0;
										}
										line+=len+1;
									} else {
										ErrorCode = "Invalid format directive";
										ErrorWord = line;
										break;
									}
								} while (*line++);
								if (ErrorCode) break;
							} else if (!strncmp(&buf,"INCLUDE ",8)){
								line = &buf[8];
								if (*line=='"'){
									int len = 0;
									line++;
									do c=line[len++]; while (c&&c!='"'); len--;
									if (len){
										if (len>8){
											ErrorCode = "Appvar name too long";
										}
										line[len]=0;
										includeFile(line);
										line+=len+1;
									} else {
										ErrorCode = "Null appvar name";
									}
								} else {
									ErrorCode = "Invalid argument";
								}
							} else if (!strncmp(&buf,"LBL ",4)){
								parseLabel(&buf[4],fp);
								ErrorCode = 0;
							} else if (edata = getEmitData(&buf)){
								ti_Write(edata+1,edata[0],1,fp);
							} else if (edata = checkIncludes(&buf)) {
								ti_Write(edata+1,edata[0],1,fp);
							} else {
								parseLabel(&buf,fp);
							}
						} else if (!strncmp(&buf,"//",2)){
							continue;
						} else {
							if (!ErrorCode) ErrorCode = "Syntax";
						}
					}
				} else {
					ptr++;
				}
				sprintf(&buffer,"line %d",assembling_line);
				printAt(&buffer,14,9);
				if (ErrorCode) {
					if (!ErrorWord) ErrorWord = &buf;
					break;
				}
				assembling_line++;
			}
		}
		if (!ErrorCode){
			int remainingTries;
			bool remainingUndefs = 0;
			remainingTries = WORD_SP+1;
			//ti_Resize(ti_Tell(fp),fp); //this bugs out for some reason
			O_FILE_SIZE = ti_Tell(fp);
			assembling_line = 0;
			printAt("Filling addresses...     ",0,9);
			do {
				label_t *gt;
				i = WORD_SP;
				remainingUndefs = 0;
				while (i--) {
					gt = &WORD_STACK[i];
					if (gt->bytes&3){ //set address for this item.
						uint8_t c;
						uint8_t *ptr;
						int val = getLabelValue(gt); //get the goto's value
						if (ErrorCode==(char*)1){ //there was an undefined label in the expression
							ErrorCode=0;
							remainingUndefs = 1;
						} else {
							if (gt->bytes&8){
								val -= gt->org+gt->offset+(gt->bytes&7); //calculate the JR offset byte
								if ((unsigned)(val+0x80)>0xFF){
									ErrorCode = "JR offset out of range";
									ErrorWord = (char*)gt->value;
									break;
								}
							}
							ti_Seek(gt->offset,SEEK_SET,fp); //seek to the file offset where the data needs to go
							ti_Write(&val,gt->bytes&3,1,fp); //write in the data
						}
					} else { //try to set its value
						if (gt->bytes&0x80){
							int val = getLabelValue(gt);
							if (ErrorCode==(char*)1){
								ErrorCode=0;
							} else {
								gt->bytes&=0x3F;
								gt->value = val;
							}
						}
					}
				}
				if (!(remainingTries--)) {
					ErrorCode = "Can't get label value";
				}
				if (ErrorCode) break;
			} while (remainingUndefs);
		}
		ti_Close(fp);
	}
	ti_Close(gfp);

	return !(int)ErrorCode;
}

void parseLabel(char *buf,ti_var_t fp){
	char c;
	int i=0;
	while ((c=buf[++i])&&c!=':'); //skip until the ':'
	if (c){
		uint8_t *ptr2;
		int val,offset;
		buf[i]=0;
		ErrorWord = ptr2 = buf+i+2;
		offset = ORIGIN+ti_Tell(fp);
		if ((c=buf[i+1])=='='){
			int len2;
			uint8_t *ptr3;
			len2 = strlen(ptr2)+1;
			if (ptr3=malloc(len2)){
				memcpy(ptr3,ptr2,len2);
				setLabelValue(buf,ptr3);
			} else {
				ErrorCode = MemoryError;
			}
		} else if (c=='+'){
			setLabelValueValue(buf,offset+getNumber(&ptr2,offset,0));
		} else if (c=='-'){
			setLabelValueValue(buf,offset-getNumber(&ptr2,offset,0));
		} else {
			setLabelValueValue(buf,offset);
			ErrorWord = 0;
		}
	} else {
		if (!ErrorCode) {
			ErrorCode = "Undefined word";
			ErrorWord = buf;
		}
	}
}

void *readTokens(uint8_t *buffer,unsigned int amount,void *ptr,void *max){
	unsigned int str_len,i;
	char *str;
	uint8_t c;
	i=0;
	memset(buffer,0,amount+1);
	while (i<amount && ptr<max && (*(uint8_t*)ptr)!=0x3F){
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

void writeArgs(char *buf,int len,ti_var_t fp){
	int num;
	char c;
	CURRENT_BYTES = len+4;
	while (c=*buf++){
		if (c=='"'){
			while ((c=*buf++)&&c!='"') {
				ti_PutC(c,fp);
			}
			buf++;
		} else {
			buf--;
			num = getNumber(&buf,0,0);
			ti_Write(&num,len,1,fp);
			buf++;
		}
	}
}

label_t *findLabel(const char *name){
	label_t *data;
	int i = WORD_SP;
	while (i--) {
		data = &WORD_STACK[i];
		if (!(data->bytes&3)){
			if (!strcmp(data->name,name)){
				return data;
			}
		}
	}
	return 0;
}

label_t *findGoto(const char *name){
	label_t *data;
	int i = WORD_SP;
	while (i--) {
		data = &WORD_STACK[i];
		if (data->bytes&3){
			if (!strcmp(data->name,name)){
				return data;
			}
		}
	}
	return 0;
}

int getLabelValue(label_t *lbl){ //get the value of a label
	int val = 0;
	if (lbl->bytes&0x80){
		void *ptr = (void*)lbl->value;
		val=getNumber(&ptr,0,0); //return the computed value
	} else {
		val=lbl->value;
	}
	return val;
}

int getLabelOffset(const char *name){ //get the offset of a label
	label_t *data;
	if (data=findLabel(name)){
		return data->offset; //return the value
	}
	return 0;
}

void setLabelOffset(const char *name,int value){ //set a label's offset from the file origin.
	label_t *data;
	if (data=findLabel(name)){
		memcpy(&data->offset,&value,3); //set its offset (from the file origin, hopefuly)
	}
}

void setLabelValue(const char *name,const char *value_ptr){
	label_t *data;
	if (data=findLabel(name)){
		data->value = (int)value_ptr; //set the pointer to it's value's expression
		data->bytes |= 0x80;
	}
}

void setLabelValueValue(const char *name,int value){
	label_t *data;
	if (data=findLabel(name)){
		data->value = value;
		data->bytes &= 0x3F;
	}
}

void defineLabel(const char *name,int val){ //write a new label
	ti_Seek(0,SEEK_END,gfp);
	ti_Write(&name,3,1,gfp); //write a pointer to the name of the label.
	ti_Write(&val,3,1,gfp); //write the value of the label.
	ti_Write("\xFF\xFF\xFF",3,1,gfp); //write 0xFFFFFF for the file offset.
	ti_Write(&ORIGIN,3,1,gfp); //write the label's origin pointer
	ti_PutC(0x40,gfp); //write 0x40 to denote that this is a label and is undefined.
	UpdateWordStack();
}

void defineGoto(void *val,int offset){ //write a new 'goto'. Basically a place to put an address.
	offset+=O_FILE_TELL;
	ti_Seek(0,SEEK_END,gfp);
	ti_Write((void*)0xFF0000,3,1,gfp); //gotos don't have names anymore
	ti_Write(&val,3,1,gfp); //write the value of the label to goto.
	ti_Write(&offset,3,1,gfp); //write file offset in output file
	ti_Write(&ORIGIN,3,1,gfp); //write the goto's origin pointer
	ti_PutC(CURRENT_BYTES|0x80,gfp); //CURRENT_BYTES -> length of data, 0x80 -> value is pointer.
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
				free(data);
				if (buf4=malloc(8)){
					int i = ptr->flags&7;
					memcpy(buf4,&ptr->bytes,(ptr->bytes&7)+1);
					if (ptr->flags&F_JR_ARG){
						CURRENT_BYTES=8;
					} else {
						CURRENT_BYTES=0;
					}
					if (ptr->flags&(F_OFFSET_ARG|F_BYTE_ARG)){
						CURRENT_BYTES|=1;
						defineGoto(getArgFromLine(name),i);
					} else if (ptr->flags&F_LONG_ARG){
						int num;
						CURRENT_BYTES|=ADDR_BYTES;
						defineGoto(getArgFromLine(name),i);
						if (ADDR_BYTES!=2) buf4[0]++;
					}
					return buf4;
				} else {
					ErrorCode = MemoryError;
					break;
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
	unsigned int i,elen;
	if (fp = ti_Open(fname,"r")){
		ptr = ti_GetDataPtr(fp);
		ti_Close(fp);
		if (strcmp(ptr,"BASM3.0 INC")) return 0;
	} else return 0;
	elen = (uint8_t)ptr[32];
	if (cname[0]==tTheta){
		i = 27;
	} else {
		i = cname[0]-0x41;
	}
	ptr+=((uint16_t)ptr[i*2+33]);
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

	ent = &first_include;
	do {
		uint8_t *rv;
		if (rv = searchIncludeFile(ent->fname,name)){
			return rv;
		}
	} while (ent = ent->next);
	return 0;
}

int includeFile(const char *fname){
	ti_var_t fp;
	include_entry_t *ent;
	if (fp=ti_Open(fname,"r")){
		ti_Close(fp);
	} else {
		ErrorCode = "Include appvar not found";
		return 0;
	}

	if (!(ent=malloc(SIZEOF_INCLUDE_ENTRY))){
		ErrorCode = MemoryError;
		return 0;
	}

	memcpy(ent->fname,fname,8);
	if (last_include){
		last_include->next = ent;
	}
	last_include = ent;
	ent->next = 0;
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
	ptr = str;
	while (c=*ptr){
		if ((unsigned)(c-0x61)<26){
			*ptr -= 0x20;
		}
		ptr++;
	}
	return str;
}

sk_key_t pause(void){
	sk_key_t k;
	while (!(k=os_GetCSC()));
	return k;
}
