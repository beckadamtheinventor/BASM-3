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

#define SIZEOF_INCLUDE_ENTRY 17
typedef struct __include_entry_t {
	char fname[8];
	char *namespace;
	void *ptr;
	void *next;
} include_entry_t;


const char *TEMP_FILE = "_BASMtmp";
const char *TEMP_FILE_2 = "_BASMtm2";
const char *MESSAGE_FILE = "_BASMlog";
const char *MemoryError = "Insufficient Memory";
const char *UndefinedLabelError = "Undefined word/label";
const char *NumberFormatError = "Number format Error";
const char *SyntaxError = "Syntax Error";
const char *NamespaceError = "No namespace";
const char *UserBreakError = "Cancel. Abort.";
const char *UserBreakWord = "\\o/";
const char *HEX_CHARS = "0123456789ABCDEF";

extern uint8_t buffer[];
uint8_t ADDR_BYTES = 3;
uint8_t CURRENT_BYTES;
uint8_t *O_FILE_PTR;
define_entry_t *internal_define_pointers[26];
char *LAST_LINE;
unsigned int assembling_line = 0;
unsigned int ORIGIN = 0;
#define MAX_STACK 2048
int DATA_STACK[MAX_STACK];
label_t *WORD_STACK;
int DATA_SP = 0;
int WORD_SP = 0;
char *ErrorCode = 0;
char *ErrorWord = 0;
unsigned int O_FILE_ORG,I_FILE_ORG,O_FILE_TELL,O_FILE_SIZE;
char *NAMESPACE;
unsigned int STACK_SP;
int *STACK;
int (*OutputWriteFunction)(void *data,int len,int count,ti_var_t fp) = ti_Write;

ti_var_t gfp;

include_entry_t first_include;
include_entry_t *last_include = &first_include;

int assemble(const char *inFile, char *outFile);
int main_assembler(uint8_t **ptr,uint8_t *max,char *endcode,char *outFile,ti_var_t fp);
int main_postprocessor(ti_var_t fp);

void trySetNamespace(const char *line);
void *readTokens(uint8_t *buffer,unsigned int amount,void *ptr,void *max);
void removeLeadingSpaces(uint8_t *buffer);
int parseLabel(char *buf);
void writeArgs(char *buf,int len,ti_var_t fp);
void setLabelValue(label_t *data,const char *value_ptr);
void setLabelValueValue(label_t *data,int value);
int getLabelValue(label_t *lbl);
label_t *findLabel(const char *name);
void defineLabel(const char *name,void *val);
void defineGoto(void *val,int offset);
void UpdateWordStack(void);
uint8_t *getEmitData(const char *name);
uint8_t *checkIncludes(const char *name);
uint8_t *searchIncludeFile(void *data_ptr, const char *cname);
bool includeFile(const char *fname,const char *namespace);

void error(const char *str,const char *word);
char *printAt(const char *str,uint8_t x,uint8_t y);
char *printXAt(const char *str,int amt,uint8_t x,uint8_t y);
char *upperCaseStr(char *str);
sk_key_t pause(void);
void stack_push(int val);
int stack_pop(void);

int _writeHexNibbles(void *data,int len,int count,ti_var_t fp);

void *cpyup(void *dest,void *src,size_t len);

extern int isNumber(const char *line);


void main(void){
	ti_var_t fp,fp2;
	char inFile[10];
	char outFile[11];
	string_t *ptr;
	uint8_t vt;

	ti_CloseAll();
	os_ClrHomeFull();
	if ((ptr = os_GetAnsData(&vt))&&vt==4){
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
				if (STACK=malloc(512*3)){
					STACK_SP=0;
					gfp = ti_OpenVar(TEMP_FILE_2,"w",TI_TPRGM_TYPE);
					if (assemble(&inFile,&outFile)){
						if (outFile[0]&&outFile[9]){
							fp2 = ti_OpenVar(TEMP_FILE,"r",TI_TPRGM_TYPE);
							if (outFile[10]&2 && outFile[9]==TI_PPRGM_TYPE){
								outFile[9]=TI_PRGM_TYPE; //make it a non-protected program if it's a hexdump
							}
							fp = ti_OpenVar(&outFile,"w",outFile[9]);
							OutputWriteFunction(ti_GetDataPtr(fp2),O_FILE_SIZE,1,fp);
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
				} else {
					printAt(MemoryError,0,8);
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
	int len;

	NAMESPACE=0;

	if (!(fp = ti_OpenVar(inFile,"r",TI_PRGM_TYPE))){
		ErrorCode = "Input file not found";
		ErrorWord = inFile;
		return 0;
	}
	I_FILE_ORG = (int)(ptr = ti_GetDataPtr(fp));
	max = ptr+(len=ti_GetSize(fp));
	ti_Close(fp);


	UpdateWordStack();
	printAt("Assembling...  ",0,9);

	fp = ti_OpenVar(TEMP_FILE,"w",TI_TPRGM_TYPE);
	if (len){
		ti_Resize(len,fp);
		ti_Rewind(fp);
	}
	ptr = (uint8_t*)I_FILE_ORG;
	assembling_line = 1;
	if (main_assembler(&ptr,max,0,outFile,fp)){
		O_FILE_SIZE = ti_Tell(fp);
		main_postprocessor(fp);
	}
	ti_Close(fp);
	return !(int)ErrorCode;
}

int main_postprocessor(ti_var_t fp){
	label_t *gt;
	int i;
	assembling_line = 0;
	ErrorCode = 0;
	printAt("Filling addresses...     ",0,9);
	i = WORD_SP;
	while (i--) {
		gt = &WORD_STACK[i];
		if (gt->bytes&F_EXPRESSION_VALUE) ErrorWord=(char*)gt->value;
		if (gt->bytes&M_NUM_BYTES){ //set address for this item.
			int val = getLabelValue(gt); //get the goto's value
			if (!ErrorCode){
				if (gt->bytes&F_OFFSET_VALUE){
					val -= gt->org; //calculate the JR offset byte
					if ((unsigned)(val+0x80)>0xFF) ErrorCode = "JR offset out of range";
				}
				ti_Seek(gt->offset,SEEK_SET,fp); //seek to the file offset where the data needs to go
				ti_Write(&val,gt->bytes&M_NUM_BYTES,1,fp); //write in the data
			}
		} else { //try to set the label's value
			if (gt->bytes&F_EXPRESSION_VALUE){ //pointer to value expression
				int val = getLabelValue(gt);
				if (!ErrorCode){ //no errors occurred
					gt->bytes&=-(F_EXPRESSION_VALUE|F_UNDEFD_VALUE);
					gt->value = val;
				}
			}
		}
		if (ErrorCode) return 0;
	}
	return 1;
}

int main_assembler(uint8_t **ptr,uint8_t *max,char *endcode,char *outFile,ti_var_t fp){
	uint8_t buf[512];
	int i,len;
	uint8_t c;
	while (*ptr<max) {
		uint8_t *edata;
		O_FILE_ORG = (int)(O_FILE_PTR=ti_GetDataPtr(fp))-(O_FILE_TELL=ti_Tell(fp));


		if (os_GetCSC()==sk_Clear){
			ErrorCode = UserBreakError;
			ErrorWord = UserBreakWord;
			break;
		}
		ErrorCode = 0;
		if (**ptr==tEnter){
			(*ptr)++;
		} else {
			*ptr=readTokens(&buf,511,*ptr,max);
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
					if (endcode){
						if (!strcmp(&buf,endcode)){
							return 1;
						}
					}
					if (!strncmp(&buf,"SIS ",4)){
						ti_PutC(0x40,fp);
						memcpy(&buf,&buf[4],508);
					} else if (!strncmp(&buf,"LIS ",4)){
						ti_PutC(0x49,fp);
						memcpy(&buf,&buf[4],508);
					} else if (!strncmp(&buf,"SIL ",4)){
						ti_PutC(0x52,fp);
						memcpy(&buf,&buf[4],508);
					} else if (!strncmp(&buf,"LIL ",4)){
						ti_PutC(0x5B,fp);
						memcpy(&buf,&buf[4],508);
					}
					if (*buf==0xC4){
						uint8_t *sptr = &buf;
						getNumber(&sptr,0,0);
					} else if (*buf=='.'){
						if (parseLabel(&buf)) ErrorCode=0;
					} else {
						if (!strncmp(&buf,"//",2)){
							continue;
						} else if ((unsigned)((c=*buf)-0x41)<27){
							if (!strncmp(&buf,"FORMAT ",7)){
								line = &buf[7];
								do {
									if (!strncmp(line,"ASM",3)){
										ti_Write("\xEF\x7B",2,1,fp);
										ORIGIN = 0xD1A881;
										outFile[9]=TI_PPRGM_TYPE;
										line+=3;
									} else if (!strncmp(line,"APPVAR",6)){
										outFile[9]=TI_APPVAR_TYPE;
										line+=6;
									} else if (!strncmp(line,"PRGM",4)){
										outFile[9]=TI_PPRGM_TYPE;
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
									} else if (!strncmp(line,"HEXDUMP",7)){
										OutputWriteFunction = _writeHexNibbles;
										outFile[10]|=2;
										line+=7;
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
							} else if (!strncmp(&buf,"VIRTUAL ",8)){
								char *oldns;
								int oldorg = ORIGIN;
								line = &buf[8];
								do {
									if (!strncmp(line,"AT ",3)){
										line+=3;
										ORIGIN = getNumber(&line,0,0);
									}
								} while (*line++);
								oldns = NAMESPACE;
								main_assembler(ptr,max,"END VIRTUAL",outFile,fp);
								NAMESPACE = oldns;
								ORIGIN = oldorg;
							} else if (!strncmp(&buf,"SECTION ",8)){
								int oldorg;
								char *oldns;
								int startoff=ti_Tell(fp);
								line = &buf[8];
								oldorg = ORIGIN;
								do {
									if (!strncmp(line,"AT ",3)){
										line+=3;
										ORIGIN = getNumber(&line,0,0);
									}
								} while (*line++);
								oldns = NAMESPACE;
								main_assembler(ptr,max,"END SECTION",outFile,fp);
								NAMESPACE = oldns;
								ORIGIN = oldorg+ti_Tell(fp)-startoff;
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
										if (len){
											if (nsname=malloc(len+1)){
												if (len) strncpy(nsname,line,len);
												else nsname=0;
												includeFile(incname,nsname);
											}
										} else {
											includeFile(incname,0);
										}
									} else {
										ErrorCode = "Null appvar name";
									}
								} else {
									ErrorCode = "Invalid argument";
								}
							} else if (!strncmp(&buf,"LBL ",4)){
								if (parseLabel(&buf[4])) trySetNamespace(&buf[4]);
							} else if (parseLabel(&buf)){
								trySetNamespace(&buf);
							} else {
								edata=getEmitData(&buf);
								if (!ErrorCode){
									if (!*edata){
										uint16_t n=*((uint16_t*)(edata+1));
										ti_Write(edata+3,n,1,fp);
										ORIGIN+=n;
									} else {
										uint8_t n;
										ti_Write(edata+1,n=*edata,1,fp);
										ORIGIN+=n;
									}
								}
							}
						} else {
							if (!ErrorCode) ErrorCode = SyntaxError;
						}
					}
				}
			}
			sprintf(&buffer,"line %d",assembling_line);
			printAt(&buffer,14,9);
			if (ErrorCode) {
				printAt(&buf,0,6);
				break;
			}
			assembling_line++;
		}
	}
	return !(int)ErrorCode;
}

void trySetNamespace(const char *line){
	char c;
	if (*line!='.'){
		char *name = line;
		while ((c=*line++)&&c!=':'); //repeat until colon or eol
		if (c&&(!*line--)){ //if the line ends with ':'
			char *ns;
			int l=line-name;
			if (ns=malloc(l+1)){
				memcpy(ns,name,l);
				ns[l]=0;
				NAMESPACE = ns; //set the namespace
			}
		}
	}
}

int parseLabel(char *buf){
	char c;
	int i=0;
	uint8_t *ptr2;
	ptr2=buf;
	ErrorCode=0;
	while ((c=*ptr2++)&&c!=':');
	if (c){
		int len2;
		uint8_t *ptr3;
		uint8_t *name;
		i=ptr2-buf;
		if (*buf=='.'){
			if (NAMESPACE){
				int nslen=strlen(NAMESPACE);
				if (name=malloc(nslen+i--)){
					memcpy(name,NAMESPACE,nslen);
					memcpy(name+nslen,buf,i);
					i+=nslen;
				} else {
					ErrorCode=MemoryError;
				}
			} else {
				ErrorCode=NamespaceError;
			}
		} else if (name=malloc(i--)){ //clone the label name
			memcpy(name,buf,i);
		} else {
			ErrorCode=MemoryError;
		}
		if (!ErrorCode){
			name[i]=0;
			if (*ptr2){
				ptr2--;
				if (ptr3=processDataLine(ptr2,0)){
					CURRENT_BYTES=F_EXPRESSION_VALUE;
					defineLabel(name,ptr3);
					return 1;
				}
			} else {
				CURRENT_BYTES=0;
				defineLabel(name,(void*)ORIGIN);
				return 1;
			}
		}
	} //this isn't a label define, is it?
	ErrorWord = buf;
	return 0;
}

void *readTokens(uint8_t *buffer,unsigned int amount,void *ptr,void *max){
	unsigned int str_len,i;
	char *str;
	uint8_t c;
	i=0;
	while (i<amount && ptr<max && (*(uint8_t*)ptr)!=tEnter){
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
	memset(buffer+i,0,amount+1-i);
	return ptr;
}

void removeLeadingSpaces(uint8_t *buffer){
	int j;
	uint8_t c;
	int i = -1;
	while (buffer[++i]==' ');
	j=i;
	while ((c=buffer[i++])&&(!(c=='/'&&buffer[i]=='/')));
	if (buffer[i]) i--;
	i-=j;
	if (j) memcpy(buffer,buffer+j,i);
	buffer[i]=0;
}

void writeArgs(char *buf,int len,ti_var_t fp){
	int num;
	char c;
	CURRENT_BYTES = len;
	while (c=*buf){
		if (c=='"'){
			char *str;
			int len;
			buf++; str=buf;
			while ((c=*buf++)&&c!='"');
			len=buf-(str+1);
			ti_Write(str,len,1,fp);
			ORIGIN+=len;
			buf++;
		} else if (c==','){
			buf++;
		} else {
			uint8_t *ptr=buf;
			num = getNumber(&buf,0,0);
			buf--;
			if (ErrorCode==UndefinedLabelError){
				int slen;
				if (ptr=processDataLine(ptr,',')){
					defineGoto(ptr,0);
					ErrorCode=0;
				} else {
					ErrorCode=MemoryError;
				}
			}
			ti_Write(&num,len,1,fp);
			ORIGIN+=len;
			if (ErrorCode){
				break;
			}
		}
	}
}

label_t *findLabel(const char *name){
	label_t *data;
	int i = WORD_SP;
	while (i--) {
		data = &WORD_STACK[i];
		if (!(data->bytes&M_NUM_BYTES)){
			if (!strcmp(data->name,name)){
				return data;
			}
		}
	}
	return 0;
}

int getLabelValue(label_t *lbl){ //get the value of a label
	if (lbl->bytes&F_EXPRESSION_VALUE){ //it's a pointer to the value
		void *ptr = (void*)lbl->value;
		return getNumber(&ptr,lbl,0); //return the computed value
	}
	ErrorCode=0;
	return lbl->value; //it's just a plain value
}

void setLabelValue(label_t *data,const char *value_ptr){
	data->value = (int)value_ptr; //set the pointer to it's value's expression
	data->bytes = (data->bytes&(-F_UNDEFD_VALUE))|F_EXPRESSION_VALUE;
}

void setLabelValueValue(label_t *data,int value){
	data->value = value;
	data->bytes &= -(F_UNDEFD_VALUE|F_EXPRESSION_VALUE);
}

void defineLabel(const char *name,void *val){ //write a new label
	ti_Write(&name,3,1,gfp); //write a pointer to the name of the label.
	ti_Write(&val,3,1,gfp); //write the value of the label.
	ti_Write(&ORIGIN,3,1,gfp); //write the label's origin pointer
	ti_Write(&O_FILE_TELL,3,1,gfp); //write the current file offset
	ti_PutC(CURRENT_BYTES&-M_NUM_BYTES,gfp);//CURRENT_BYTES is also used to hold whether the value is a pointer to an expression or just a value.
	UpdateWordStack();
}

void defineGoto(void *val,int offset){ //write a new 'goto'. Basically a place to put an address.
	int tell=O_FILE_TELL+offset;
	offset+=ORIGIN;
	ti_Write(&val,3,1,gfp); //gotos don't have names, so just write the value so that we can display the error word correctly.
	ti_Write(&val,3,1,gfp); //write a pointer to the value of the address.
	ti_Write(&offset,3,1,gfp); //write the goto's origin pointer
	ti_Write(&tell,3,1,gfp); //write the current file offset
	ti_PutC(CURRENT_BYTES|F_EXPRESSION_VALUE,gfp); //CURRENT_BYTES -> length of data & flags. Gotos are always expressions until they are defined later.
	UpdateWordStack();
}

uint8_t *getEmitData(const char *name){ //data to write to the file during assembly
	uint8_t *data;
	define_entry_t *ptr;
	if (data=checkNoArgOpcode(name,&ptr)){
		return data;
	} else if (ErrorCode){
		return 0;
	} else if (data=checkIncludes(name)){
		return data;
	} else if (data=processOpcodeLine(name)){
		while (ptr->bytes){
			if (!strncmp(data,&ptr->opcode,N_OPCODE_NAME_BYTES)){
				uint8_t *buf4;
				if (buf4=malloc(8)){
					int i = ptr->flags&M_ARG_BYTE;
					memcpy(buf4+1,&ptr->emit,*buf4=ptr->bytes&M_NUM_BYTES);
					if (ptr->flags&F_JR_ARG){
						CURRENT_BYTES=F_EXPRESSION_VALUE|F_OFFSET_VALUE;
					} else {
						CURRENT_BYTES=F_EXPRESSION_VALUE;
					}
					if (ptr->flags&(F_OFFSET_ARG|F_BYTE_ARG)){
						CURRENT_BYTES|=1;
					} else if (ptr->flags&F_LONG_ARG){
						CURRENT_BYTES|=ADDR_BYTES;
						if (ADDR_BYTES!=2) buf4[0]++;
					}
					free(data);
					if (data=getArgFromLine(name)){
						int val;
						label_t *lbl;
						defineGoto(data,i); //we need to set this address eventually if it's undefined.
						val=getLabelValue(lbl=&WORD_STACK[WORD_SP]);
						//if (ErrorCode==UndefinedLabelError){ //probably don't need this
							//ErrorCode=0;
						//} else {
							//if (lbl->bytes&F_OFFSET_VALUE){
								//val+=ORIGIN+1;
							//}
							//memcpy(buf4+i+1,&val,CURRENT_BYTES&M_NUM_BYTES);
							//WORD_SP--;
							//ti_Seek(SIZEOF_LABEL_T,SEEK_END,gfp); //basically forget about this goto, it's been taken care of.
						//}
					}
					return buf4;
				} else {
					ErrorCode = MemoryError;
					return 0;
				}
			}
			ptr++;
		}
		ErrorCode=UndefinedLabelError;
	} else {
		ErrorCode = MemoryError;
	}
	return 0;
}

uint8_t *searchIncludeFile(void *data_ptr, const char *cname){
	ti_var_t fp;
	uint8_t *ptr;
	uint8_t *org;
	ErrorCode=0;
	ptr = org = data_ptr;
	ptr+=*((uint16_t*)(ptr+32+((cname[0]-0x41)<<1))); //get the pointer to the list of constants starting with the first letter
	while (*ptr){
		int total_len;
		uint8_t *next=ptr+strlen(ptr)+1;
		if (!strcmp(ptr,cname)){ //found it!
			if (*next){ //return the value
				return next;
			} else { //this is a special kind of include definition; is inserts code.
				//instead of just returning the data, we have to process it a bit.
				//the first two bytes must be null for this kind of definition.
				//the next two bytes are the total length of the entry.
				//the next two bytes are the length of the relocations table.
				void *data;
				int code_len,relo_len;
				total_len = *((uint16_t*)(next+2));
				relo_len = *((uint16_t*)(next+4));
				code_len = total_len-(relo_len+6);
				if (data=malloc(code_len+3)){
					uint16_t *relo_tbl=(uint16_t*)(next+6);
					memcpy(data+3,next+relo_len+3,code_len); //write the code
					while (relo_len-=2){ //add the new offset to all the specified jumps
						(*(int*)(data+3+(*relo_tbl++)))+=ORIGIN;
					}
					*((int*)data)=code_len<<8;
					return (uint8_t*)data;
				} else {
					ErrorCode=MemoryError;
					return 0;
				}
			}
		}
		//continue searching
		if (!(total_len=*next)) total_len=*((uint16_t*)(next+2))+1;
		ptr=next+total_len+1;
	}
	return 0; //nothing found
}

void UpdateWordStack(void){
	ti_Rewind(gfp);
	WORD_STACK = ti_GetDataPtr(gfp);
	WORD_SP = ti_GetSize(gfp) / SIZEOF_LABEL_T;
	ti_Seek(0,SEEK_END,gfp);
}

uint8_t *checkIncludes(const char *name){
	include_entry_t *ent = &first_include;
	do {
		uint8_t *rv;
		char *cname;
		bool i = 0;
		if (!ent->namespace){
			i = 1;
			cname = name;
		} else {
			int len = strlen(ent->namespace);
			if (!strncmp(name,ent->namespace,len)){ //check the prefix
				i = 1;
				cname = name+len;
			}
		}
		if (i){ //if the prefix is correct or there isn't a prefix
			if (rv=searchIncludeFile(ent->ptr,cname)){
				return rv; //return the value if it's non-zero
			}
		}
	} while (ent=ent->next); //next include entry
	ErrorWord = name;
	return 0;
}

bool includeFile(const char *fname,const char *namespace){
	ti_var_t fp;
	include_entry_t *ent;
	void *ptr;
	if (fp=ti_Open(fname,"r")){
		ptr=ti_GetDataPtr(fp);
		ti_Close(fp);
	} else {
		ErrorCode = "Include appvar not found";
		return 0;
	}
	if (strcmp(ptr,"BASM3.0 INC")){
		ErrorCode = "Malformed include file";
		return 0;
	}

	if (!(ent=malloc(SIZEOF_INCLUDE_ENTRY))){
		ErrorCode = MemoryError;
		return 0;
	}

	memcpy(&last_include->fname,fname,8);
	last_include->namespace = namespace;
	last_include->ptr = ptr;
	last_include->next = ent;
	last_include = ent;
	ent->next = 0;
	return 1;
}

void error(const char *str,const char *word){
	printAt(str,0,1);
	if (word){
		printAt(" \"",0,2);
		printAt(word,2,2);
		os_PutStrFull("\"");
	}
	if (assembling_line){
		char sbuf[26];
		sprintf(&sbuf,"Error on line %d",assembling_line);
		printAt(&sbuf,0,3);
	}
}

char *printAt(const char *str,uint8_t x,uint8_t y){
	os_SetCursorPos(y,x);
	if (strlen(str)){
		os_PutStrFull(str);
	}
	return str;
}

char *printXAt(const char *str,int amt,uint8_t x,uint8_t y){
	char *s = (char*)malloc(amt+1);
	memcpy(s,str,amt);
	s[amt] = 0;
	printAt(s,x,y);
	free(s);
	return str;
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

void stack_push(int val){
	STACK[STACK_SP++]=val;
}
int stack_pop(void){
	return STACK[--STACK_SP];
}

int _writeHexNibbles(void *data,int len,int count,ti_var_t fp){
	uint8_t *dataptr;
	int i;
	int total = len*count;
	dataptr = data;
	ti_Resize(ti_GetSize(fp)+(total<<1),fp);
	for (i=0;i<total;i++){
		uint8_t c = *dataptr++;
		ti_PutC(HEX_CHARS[c>>4],fp);
		ti_PutC(HEX_CHARS[c&0xF],fp);
		if ((i&7)==7) ti_PutC(0x3F,fp);
	}
	return total;
}

