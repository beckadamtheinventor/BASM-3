
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

#define isAlphaNumeric(c) ((unsigned)(c-0x41)<27||(unsigned)(c-0x30)<10||c=='.')


char *processOpcodeLine(const char *name){
	unsigned char c;
	char *rv;
	char *ptr;
	int len;
	bool w=1;
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
							} else if (c==')'){
								*ptr++='@';
								*ptr++=c; name++;
								w=0;
							} else { //ix/y
								testAfterIXY:;
								if ((c=*name)==','){ //are we an offset or not?
									*ptr++=c; name++;
								} else if (c==' '){
									name++;
									goto testAfterIXY;
								} else if (c=='+'||c=='-'||c==')'||(!c)) {
									if (w) *ptr++='@'; //write the offset placeholder
									while ((c=*name++)!=')'&&c); //skip until ')' or EOL.
									if (c){
										*ptr++=c;
									}
									w=0;
								}
							}
						}
					}
				} else if (isCondition(name-1)){
					*ptr++=c;
					if ((c=*name)!=','){
						*ptr++=c; name++;
					}
				} else if (isAlphaNumeric(c)) { //if it's an alphanumeric constant we need to skip it in order to match the opcode.
					if (w) *ptr++='#'; //write the placeholder
					loopexpression:;
					do c=*name++; while (isAlphaNumeric(c)); //skip until it's not
					if (c=='!'){
						name++;
					} else if (c=='-'||c=='+'||c=='*'||c=='/'||c=='='){
					} else if (c=='<'||c=='>'){
						if (*name=='=') name++;
					} else {
						name--;
						w=0;
						continue;
					}
					goto loopexpression;
				} else if (c!=' ') { //if it's not whitespace
					*ptr++=c; //otherwise it's punctuation, and we probably need that.
				}
			}
		}
		return rv;
	} else {
		return 0;
	}
}

char *getArgFromLine(const char *line){
	char c,cc;
	cc=',';
	while ((c=*line++)&&c!=' '); //skip the first word
	if (c){
		if ((unsigned)(c-0x30)>=10){
			while (c=*line){
				if (isRegister(line)){
					if ((c=*line++)!='A'||(*line=='F')){
						char c2 = *line++;
						if (c=='I'&&(c2=='X'||c2=='Y')){
							if ((c=*line)=='H'||c=='L'){ //ixh/l, iyh/l
								line++;
							} else {
								if (c=='+'||c=='-'||c==')'||(!c)){
									if (c=='+'){
										line++;
									}
									break;
								}
							}
						}
					}
				} else if (isCondition(line)){
					if ((c=*line)!=','){
						line++;
					}
					line++;
				} else if (isAlphaNumeric(c)){
					break;
				} else {
					if (c=='(') cc=')';
					line++;
				}
			}
		}
		if (c=*line){
			char *data;
			int len=0;
			if (c==',') line++;
			do c=line[len++]; while (c&&c!=cc); len--;
			if (len){
				if (data=malloc(len+1)){
					int num;
					memcpy(data,line,len);
					data[len]=0;
					return data;
				} else {
					ErrorCode = MemoryError;
				}
			}
		}
	}
	return 0;
}

uint8_t *checkInternal(const char *line,define_entry_t **endptr){
	define_entry_t *ptr;
	int len;
	char *line2;
	char c;
	len=strlen(line);
	ptr=internal_define_pointers[(*line)-0x41];
	if (line2=malloc(len+1)){
		char *line3 = line2;
		while ((c=*line++)&&c!=' ') *line2++=c;
		*line2++=c;
		if (c){
			while (c=*line++){
				if (c!=' ') *line2++=c;
			}
			*line2++=0;
		}
		while (ptr->bytes&&(ptr->flags&F_DIRECT_CMP)){
			if (!strncmp(line3,&ptr->opcode,12)) {
				free(line3);
				return &ptr->bytes;
			}
			ptr++;
		}
		free(line3);
		*endptr = ptr;
	} else {
		ErrorCode = MemoryError;
	}
	return 0;
}

bool isRegister(const char *name){
	char c,c2,c3;
	c=name[0]; c2=name[1]; c3=name[2];
	return (((c=='H' && c2=='L')||(c=='D'||c2=='E')||(c=='B'&&c2=='C')||(c=='A'&&c2=='F')||
	(c=='S'&&c2=='P'))&&(!isAlphaNumeric(c3)))||
	(c=='I'&&(c2=='X'||c2=='Y'||(!isAlphaNumeric(c))&&(c3=='H'||c3=='L'||(!isAlphaNumeric(c3)))))
	||(((unsigned)(c-0x41)<5||c=='H'||c=='L')&&(!isAlphaNumeric(c2)));
}

bool isCondition(const char *name){
	char c,c2,c3;
	c=name[0]; c2=name[1]; c3=name[2];
	return (c=='Z'||c=='C'||c=='M')&&(!isAlphaNumeric(c2))||(c=='N'&&(c2=='Z'||c2=='C')&&(!isAlphaNumeric(c3)))||
	(c=='P'&&(c2=='O'||c2=='E'||(!isAlphaNumeric(c2))));
}

int getNumber(char **line,label_t *gt,bool jr){
	unsigned char c,c2;
	int number;
	uint8_t base;
	base=10;
	ErrorCode=0;
	if ((c=**line)==':'&&gt){
		(*line)++; c=*(*line)++;
		if (c=='-'){
			number = gt->offset+gt->org - getNumberNoMath(line,&base);
		} else if (c=='+'){
			number = gt->offset+gt->org + getNumberNoMath(line,&base);
		} else if (c=='='){
			number = getNumberNoMath(line,&base);
		} else if (!c){
			return gt->offset+gt->org;
		} else {
			ErrorCode = NumberFormatError;
			return 0;
		}
	} else {
		number = getNumberNoMath(line,&base);
	}
	if (jr) CURRENT_BYTES|=8;
	while (c=*(*line)++){
		if (c=='('){
			(*line)++;
			number = getNumber(line,gt,jr);
		} else if (c=='-') {
			number -= getNumberNoMath(line,&base);
		} else if (c=='+') {
			number += getNumberNoMath(line,&base);
		} else if (c=='/') {
			number /= getNumberNoMath(line,&base);
		} else if (c=='*') {
			number *= getNumberNoMath(line,&base);
		} else if (c=='=') {
			if (*(*line)++=='='){
				number = number==getNumberNoMath(line,&base);
			} else {
				ErrorCode = "Invalid operator '='";
				return 0;
			}
		} else if (c=='>') {
			if ((c2=*(*line))=='>'){
				(*line)++;
				number = number >> getNumberNoMath(line,&base);
			} else if (c2=='='){
				number = number >= getNumberNoMath(line,&base);
			} else {
				number = number > getNumberNoMath(line,&base);
			}
		} else if (c=='<'){
			if ((c2=*(*line))=='<'){
				(*line)++;
				number = number << getNumberNoMath(line,&base);
			} else if (c2=='=') {
				number = number <= getNumberNoMath(line,&base);
			} else {
				number = number < getNumberNoMath(line,&base);
			}
		} else if (c=='!'){
			if ((c2=*(*line))=='A'){
				number = number && getNumberNoMath(line,&base);
			} else if (c2=='O'){
				number = number || getNumberNoMath(line,&base);
			} else if (c2=='X'){
				number = ((number>0) ^ (getNumberNoMath(line,&base)>0));
			} else if (c2=='+'){
				number = number & getNumberNoMath(line,&base);
			} else if (c2=='-') {
				number = number | getNumberNoMath(line,&base);
			} else if (c2=='*') {
				number = number ^ getNumberNoMath(line,&base);
			} else if (c2=='M') {
				number = number % getNumberNoMath(line,&base);
			} else {
				ErrorCode = "Expected logical operator";
				return 0;
			}
		} else if (c==')'||c==','){
			(*line)++;
			return number;
		} else {
			ErrorCode = NumberFormatError;
		}
		if (ErrorCode) {
			ErrorWord = (*line)-1;
			return 0;
		}
	}
	return number;
}

int getNumberNoMath(char **line,uint8_t *base){
	int number;
	char c;
	uint8_t a;
	bool neg=0;
	number = 0;
	if (((unsigned)((c=*(*line))-0x30)>=10)&&c){
		uint8_t *data;
		char *oldline;
		char *nbuf;
		label_t *gt;
		oldline = *line;
		if (nbuf=getWord(line)){
			if (*nbuf=='.') {
				uint8_t *tptr;
				int nlen=strlen(nbuf);
				if (tptr=malloc(NAMESPACE_LEN+nlen)){
					memcpy(tptr,NAMESPACE,NAMESPACE_LEN);
					memcpy(tptr+NAMESPACE_LEN,nbuf,nlen);
					free(nbuf);
					nbuf=tptr;
				} else {
					ErrorCode=MemoryError;
				}
			}
			if (data=checkIncludes(nbuf)){
				free(nbuf);
				if (data[0]){
					memcpy(&number,data+1,data[0]);
				}
			} else if (gt=findLabel(nbuf)){
				free(nbuf);
				if (gt->bytes&0x40){
					ErrorCode = UndefinedLabelError;
				} else {
					number = getLabelValue(gt);
				}
			} else {
				ErrorCode = UndefinedLabelError;
			}
		} else {
			ErrorCode = MemoryError;
		}
		return number;
	}
	while (c=*(*line)++){
		if (c=='.') {
			c=*(*line)++;
			if (c=='X'){
				*base = 16;
			} else if (c=='O') {
				*base = 8;
			} else if (c=='D') {
				*base = 10;
			} else if (c=='B') {
				*base = 2;
			} else {
				ErrorCode = "Invalid Number Base";
				return 0;
			}
		} else if (c=='-'){
			neg=1;
		} else if ((a=digitValue(c))<(*base)) {
			number = number*(*base) + a;
		} else if (c!=' '){
			break;
		}
	}
	(*line)--;
	if (neg) number = -number;
	return number;
}

int digitValue(char c){
	uint8_t a;
	if ((a=c-0x30)<10){
		return a;
	} else if ((a=c-0x41)<27){
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
	if (amt){
		if (rv=malloc((amt=(int)(ptr-*line))+1)){
			memcpy(rv,*line,amt);
			rv[amt] = 0;
		} else {
			ErrorCode = MemoryError;
			return 0;
		}
	} else {
		return 0;
	}
	*line = ptr;
	return rv;
}

