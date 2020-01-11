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

#define SIZEOF_INCLUDE_ENTRY 14
typedef struct __include_entry_t {
	char fname[8];
	char *namespace;
	void *next;
} include_entry_t;


const char *TEMP_FILE = "_BASMtmp";
const char *TEMP_FILE_2 = "_BASMtm2";
const char *MemoryError = "Insufficient Memory";
const char *UndefinedLabelError = "Undefined word/label";
const char *UserBreakError = "Cancel. Abort.";
const char *UserBreakWord = "\\o/";

extern uint8_t buffer[];
uint8_t ADDR_BYTES = 3;
uint8_t CURRENT_BYTES;
uint8_t *O_FILE_PTR;
define_entry_t *internal_define_pointers[27];
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
include_entry_t *last_include = &first_include;


void *readTokens(uint8_t *buffer,unsigned int amount,void *ptr,void *max);
void removeLeadingSpaces(uint8_t *buffer);
int parseLabel(char *buf,ti_var_t fp);
void writeArgs(char *buf,int len,ti_var_t fp);
void setLabelValue(const char *name,const char *value_ptr);
void setLabelValueValue(const char *name,int value);
int getLabelValue(label_t *lbl);
label_t *findLabel(const char *name);
void defineLabel(const char *name,int val);
void defineGoto(void *val,int offset);
void UpdateWordStack(void);
int assemble(const char *inFile, char *outFile);
uint8_t *getEmitData(const char *name);
uint8_t *checkIncludes(const char *name);
uint8_t *searchIncludeFile(const char *fname, const char *cname);
bool includeFile(const char *fname,const char *namespace);

void error(const char *str,const char *word);
void printAt(const char *str,uint8_t x,uint8_t y);
void printXAt(const char *str,int amt,uint8_t x,uint8_t y);
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
				for (l=0;l<27;l++){
					internal_define_pointers[l] = (define_entry_t*)(org + *ptr++);
				}
				ti_Close(fp2);
				gfp = ti_OpenVar(TEMP_FILE_2,"w",TI_TPRGM_TYPE);
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
				ti_Close(gfp);
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
	int i,len;

	if (!(fp = ti_OpenVar(inFile,"r",TI_PRGM_TYPE))){
		ErrorCode = "Input file not found";
		ErrorWord = inFile;
		return 0;
	}
	I_FILE_ORG = (int)(ptr = ti_GetDataPtr(fp));
	max = ptr+(len=ti_GetSize(fp));
	ti_Close(fp);



	printAt("Prescanning...",0,9);
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
			removeLeadingSpaces(&buf);
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
				len = strlen(line)+1;
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
		if (len) ti_Resize(len,fp);
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
				removeLeadingSpaces(&buf);
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
						if ((unsigned)((c=*buf)-0x41)<27){
							if (!strncmp(&buf,"FORMAT ",7)){
								line = &buf[7];
								do {
									if (!strncmp(line,"ASM",3)){
										ti_Write("\xEF\x7B",2,1,fp);
										ORIGIN = 0xD1A87F;
										outFile[9]=0x06;
										line+=3;
									} else if (!strncmp(line,"APPVAR",6)){
										outFile[9]=0x15;
										line+=6;
									} else if (!strncmp(line,"PRGM",4)){
										outFile[9]=0x06;
										line+=4;
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
							} else if (!strncmp(&buf,"ORIGIN ",7)){
								line = &buf[7];
								ORIGIN=getNumber(&line,0,0);
							} else if (!strncmp(&buf,"INCLUDE ",8)){
								line = &buf[8];
								if (*line=='"'){
									int len = 0;
									line++;
									do c=line[len++]; while (c&&c!='"'); len--;
									if (len){
										char *incname;
										char *nsname;
										if (len>8){
											ErrorCode = "Appvar name too long";
										}
										line[len]=0;
										incname = line;
										line+=len+2;
										len = strlen(line);
										if (nsname=malloc(len+1)){
											if (len) memcpy(nsname,line,len);
											nsname[len]=0;
											includeFile(incname,nsname);
										}
									} else {
										ErrorCode = "Null appvar name";
									}
								} else {
									ErrorCode = "Invalid argument";
								}
							} else if (!strncmp(&buf,"LBL ",4)){
								parseLabel(&buf[4],fp);
								ErrorCode = 0;
							} else if (parseLabel(&buf,fp)){
								ErrorCode = 0;
							} else if (edata=getEmitData(&buf)){
								ti_Write(edata+1,edata[0],1,fp);
							} else {
								ErrorCode = UndefinedLabelError;
							}
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
					printAt(&buf,0,6);
					break;
				}
				assembling_line++;
			}
		}
		if (!ErrorCode){
			//ti_Resize(ti_Tell(fp),fp); //this bugs out for some reason
			O_FILE_SIZE = ti_Tell(fp);
			assembling_line = 0;
			printAt("Filling addresses...     ",0,9);
			i = WORD_SP;
			while (i--) {
				label_t *gt = &WORD_STACK[i];
				if (gt->bytes&7){ //set address for this item.
					int val = getLabelValue(gt); //get the goto's value
					if (gt->bytes&8){
						if ((unsigned)(val+0x80)>0xFF){ //if it's not already within the correct range
							val -= gt->org+gt->offset+(gt->bytes&7); //calculate the JR offset byte
							if ((unsigned)(val+0x80)>0xFF){
								ErrorCode = "JR offset out of range";
								ErrorWord = (char*)gt->value;
								break;
							}
						}
					} else {
						val -= 2; //argument offsets from the start of the opcode.
					}
					ti_Seek(gt->offset,SEEK_SET,fp); //seek to the file offset where the data needs to go
					ti_Write(&val,gt->bytes&3,1,fp); //write in the data
				} else { //try to set the label's value
					int val = getLabelValue(gt);
					if (gt->bytes&0x80){ //pointer to value
						if (!ErrorCode){
							gt->bytes&=0x3F;
							gt->value = val;
						}
					}
				}
				if (ErrorCode) {
					if (!ErrorWord) ErrorWord=(char*)gt->value;
				}
			}
		}
		ti_Close(fp);
	}

	return !(int)ErrorCode;
}

int parseLabel(char *buf,ti_var_t fp){
	char c;
	int i=0;
	while ((c=buf[++i])&&c!=':'); //skip until the ':'
	if (c){
		uint8_t *ptr2;
		int val,offset;
		buf[i++]=0; //overwrite the ':'
		ErrorWord = ptr2 = buf+i+1; //the error will be the value of the label in case something goes wrong
		offset = ORIGIN+ti_Tell(fp); //the offset in the output program plus the assembly origin
		if ((c=buf[i])=='='){ //set the label value to an expression
			int len2;
			uint8_t *ptr3;
			len2 = strlen(ptr2)+1;
			if (ptr3=malloc(len2)){ //clone the data so we can keep it
				memcpy(ptr3,ptr2,len2);
				val=(int)ptr3;
				setLabelValue(buf,ptr3);
			} else {
				ErrorCode = MemoryError; //if we can't malloc, there's no memory left. Abort.
			}
		} else if (c=='+'){ //set the label value to the offset plus the following expression
			setLabelValueValue(buf,val=offset+getNumber(&ptr2,offset,0));
		} else if (c=='-'){ //set the label value to the offset minus the following expression
			setLabelValueValue(buf,val=offset-getNumber(&ptr2,offset,0));
		} else { //set the label value to the offset
			setLabelValueValue(buf,val=offset);
			ErrorWord = 0; //there is no value, therefore the old error word is invalid.
		}
		return val;
	} else { //this isn't a label define, is it?
		ErrorWord = buf;
		return 0;
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

void removeLeadingSpaces(uint8_t *buffer){
	int j;
	int i = 0;
	while (buffer[i++]==' '); i--;
	j=i;
	do i++; while (buffer[i]&&strncmp(buffer+i,"//",2));
	if (j) strcpy(buffer,buffer+j);
	buffer[i]=0;
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
			buf--;
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

int getLabelValue(label_t *lbl){ //get the value of a label
	if (lbl->bytes&0x80){ //it's a pointer to the value
		void *ptr = (void*)lbl->value;
		return getNumber(&ptr,0,0); //return the computed value
	}
	return lbl->value; //it's just a plain value
}

void setLabelValue(const char *name,const char *value_ptr){
	label_t *data;
	if (data=findLabel(name)){
		data->value = (int)value_ptr; //set the pointer to it's value's expression
		data->bytes = (data->bytes&0x3F)|0x80;
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
	ti_Write(&val,3,1,gfp); //write a pointer to the value of the address.
	ti_Write(&offset,3,1,gfp); //write file offset in output file
	ti_Write(&ORIGIN,3,1,gfp); //write the goto's origin pointer
	ti_PutC(CURRENT_BYTES|0x80,gfp); //CURRENT_BYTES -> length of data & flags, 0x80 -> pointer to value's expression
	UpdateWordStack();
}

uint8_t *getEmitData(const char *name){ //data to write to the file during assembly
	uint8_t *data;
	define_entry_t *ptr;
	if (data=checkInternal(name,&ptr)){
		return data;
	} else if (data=checkIncludes(name)){
		return data;
	} else if (data=processOpcodeLine(name)){
		while (ptr->bytes){
			if (!strncmp(data,&ptr->opcode,12)){
				uint8_t *buf4;
				if (buf4=malloc(8)){
					int i = ptr->flags&7;
					memcpy(buf4,&ptr->bytes,(ptr->bytes&7)+1);
					if (ptr->flags&F_JR_ARG){
						CURRENT_BYTES=0x88;
					} else {
						CURRENT_BYTES=0x80;
					}
					if (ptr->flags&(F_OFFSET_ARG|F_BYTE_ARG)){
						CURRENT_BYTES|=1;
					} else if (ptr->flags&F_LONG_ARG){
						CURRENT_BYTES|=ADDR_BYTES;
						if (ADDR_BYTES!=2) buf4[0]++;
					}
					free(data);
					if (data=getArgFromLine(name)){
						defineGoto(data,i); //we need to set this address eventually if it's undefined.
					}
					return buf4;
				} else {
					ErrorCode = MemoryError;
					return 0;
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
	unsigned int elen;
	uint8_t i;
	if (fp = ti_Open(fname,"r")){
		ptr = ti_GetDataPtr(fp);
		ti_Close(fp);
		if (!strcmp(ptr,"BASM3.0 INC")){ //check if it's a valid include file
			i = cname[0]-0x41;
			ptr += *((uint16_t*)(ptr+i*2+32)); //get the pointer to the list of constants starting with the first letter
			while (*ptr){
				uint8_t *next = ptr+strlen(ptr)+1;
				if (!strcmp(cname,ptr)){ //found it!
					return next; //return the value
				}
				ptr=next+*next+1; //continue searching
			}
		}
	}
	return 0; //nothing found
}

void UpdateWordStack(void){
	ti_Rewind(gfp);
	WORD_STACK = ti_GetDataPtr(gfp);
	WORD_SP = ti_GetSize(gfp) / SIZEOF_LABEL_T;
}

uint8_t *checkIncludes(const char *name){
	include_entry_t *ent = &first_include;
	do {
		uint8_t *rv;
		char *cname;
		int len;
		bool i = 0;
		if (!*ent->namespace){
			i = 1;
			len = 0;
		} else {
			len = strlen(ent->namespace);
			if (!strncmp(name,ent->namespace,len)){
				i = 1;
			}
		}
		if (i){
			cname = name+len;
			if (rv=searchIncludeFile(ent->fname,cname)){
				return rv;
			}
		}
	} while (ent=ent->next);
	ErrorWord = name;
	return 0;
}

bool includeFile(const char *fname,const char *namespace){
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

	memcpy(last_include->fname,fname,8);
	last_include->namespace = namespace;
	last_include = last_include->next = ent;
	ent->next = 0;
	return 1;
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
