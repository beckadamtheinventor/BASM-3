
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opcodes.h"

#define buffer_len 16
uint8_t buffer[buffer_len];

#define isAlphaNumeric(c) ((unsigned)(c-0x41)<26||(unsigned)(c-0x30)<10||c=='.')


char *processOpcodeLine(const char *name){
	unsigned char c;
	char *rv;
	char *ptr;
	int len;
	if (rv=ptr=malloc(len=strlen(name)+1)){
		memset(ptr,0,len);
		while ((c=*name++)&&c!=' ') *ptr++=c;
		*ptr++=c;
		if (c){
			while (c=*name++){ //loop over the line
				if (isRegister(name-1)){ //check if it's a register.
					*ptr++=c;
					if (c!='A'||*name=='F'){
						char c2;
						*ptr++=c2=*name++;
						if (c=='I'&&(c2=='X'||c2=='Y')){
							if ((c=*name)=='H'||c=='L'){ //ixh/l, iyh/l
								name++;
								*ptr++=c;
							} else { //ix/y
								*ptr++='@'; //write the offset placeholder
								while (isAlphaNumeric(*name++)); //skip until next non-value character.
							}
						}
					}
				} else if (isCondition(name-1)){
					*ptr++=c;
					if ((c=*name)!=','){
						*ptr++=c; name++;
					}
				} else if (isAlphaNumeric(c)) { //if it's an alphanumeric constant we need to skip it in order to match the opcode.
					*ptr++='#'; //write the placeholder
					loopexpression:;
					do c=*name++; while (isAlphaNumeric(c)); //skip until it's not
					if (c=='!'){
						name++;
					} else if (c=='-'||c=='+'||c=='*'||c=='/'||c=='='){
					} else if (c=='<'||c=='>'){
						if (*name=='=') name++;
					} else {
						name--;
						continue;
					}
					goto loopexpression;
				} else {
					*ptr++=c; //otherwise it's punctuation, and we probably need that.
				}
			}
		}
		return rv;
	} else {
		return 0;
	}
}

int getArgFromLine(const char *line,int offset){
	char c,cc;
	bool jr=0;
	cc=',';
	if ((!strncmp(line,"JR ",3))||(!strncmp(line,"DJNZ ",5))) jr=1;
	while ((c=*line++)&&c!=' '); //skip the first word
	if (c){
		char *data;
		int len;
		bool neg=0;
		while (c=*line){
			if (isRegister(line)){
				if ((c=*line++)!='A'){
					char c2 = *line++;
					if (c=='I'&&(c2=='X'||c2=='Y')){
						if ((c=*line)=='H'||c=='L'){ //ixh/l, iyh/l
							line++;
						} else {
							if (c=='-'){
								neg = 1;
								line++;
							} else if (c=='+'){
								line++;
							}
							break;
						}
					}
				}
			} else if (isCondition(line)){
				if ((c=*line)!=','){
					line++;
				}
			} else if (isAlphaNumeric(c)){
				break;
			} else {
				if (*line++=='(') cc=')';
			}
		}
		if (*line){
			len=0;
			do c=line[++len]; while (c&&c!=cc);
			if (data=malloc(len+1)){
				int num;
				memcpy(data,line,len);
				data[len]=0;
				num = getNumber(&data,offset,jr);
				if (neg) num = -num;
				return num;
			} else {
				ErrorCode = MemoryError;
			}
		}
		return 0;
	}
	return 0;
}

uint8_t *checkInternal(const char *line,define_entry_t **endptr){
	define_entry_t *ptr;
	ptr=internal_define_pointers[(*line)-0x41];
	while (ptr->bytes&&(ptr->flags&F_DIRECT_CMP)){
		if (!strncmp(line,&ptr->opcode,12)) {
			return &ptr->bytes;
		}
		ptr++;
	}
	*endptr = ptr;
	return 0;
}

void emitArgument(uint8_t *buf,const char *line,uint8_t flags,uint8_t bytes){
	int i = flags&7;
	CURRENT_BYTES = 0;
	if (flags&F_JR_ARG){
		CURRENT_BYTES|=8;
	}
	if (flags&F_OFFSET_ARG){
		CURRENT_BYTES|=1;
		buf[i+1] = getArgFromLine(line,i) - (ORIGIN+O_FILE_TELL+bytes);
	} else if (flags&F_BYTE_ARG){
		CURRENT_BYTES|=1;
		buf[i+1] = getArgFromLine(line,i);
	} else if (flags&F_LONG_ARG){
		int num;
		CURRENT_BYTES|=ADDR_BYTES;
		num = getArgFromLine(line,i);
		memcpy(buf+i+1,&num,ADDR_BYTES);
		if (ADDR_BYTES!=2) buf[0]++;
	}
}

bool isRegister(const char *name){
	char c,c2,c3;
	c=name[0]; c2=name[1]; c3=name[2];
	return (((c=='H' && c2=='L')||(c=='D'||c2=='E')||(c=='B'&&c2=='C')||(c=='A'&&c2=='F')||
	(c=='S'&&c2=='P'))&&(!isAlphaNumeric(c3)))||(c=='I'&&(c2=='X'||c2=='Y')&&(c3=='H'||c3=='L'||(!isAlphaNumeric(c3))))
	||(((unsigned)(c-0x41)<5||c=='H'||c=='L')&&(!isAlphaNumeric(c2)));
}

bool isCondition(const char *name){
	char c,c2,c3;
	c=name[0]; c2=name[1]; c3=name[2];
	return (c=='Z'||c=='C'||c=='M')&&(!isAlphaNumeric(c2))||(c=='N'&&(c2=='Z'||c2=='C')&&(!isAlphaNumeric(c3)))||
	(c=='P'&&(c2=='O'||c2=='E'||(!isAlphaNumeric(c2))));
}

int getNumber(char **line,int offset,bool jr){
	unsigned char c,c2;
	int number;
	uint8_t base;
	base=10;
	ErrorCode=0;
	if ((c=*(*line))=='('){
		(*line)++;
		number = getNumber(line,offset,jr);
	} else {
		number = 0;
		if ((c<0x30||c>0x39)&&c!='.'){
			uint8_t *data;
			char *oldline;
			char *nbuf;
			label_t *gt;
			oldline = *line;
			if (nbuf=getWord(line)){
				if (data=checkIncludes(nbuf)){
					free(nbuf);
					if (data[0]){
						memcpy(&number,data+1,data[0]);
					}
					return number;
				} else {
					label_t *lbl;
					int len=strlen(oldline)+1;
					if (data=malloc(len)){
						memcpy(data,oldline,len);
						if (jr) CURRENT_BYTES|=8;
						defineGoto(nbuf,data,offset);
					} else {
						ErrorCode = MemoryError;
						return 0;						
					}
					if (lbl=findLabel(nbuf)){
						number = getLabelValue(lbl);
					}
				}
			}
		}
	}
	if (ErrorCode) return 0;
	while (c=*(*line)++){
		if (c=='.') {
			c=*(*line)++;
			if (c=='X'){
				base = 16;
			} else if (c=='O') {
				base = 8;
			} else if (c=='D') {
				base = 10;
			} else if (c=='B') {
				base = 2;
			} else {
				ErrorCode = "Invalid Number Base";
				return 0;
			}
		} else if (c=='-') {
			number -= getNumber(line,offset,jr);
		} else if (c=='+') {
			number += getNumber(line,offset,jr);
		} else if (c=='/') {
			number /= getNumber(line,offset,jr);
		} else if (c=='*') {
			number *= getNumber(line,offset,jr);
		} else if (c=='='){
			if (*(*line)++=='='){
				number = (number==getNumber(line,offset,jr));
			} else {
				ErrorCode = "Invalid operator '='";
				return 0;
			}
		} else if (c=='>') {
			if ((c2=*(*line))=='>'){
				(*line)++;
				number >>= getNumber(line,offset,jr);
			} else if (c2=='='){
				number = number >= getNumber(line,offset,jr);
			} else {
				number = number > getNumber(line,offset,jr);
			}
		} else if (c=='<'){
			if ((c2=*(*line))=='<'){
				(*line)++;
				number <<= getNumber(line,offset,jr);
			} else if (c2=='=') {
				number = number <= getNumber(line,offset,jr);
			} else {
				number = number < getNumber(line,offset,jr);
			}
		} else if (c=='!'){
			if ((c2=*(*line))=='A'){
				number = number && getNumber(line,offset,jr);
			} else if (c2=='O'){
				number = number || getNumber(line,offset,jr);
			} else if (c2=='X'){
				number = !(number && getNumber(line,offset,jr));
			} else if (c2=='+'){
				number &= getNumber(line,offset,jr);
			} else if (c2=='-') {
				number |= getNumber(line,offset,jr);
			} else if (c2=='*') {
				number ^= getNumber(line,offset,jr);
			} else if (c2=='M') {
				number %= getNumber(line,offset,jr);
			} else {
				ErrorCode = "Syntax Error: Expected logical operator";
				return 0;
			}
		} else if (c==')'||c==','){
			(*line)++;
			return number;
		} else if (c!=' '){
			uint8_t a = digitValue(c);
			if (a<base){
				number = number*base + a;
			} else {
				ErrorCode = "Number Format Error";
				ErrorWord = (*line)-1;
				return 0;
			}
		}
	}
	return number;
}


int digitValue(char c){
	uint8_t a;
	if ((a = c-0x30)<10){
		return a;
	} else if ((a = c-0x41)<26){
		return a+10;
	}
	return 0xFF;
}

char *getWord(const char **line){
	char c;
	char *rv;
	int amt;
	char *ptr = *line;
	do c=*ptr++; while (isAlphaNumeric(c)); ptr--;
	if (rv=malloc((amt=(int)(ptr-*line))+1)){
		memcpy(rv,*line,amt);
		rv[amt] = 0;
	} else {
		if (amt){
			ErrorCode = MemoryError;
		}
		return 0;
	}
	*line = ptr;
	return rv;
}

