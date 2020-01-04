
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

char *processOpcodeLine(const char *name){
	char c,c2;
	char *rv;
	char *ptr;
	int len;
	if (rv=ptr=malloc(len=strlen(name))){
		memset(ptr,0,len);
		while ((c=*name++)&&c!=' ');
		if (c){
			while (c=*name++){ //loop over the line
				if (isRegister(name-1)){ //check if it's a register. Hopefuly this can be optimized at some point.
					*ptr++=c;
					*ptr++=c2;
					if ((c2=*(++name))=='H'||c2=='L'){ //ixh/l
						name++;
						*ptr++=c2;
					}
				} else if ((c-0x41)<26||((c-0x30)<10)) { //if it's an alphanumeric constant we need to skip it in order to match the opcode.
					while ((c=*name++)&&((c-0x41)<26||(c-0x30)<10)); //skip until it's not
					*ptr++='#'; //write the placeholder
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

int getArgFromLine(const char *line){
	char c;
	while ((c=*line++)&&c!=' ');
	if (c){
		char *data;
		int len;
		while (isRegister(line)||(*line-0x30)>=10||(*line-0x41)>=26) line++; //repeat until it's a value
		len=strlen(line)+1;
		if (data=malloc(len)){
			memcpy(data,line,len);
			return getNumberWrapper(&data);
		} else {
			ErrorCode = MemoryError;
			return 0;
		}
	}
	return 0;
}

uint8_t *checkInternal(const char *line){
	define_entry_t *ptr;
	uint8_t buf[6];
	uint8_t *emit;
	ptr=internal_define_pointers[*line-0x41];
	while ((ptr++)->bytes&&(!ptr->flags&F_DIRECT_CMP)){
		if (!strncmp(line,ptr->opcode,16)){
			memcpy(&buf,&ptr->bytes,ptr->bytes+1);
			emit=&buf;
			break;
		}
	}
	if (emit) return emit;
	return 0;
}

void emitArgument(uint8_t *buf,const char *line,uint8_t flags){
	if (!flags&F_DIRECT_CMP){
		int num = getArgFromLine(line);
		if (flags&F_OFFSET_ARG){
			*buf = num;
		} else if (flags&F_ARG_BYTE){
			*buf = num;
		} else if (flags&F_LONG_ARG){
			memcpy(buf,&num,3);
		}
	}
}

bool isRegister(const char *name){
	char c,c2,c3;
	c=name[0]; c2=name[1]; c3=name[2];
	return (((c=='H' && c2=='L')||(c=='D'||c2=='E')||(c=='B'&&c2=='C')||(c=='A'&&c2=='F')||
	(c=='S'&&c2=='P')||(c=='I'&&(c2=='X'||c2=='Y')))&&((c3-0x41)<26))
	||(c=='A'&&((c2-0x41)<26));
}

int getNumberWrapper(char **line){
	LAST_LINE = *line;
	return getNumber(line);
}


int getNumber(char **line){
	unsigned char c,c2;
	int number;
	uint8_t base;
	base=10;
	if ((c=*(*line))=='('){
		(*line)++;
		number = getNumber(line);
		c=*(*line);
	} else {
		number = 0;
	}
	if (c<0x30||c>0x39){
		uint8_t *data;
		char *nbuf;
		if (!(nbuf=getWord(line))){
			ErrorCode = MemoryError;
		} else if (data=checkIncludes(nbuf)){
			if (data[0]){
				memcpy(&number,data+1,data[0]);
				return number;
			}
		} else {
			defineGoto(nbuf,0);
			setGotoOffset(nbuf);
		}
		return 0;
	}
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
			number -= getNumber(line);
		} else if (c=='+') {
			number += getNumber(line);
		} else if (c=='/') {
			number /= getNumber(line);
		} else if (c=='*') {
			number *= getNumber(line);
		} else if (c=='>') {
			if ((c2=*(*line))=='>'){
				(*line)++;
				number >>= getNumber(line);
			} else if (c2=='='){
				number = number >= getNumber(line);
			} else {
				number = number > getNumber(line);
			}
		} else if (c=='<'){
			if ((c2=*(*line))=='<'){
				(*line)++;
				number <<= getNumber(line);
			} else if (c2=='=') {
				number = number <= getNumber(line);
			} else {
				number = number < getNumber(line);
			}
		} else if (c=='!'){
			if ((c2=*(*line))=='A'){
				number = number && getNumber(line);
			} else if (c2=='O'){
				number = number || getNumber(line);
			} else if (c2=='X'){
				number = !(number && getNumber(line));
			} else if (c2=='+'){
				number &= getNumber(line);
			} else if (c2=='-') {
				number |= getNumber(line);
			} else if (c2=='*') {
				number ^= getNumber(line);
			} else if (c2=='M') {
				number %= getNumber(line);
			} else {
				ErrorCode = "Syntax Error: Expected logical operator";
				return 0;
			}
		} else if (c==')' || c==',' || c==' '){
			(*line)++;
			return number;
		} else {
			uint8_t a = digitValue(c);
			if (a<base){
				number = number*base + a;
			} else {
				ErrorCode = "Number Format Error";
				return 0;
			}
		}
	}
	return number;
}



uint8_t checkRRArg(const char *args,uint8_t base){
	uint8_t c1,c2,o,i;
	c1 = args[0];
	c2 = args[1];
	if (c1=='S' && c2=='P')      o=0x30;
	else if (c1=='H' && c2=='L') o=0x20;
	else if (c1=='D' && c2=='E') o=0x10;
	else {
		if (c1!='I'){
			if (!(c1=='B' && c2=='C')) return 0;
			o=0x00;
		}
	}
	if (c1=='I'){
		buffer[2] = base+0x20;
		if (c2=='X')      buffer[1]=0xDD;
		else if (c2=='Y') buffer[1]=0xFD;
		buffer[0] = 2;
	} else {
		buffer[1] = base+o;
		buffer[0] = 1;
	}
	return buffer[0];
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

uint8_t checkRArg(const char *args,uint8_t base){
	uint8_t c=getRArgN(args);
	if (c==0xFF) return 0;
	if (args[1]=='X'){
		buffer[1]=0xDD;
	} else if (args[1]=='Y'){
		buffer[1]=0xFD;
	} else {
		buffer[1]=base+c;
		return (buffer[0]=1);
	}
	buffer[2]=base+c;
	return (buffer[0]=2);
}

uint8_t getRArgN(const char *args){
	uint8_t c,o;
	c = args[0]; o=0xFF;
	if (!strncmp(args,"(HL)",4)){
		o=6;
	} else if (c=='I'){
		if (args[2]=='H')      o=4;
		else if (args[2]=='L') o=5;
		if ((c=args[3])!=',' && c!=' ' && c){
			return 0xFF;
		}
	} else {
		uint8_t c2;
		if ((c2=args[1])!=',' && c2!=' ' && c2) return 0xFF;
		else if (c=='A') o=7;
		else if (c=='B') o=0;
		else if (c=='C') o=1;
		else if (c=='D') o=2;
		else if (c=='E') o=3;
		else if (c=='H') o=4;
		else if (c=='L') o=5;
	}
	return o;
}

uint8_t getCondition(const char **line){
	uint8_t c;
	if (!strncmp(*line,"NZ,",3)){
		c=0x00;
		(*line)+=3;
	} else if (!strncmp(*line,"NC,",3)){
		c=0x10;
		(*line)+=3;
	} else if (!strncmp(*line,"Z,",2)){
		c=0x08;
		(*line)+=2;
	} else if (!strncmp(*line,"C,",2)){
		c=0x18;
		(*line)+=2;
	} else if (!strncmp(*line,"PO,",3)) {
		c=0x20;
		(*line)+=3;
	} else if (!strncmp(*line,"PE,",3)) {
		c=0x28;
		(*line)+=3;
	} else if (!strncmp(*line,"P,",2)) {
		c=0x30;
		(*line)+=3;
	} else if (!strncmp(*line,"M,",2)) {
		c=0x38;
		(*line)+=3;
	} else return 0xFF;
	return c;
}

char *getWord(const char **line){
	char c;
	char *rv;
	int amt;
	char *ptr = *line;
	while ((c=*(ptr++))>0x40&&c<=0x5A);
	if (rv=malloc((amt=(int)ptr-(int)*line))+1){
		memcpy(rv,*line,amt);
		rv[amt] = 0;
	} else {
		ErrorCode = MemoryError;
	}
	*line = ptr;
	return rv;
}

