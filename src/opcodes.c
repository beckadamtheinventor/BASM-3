
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

void clearBuffer(void){
	memset(&buffer,0,buffer_len);
}

uint8_t *OpcodesA(const char *line){
	if (!strncmp(line,"ADD ",4)){
		if (checkRRArg(line+4,0x09)) {
			return &buffer;
		} else if (checkRArg(line+4,0x80)){
			return &buffer;
		} else {
			return invalidArgument();
		}
	} else if (!strncmp(line,"ADC ",4)) {
		if (checkRRArg(line+4,0x4A)){
			if (buffer[1]==0xDD || buffer[1]==0xFD){
				return invalidArgument();
			} else {
				buffer[2] = buffer[1]+0x41;
				buffer[1] = 0xED;
			}
			return &buffer;
		} else if (checkRArg(line+4,0x88)){
			return &buffer;
		}
	} else if (!strncmp(line,"AND ",4)) {
		if (isNumber(line+4)){
			line+=4;
			buffer[0]=2;
			buffer[1]=0xE6;
			buffer[2]=getNumberWrapper(&line);
			return &buffer;
		} else if (checkRArg(line+4,0xA0)){
			return &buffer;
		} else {
			return invalidArgument();
		}
	}
	return 0;
}

uint8_t *OpcodesB(const char *line){
	if (!strncmp(line,"BIT ",4)){
		uint8_t r,bit;
		line+=4;
		bit=getNumberWrapper(&line);
		if (bit<=7){
			if (r=isIrOff(line)){
				buffer[0]=4;
				buffer[1]=r;
				buffer[2]=0xCB;
				buffer[3]=getIrOff(&line);
				buffer[4]=0x46+(bit<<3);
			} else if ((r=getRArgN(line))!=0xFF){
				buffer[0]=2;
				buffer[1]=0xCB;
				buffer[2]=0x40+r+(bit<<3);
				return &buffer;
			}
		}
		return invalidArgument();
	}
	return 0;
}

uint8_t *OpcodesC(const char *line){
	if (!strncmp(line,"CP ",3)){
		if (isNumber(line+3)){
			CURRENT_BYTES = 1;
			line+=3;
			buffer[0]=2;
			buffer[1]=0xFE;
			buffer[2] = getNumberWrapper(&line);
			return &buffer;
		} else if (checkRArg(line+3,0xB8)){
			return &buffer;
		}
	} else if (!strncmp(line,"CPL",3)){ 
		buffer[0]=1;
		buffer[1]=0x2F;
		return &buffer;
	} else if (!strncmp(line,"CCF",3)){
		buffer[0]=1;
		buffer[1]=0x3F;
		return &buffer;
	} else {
		buffer[0]=2;
		buffer[1]=0xED;
		if (!strncmp(line,"CPIR",4)){
			buffer[2]=0xB1;
			return &buffer;
		} else if (!strncmp(line,"CPDR",4)){
			buffer[2]=0xB9;
			return &buffer;
		} else if (!strncmp(line,"CPI",3)){
			buffer[2]=0xA1;
			return &buffer;
		} else if (!strncmp(line,"CPD",3)){
			buffer[2]=0xA9;
			return &buffer;
		}
		buffer[0]=0;
	}
	return 0;
}

uint8_t *OpcodesD(const char *line){
	uint8_t o;
	if (!strncmp(line,"DEC ",4)){
		line+=4;
		if (checkRRArg(line,0x0B)){
			return &buffer;
		} else if ((o = getRArgN(line))!=0xFF){
			if (buffer[0]==1){
				buffer[1]=(o<<3)+0x05;
			} else {
				buffer[2]=(o<<3)+0x05;
			}
			return &buffer;
		} else {
			return invalidArgument();
		}
	} else if (!strncmp(line,"DJNZ ",5)){
		int num;
		CURRENT_BYTES = 1;
		line+=5;
		num = getNumberWrapper(&line);
		if (num<-0x80 || num>=0x80){
			ErrorCode = "DJNZ Offset out of range";
			return 0;
		}
		buffer[0]=2;
		buffer[1]=0x10;
		buffer[2]=num;
	} else if (!strncmp(line,"DAA",3)){
		buffer[0]=1;
		buffer[1]=0x27;
		return &buffer;
	} else if (!strncmp(line,"DB ",3)){
		uint8_t buf2[65];
		uint8_t i=1;
		CURRENT_BYTES = 1;
		line+=3;
		do {
			buf2[i++] = getNumberWrapper(&line);
			if (i>=64){
				ErrorCode = "DB takes at most 64 args";
				break;
			}
		} while (*(line-1)==',');
		buf2[0]=i;
		return &buf2;
	} else if (!strncmp(line,"DW ",3)){
		uint16_t buf2[65];
		uint8_t i=1;
		CURRENT_BYTES = 2;
		line+=3;
		do {
			buf2[i++] = getNumberWrapper(&line);
			if (i>64){
				ErrorCode = "DW takes at most 64 args";
				break;
			}
		} while (*(line-1)==',');
		buf2[0]=i<<8;
		return (uint8_t*)&buf2+1;
	} else if (!strncmp(line,"DL ",3)){
		unsigned int buf2[65];
		uint8_t i=1;
		CURRENT_BYTES = ADDR_BYTES;
		line+=3;
		do {
			buf2[i++] = getNumberWrapper(&line);
			if (i>64){
				ErrorCode = "DL takes at most 64 args";
				break;
			}
		} while (*(line-1)==',');
		buf2[0]=i<<16;
		return (uint8_t*)&buf2+2;
	} else if (!strncmp(line,"DI",2)){
		buffer[0]=1;
		buffer[1]=0xF3;
		return &buffer;
	}
	return 0;
}

uint8_t *OpcodesE(const char *line){
	if (!strncmp(line,"EX ",3)){
		buffer[0]=1;
		line+=3;
		if (!strncmp(line,"HL,DE",5)){
			buffer[1]=0xEB;
		} else if (!strncmp(line,"HL,(SP)",7)){
			buffer[1]=0xE3;
		} else if (!strncmp(line,"AF",2)) {
			buffer[1]=0x08;
		} else {
			return invalidArgument();
		}
		return &buffer;
	} else if (!strncmp(line,"EXX",3)){
		buffer[0]=1;
		buffer[1]=0xD9;
		return &buffer;
	} else if (!strncmp(line,"EI",2)){
		buffer[0]=1;
		buffer[1]=0xFB;
		return &buffer;
	}
	return 0;
}

uint8_t *OpcodesF(const char *line){
	if (!strncmp(line,"FORMAT ",7)){
		line+=7;
		if (!strncmp(line,"ASM",3)){
			buffer[0]=2;
			buffer[1]=0xEF;
			buffer[2]=0x7B;
			return &buffer;
		} else {
			ErrorCode = "Bad Format";
		}
	}
	return 0;
}

uint8_t *OpcodesI(const char *line){
	if (!strncmp(line,"INC ",4)){
		uint8_t o;
		line+=4;
		if (checkRRArg(line,0x03)){
			
		} else if ((o = getRArgN(line))!=0xFF){
			if (buffer[0]==1){
				buffer[1]=(o<<3)+0x04;
			} else {
				buffer[2]=(o<<3)+0x04;
			}
		} else {
			return invalidArgument();
		}
	} else if (!strncmp(line,"IN0 ",4)){
		uint8_t o;
		line+=4;
		if ((o = getRArgN(line))!=0xFF){
			if (buffer[0]==2){
				return invalidArgument();
			} else {
				CURRENT_BYTES = 1;
				buffer[0]=3;
				buffer[1]=0xED;
				buffer[2]=o<<3;
				buffer[3]=getNumberWrapper(&line);
			}
		}
	} else if (!strncmp(line,"INIMR",5)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x92;
	} else if (!strncmp(line,"INI2R",5)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x94;
	} else if (!strncmp(line,"INDMR",5)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x9A;
	} else if (!strncmp(line,"IND2R",5)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x9C;
	} else if (!strncmp(line,"INIM",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x82;
	} else if (!strncmp(line,"INI2",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x84;
	} else if (!strncmp(line,"INDM",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x8A;
	} else if (!strncmp(line,"IND2",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x8C;
	} else if (!strncmp(line,"IN ",3)){
		uint8_t r = getRArgN(line+3);
		if (r!=0xFF){
			buffer[0]=2;
			buffer[1]=0xED;
			buffer[2]=0x40+(r<<3);
		}
	} else if (!strncmp(line,"INIR",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xB2;
		
	} else if (!strncmp(line,"INDR",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xBA;
		
	} else if (!strncmp(line,"INI",3)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xA2;
	} else if (!strncmp(line,"IND",3)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xAA;
	} else if (!strncmp(line,"INCLUDE ",8)){
		if (includeFile(line+8)){
			buffer[0]=0;
		} else {
			ErrorCode = "failed to include appvar";
			return 0;
		}
	}
	return &buffer;
}

uint8_t *OpcodesJ(const char *line){
	if (!strncmp(line,"JR ",3)){
		int num;
		uint8_t c;
		line+=3;
		buffer[0]=2;
		if (!strncmp(line,"NZ,",3)){
			buffer[1]=0x20;
			line+=3;
		} else if (!strncmp(line,"NC,",3)){
			buffer[1]=0x30;
			line+=3;
		} else if (!strncmp(line,"Z,",2)){
			buffer[1]=0x28;
			line+=2;
		} else if (!strncmp(line,"C,",2)){
			buffer[1]=0x38;
			line+=2;
		} else if (isNumber(line)) {
			buffer[1]=0x18;
		} else {
			return invalidArgument();
		}
		CURRENT_BYTES = 1;
		num=getNumberWrapper(&line);
		if (num<-0x80 || num>=0x80){
			ErrorCode = "JR Offset out of range";
			return 0;
		}
		buffer[2]=num;
		return &buffer;
	} else if (!strncmp(line,"JP ",3)){
		int num;
		uint8_t cc;
		line+=3;
		CURRENT_BYTES = ADDR_BYTES;
		if (isNumber(line)){
			buffer[1]=0xC3;
		} else {
			if ((cc=getCondition(&line))==0xFF){
				return 0;
			}
			buffer[1]=0xC2+cc;
		}
		num = getNumberWrapper(&line);
		memcpy(buffer+2,&num,ADDR_BYTES);
		buffer[0]=ADDR_BYTES+1;
		return &buffer;
	}
	return 0;
}

uint8_t *OpcodesL(const char *line){
	char *oldline;
	if (!strncmp(line,"LD ",3)){
		uint8_t o,iro;
		line+=3;
		if (!strncmp(line,"(HL),",5)){ //ld (hl)
			line+=5;
			if (checkRArg(line,0x70)){ //ld (hl),r
				//return &buffer; optimize!
			} else if (isNumber(line)){ //ld (hl),$00
				CURRENT_BYTES = 1;
				buffer[0]=2;
				buffer[1]=0x36;
				buffer[2]=getNumberWrapper(&line);
			} else if (checkRRArg(line,0x0F)){ //ld (hl),rr
				if (buffer[1]==0xDD){
					buffer[2]=0x3F;
				} else if (buffer[1]==0xFD){
					buffer[2]=0x3E;
				} else {
					buffer[0]=2;
					buffer[2]=buffer[1];
				}
				buffer[1]=0xED;
			} else {
				return invalidArgument();
			}
		} else if (*line=='(' && isNumber(oldline=line+1)){ //ld ($000000),A
			int num = getNumberWrapper(&line);
			CURRENT_BYTES = ADDR_BYTES;
			line++;
			if (*line=='A'){
				buffer[0]=ADDR_BYTES+1;
				buffer[1]=0x32;
				memcpy(&buffer[2],&num,ADDR_BYTES);
			} else if (!checkRRArg(line,0x43)){ //ld ($000000),rr
				return invalidArgument(); //this acts as an "else" case
			}
		} else if ((o=getRArgN(line))!=0xFF){ //ld r,X
			uint8_t irc;
			if (line[0]=='I'){ //ixh/l
				if (line[1]=='X'){
					irc=0xDD;
				} else {
					irc=0xFD;
				}
				line+=4;
			} else {
				irc = 0;
				line+=2;
			}
			if (*line=='(' && isNumber(line+1) && o==7){ //ld A,($000000)
				int num;
				CURRENT_BYTES = ADDR_BYTES;
				line++;
				buffer[0] = ADDR_BYTES+1;
				buffer[1] = 0x3A;
				num = getNumberWrapper(&line);
				memcpy(&buffer[2],&num,ADDR_BYTES);
			} else if (isNumber(line)){ //ld r,$00
				uint8_t num = getNumberWrapper(&line);
				CURRENT_BYTES = 1;
				if (irc){ //ld ixh/l,$00
					buffer[0]=3;
					buffer[1]=irc;
					buffer[2]=(o<<3)+0x06;
					buffer[3]=num;
				} else { //ld r,$00
					buffer[0]=2;
					buffer[1]=(o<<3)+0x06;
					buffer[2]=num;
				}
			} else if (iro = isIrOff(line)){ //ld r,(ir+dd)
				line+=3;
				CURRENT_BYTES = 1;
				if (buffer[0]==2){
					return invalidArgument(); //cant ``ld ixh/l,(ir+dd)``
				}
				if (*line=='+'){
					line++;
					buffer[3]=getNumberWrapper(&line);
				} else if (*line=='-'){
					line++;
					buffer[3]=-getNumberWrapper(&line);
				} else {
					return invalidArgument();
				}
				buffer[0]=3;
				buffer[1]=iro;
				buffer[2]=0x06+(o<<3);
			} else if (checkRArg(line,0x40)){ //ld r,r
				if (buffer[0]==2){
					buffer[2]+=o<<3;
				} else {
					buffer[1]+=o<<3;
				}
			} else {
				return invalidArgument();
			}
		} else if (checkRRArg(line,0x01)){ //ld rr,X
			line+=3;
			if (*line=='(' && isNumber(line+1)){ //ld rr,($000000)
				int num;
				CURRENT_BYTES = ADDR_BYTES;
				line++;
				getNumberWrapper(&line);
				if (buffer[0]==2){ //ld ir,($000000)
					buffer[2]=0x2A;
				} else { //ld rr,($000000)
					int num = getNumberWrapper(&line);
					if (buffer[1]==0x21){ //ld hl,($000000)
						buffer[1]=0x2A;
						memcpy(&buffer[2],&num,ADDR_BYTES);
						buffer[0]=ADDR_BYTES+1;
					} else { //ld rr,($000000)
						buffer[2]=buffer[1]+0x4A;
						buffer[1]=0xED;
						memcpy(&buffer[3],&num,ADDR_BYTES);
						buffer[0]=ADDR_BYTES+2;
					}
				}
			} else if (isNumber(line)){ //ld rr,$000000
				int num = getNumberWrapper(&line);
				CURRENT_BYTES = ADDR_BYTES;
				if (buffer[0]==2){
					memcpy(&buffer[3],&num,ADDR_BYTES);
					buffer[0] = ADDR_BYTES+2;
				} else {
					memcpy(&buffer[2],&num,ADDR_BYTES);
					buffer[0] = ADDR_BYTES+1;
				}
			} else if (!strncmp(line,"(HL)",4)){ //ld rr,(hl)
				if (buffer[0]==2) {
					if (buffer[1]==0xDD){
						buffer[2]=0x36;
					} else {
						buffer[2]=0x37;
					}
				} else {
					buffer[0]=2;
					buffer[2]=buffer[1];
				}
				buffer[1]=0xED;
			} else if (iro=isIrOff(line)) { //ld rr,(ir+dd)
				uint8_t dd=getIrOff(&line);
				CURRENT_BYTES = 1;
				if (buffer[0]==2){ //ld ir,(ir+dd)
					if (buffer[1]==0xDD){ //ld ix,(ir+dd)
						buffer[2]=0x31;
					} else { //ld iy,(ir+dd)
						buffer[2]=0x37;
					}
				} else { //ld rr,(ir+dd)
					buffer[2]=buffer[1]+7;
				}
				buffer[0]=3;
				buffer[1]=iro;
			} else {
				return invalidArgument();
			}
		} 
	} else if (!strncmp(line,"LDIR",4)){
		buffer[0]=2; buffer[1]=0xED; buffer[2]=0xB0;
	} else if (!strncmp(line,"LDDR",4)){
		buffer[0]=2; buffer[1]=0xED; buffer[2]=0xB8;
	} else if (!strncmp(line,"LDI",3)){
		buffer[0]=2; buffer[1]=0xED; buffer[2]=0xA0;
	} else if (!strncmp(line,"LDD",3)){
		buffer[0]=2; buffer[1]=0xED; buffer[2]=0xA8;
	} else if (!strncmp(line,"LEA ",4)){
		if (checkRRArg(line,0x02)){
			uint8_t cc,c3;
			line+=2;
			if (buffer[0]==2){
				if (buffer[1]==0xDD){
					cc = buffer[2]+0x10;
				} else {
					cc = buffer[2]+0x11;
				}
			}
			if (*line!='I'){
				return invalidArgument();
			}
			line++;
			if ((c3=*line)=='X'){
				if (buffer[1]==0xDD){
					buffer[2]=0x32;
				} else {
					buffer[2]=0x54;
				}
			} else if (c3=='Y') {
				if (buffer[1]==0xFD){
					buffer[2]=0x33;
				} else {
					buffer[2]=0x55;
				}
			} else {
				return invalidArgument();
			}
			CURRENT_BYTES = 1;
			buffer[0]=3;
			buffer[1]=0xED;
			buffer[3] = getIrOff(&line);
		} else {
			return invalidArgument();
		}
	} else {
		return 0;
	}
	return &buffer;
}

uint8_t *OpcodesM(const char *line){
	if (!strncmp(line,"MULT ",5)){
		line+=5;
		if (checkRRArg(line,0x4C)){
			buffer[0]=2;
			buffer[2]=buffer[1];
			buffer[1]=0xED;
			return &buffer;
		} else {
			return invalidArgument();
		}
	}
	return 0;
}
uint8_t *OpcodesN(const char *line){
	if (!strncmp(line,"NEG",3)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x44;
		return &buffer;
	} else if (!strncmp(line,"NOP",3)){
		buffer[0]=1;
		buffer[1]=0;
		return &buffer;
	}
	return 0;
}

uint8_t *OpcodesO(const char *line){
	if (!strncmp(line,"OR ",3)){
		line+=3;
		if (checkRArg(line,0xB0)){
		} else if (isNumber(line)) {
			CURRENT_BYTES = 1;
			buffer[0]=2;
			buffer[1]=0xF6;
			buffer[2]=getNumber(&line);
		} else {
			return invalidArgument();
		}
	} else if (!strncmp(line,"OUT ",4)){
		uint8_t r = getRArgN(line+4);
		if (r!=0xFF){
			buffer[0]=2;
			buffer[1]=0xED;
			buffer[2]=0x41+(r<<3);
		}
	} else if (!strncmp(line,"OUT0 (",6)){
		uint8_t r;
		line+=6;
		CURRENT_BYTES = 1;
		buffer[3] = getNumberWrapper(&line);
		line++;
		r = getRArgN(line);
		if (r!=0xFF){
			buffer[0]=3;
			buffer[1]=0xED;
			buffer[2]=0x01+(r<<3);
		}
	} else if (!strncmp(line,"OTI2R",5)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xB4;
	} else if (!strncmp(line,"OTD2R",5)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xBC;
	} else if (!strncmp(line,"OTIR",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xB3;
	} else if (!strncmp(line,"OTDR",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xBB;
	} else if (!strncmp(line,"OTI2",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xA4;
	} else if (!strncmp(line,"OTD2",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xAC;
	} else if (!strncmp(line,"OTI",3)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xA3;
	} else if (!strncmp(line,"OTD",3)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0xAB;
	} else if (!strncmp(line,"ORG ",4)){
		ORIGIN = getNumberWrapperNoLabels(&line);
		buffer[0]=0;
	} else {
		return 0;
	}
	return &buffer;
}

uint8_t *OpcodesP(const char *line){
	if (!strncmp(line,"POP ",4)){
		if (!checkRRArg(line+4,0xC1)){
			return invalidArgument();
		}
	} else if (!strncmp(line,"PUSH ",5)){
		if (!checkRRArg(line+5,0xC5)){
			return invalidArgument();
		}
	} else if (!strncmp(line,"PEA ",4)){
		uint8_t c;
		line+=4;
		if (c=isIrOff(line)){
			buffer[0]=3;
			buffer[1]=0xED;
			if (c==0xDD){
				buffer[2]=0x65;
			} else {
				buffer[2]=0x66;
			}
			CURRENT_BYTES = 1;
			buffer[3]=getIrOff(&line);
		} else {
			return invalidArgument();
		}
	} else {
		return 0;
	}
	return &buffer;
}

uint8_t *OpcodesR(const char *line) {
	if (!strncmp(line,"RETN",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x45;
		return &buffer;
	} else if (!strncmp(line,"RETI",4)){
		buffer[0]=2;
		buffer[1]=0xED;
		buffer[2]=0x4D;
		return &buffer;
	} else if (!strncmp(line,"RLCA",4)){
		buffer[0]=1;
		buffer[1]=0x07;
		return &buffer;
	} else if (!strncmp(line,"RRCA",4)){
		buffer[0]=1;
		buffer[1]=0x0F;
		return &buffer;
	} else if (!strncmp(line,"RLA",3)){
		buffer[0]=1;
		buffer[1]=0x17;
		return &buffer;
	} else if (!strncmp(line,"RRA",3)){
		buffer[0]=1;
		buffer[1]=0x1F;
		return &buffer;
	} else if (!strncmp(line,"RES ",4)){
		uint8_t r,bit;
		line+=4;
		bit=getNumberWrapperNoLabels(&line);
		if (bit<=7){
			if (r=isIrOff(line)){
				buffer[0]=4;
				buffer[1]=r;
				buffer[2]=0xCB;
				buffer[3]=getIrOff(&line);
				buffer[4]=0x86+(bit<<3);
			} else if ((r=getRArgN(line))!=0xFF){
				buffer[0]=2;
				buffer[1]=0xCB;
				buffer[2]=0x80+r+(bit<<3);
				return &buffer;
			}
		}
		return invalidArgument();
	} else if (!strncmp(line+4,"RLC ",4)){
		if (checkRArg(line,0x00)){
			buffer[0]=2;
			buffer[2]=buffer[1];
			buffer[1]=0xCB;
		}
	} else if (!strncmp(line,"RRC ",4)){
		if (checkRArg(line+4,0x08)){
			buffer[0]=2;
			buffer[2]=buffer[1];
			buffer[1]=0xCB;
		}
	} else if (!strncmp(line,"RL ",3)){
		if (checkRArg(line+3,0x10)){
			buffer[0]=2;
			buffer[2]=buffer[1];
			buffer[1]=0xCB;
		}
	} else if (!strncmp(line,"RR ",3)){
		if (checkRArg(line+3,0x18)){
			buffer[0]=2;
			buffer[2]=buffer[1];
			buffer[1]=0xCB;
		}
	} else if (!strncmp(line,"RST ",4)){
		uint8_t n;
		line+=4;
		n = getNumberWrapperNoLabels(&line);
		if (!(n&7||n>0x38)){
			buffer[0]=1;
			buffer[1]=0xC7+n;
			return &buffer;
		}
		return invalidArgument();
	} else if (!strncmp(line,"RET ",4)){
		uint8_t cc;
		line+=4;
		buffer[0]=1;
		if ((cc=getCondition(&line))==0xFF){
			return 0;
		}
		buffer[1]=0xC0+cc;
		return &buffer;
	} else if (!strncmp(line,"RET",3)){
		buffer[0]=1;
		buffer[1]=0xC9;
		return &buffer;
	}
	return 0;
}

uint8_t *OpcodesS(const char *line){
	if (!strncmp(line,"SUB ",4)){
		line+=4;
		if (checkRArg(line,0x90)){
			return &buffer;
		} else if (isNumber(line)){
			buffer[0]=2;
			buffer[1]=0xD6;
			CURRENT_BYTES = 1;
			buffer[2]=getNumberWrapper(&line);
			return &buffer;
		}
		return invalidArgument();
	} else if (!strncmp(line,"SBC ",4)){
		line+=4;
		if (checkRRArg(line,0x42)){
			buffer[0]=2;
			buffer[2]=buffer[1];
			buffer[1]=0xED;
			return &buffer;
		} else if (checkRArg(line,0x98)){
			return &buffer;
		}
	} else if (!strncmp(line,"SCF",3)){
		buffer[0]=1;
		buffer[1]=0x37;
		return &buffer;
	} else if (!strncmp(line,"SET ",4)){
		uint8_t r,bit;
		line+=4;
		bit=getNumberWrapperNoLabels(&line);
		if (bit<=7){
			if (r=isIrOff(line)){
				buffer[0]=4;
				buffer[1]=r;
				buffer[2]=0xCB;
				buffer[3]=getIrOff(&line);
				buffer[4]=0xC6+(bit<<3);
			} else if ((r=getRArgN(line))!=0xFF){
				buffer[0]=2;
				buffer[1]=0xCB;
				buffer[2]=0xC0+r+(bit<<3);
				return &buffer;
			}
		}
		return invalidArgument();
	} else if (!strncmp(line,"SLA ",4)){
		uint8_t r;
		line+=4;
		if (r=isIrOff(line)){
			buffer[0]=4;
			buffer[1]=r;
			buffer[2]=0xCB;
			buffer[3]=getIrOff(&line);
			buffer[4]=0x26;
		} else if ((r=getRArgN(line))!=0xFF){
			buffer[0]=2;
			buffer[1]=0xCB;
			buffer[2]=0x20+r;
			return &buffer;
		}
	} else if (!strncmp(line,"SRA ",4)){
		uint8_t r;
		line+=4;
		if (r=isIrOff(line)){
			buffer[0]=4;
			buffer[1]=r;
			buffer[2]=0xCB;
			buffer[3]=getIrOff(&line);
			buffer[4]=0x2E;
		} else if ((r=getRArgN(line))!=0xFF){
			buffer[0]=2;
			buffer[1]=0xCB;
			buffer[2]=0x28+r;
			return &buffer;
		}
	} else if (!strncmp(line,"SRL ",4)){
		uint8_t r;
		line+=4;
		if (r=isIrOff(line)){
			buffer[0]=4;
			buffer[1]=r;
			buffer[2]=0xCB;
			buffer[3]=getIrOff(&line);
			buffer[4]=0x36;
		} else if ((r=getRArgN(line))!=0xFF){
			buffer[0]=2;
			buffer[1]=0xCB;
			buffer[2]=0x30+r;
			return &buffer;
		}
	}
	return 0;
}

uint8_t *OpcodesT(const char *line){
	if (!strncmp(line,"TST ",4)){
		uint8_t c;
		line+=4;
		if ((c=getRArgN(line))==0xFF){
			if (isNumber(line)){
				CURRENT_BYTES = 1;
				buffer[1]=0xED;
				buffer[2]=0x64;
				buffer[3]=getNumberWrapper(&line);
			} else {
				return invalidArgument();
			}
		} else {
			buffer[0]=2;
			buffer[1]=0xED;
			buffer[2]=(c<<3)+4;
		}
		return &buffer;
	} else if (!strncmp(line,"TSR ",4)){
		line+=4;
		buffer[0]=3;
		buffer[1]=0xED;
		buffer[2]=0x74;
		CURRENT_BYTES = 1;
		buffer[3]=getNumberWrapper(&line);
		return &buffer;
	}
	return 0;
}

uint8_t *OpcodesX(const char *line){
	if (!strncmp(line,"XOR ",4)){
		line+=4;
		if (checkRArg(line,0xA8)){
		} else if (isNumber(line)){
			buffer[0]=2;
			buffer[1]=0xEE;
			CURRENT_BYTES = 1;
			buffer[2]=getNumberWrapper(&line);
		} else {
			return invalidArgument();
		}
		return &buffer;
	}
	return 0;
}







int getNumberWrapper(char **line){
	LAST_LINE = *line;
	return getNumber(line);
}

int getNumberWrapperNoLabels(char **line){
	int val;
	LAST_LINE = *line;
	ALLOWLABELS = 0;
	val = getNumber(line);
	ALLOWLABELS = 1;
	return val;
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
			return 0;
		}
		if (data=checkIncludes(nbuf)){
			if (data[0]){
				memcpy(&number,data+1,data[0]);
				return number;
			}
		} else {
			if (ALLOWLABELS){
				defineGoto(nbuf,0);
				setGotoOffset(nbuf);
				ErrorCode = 0;
			} else {
				ErrorCode = "Labels can't be used here";
			}
			return 0;
		}
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

int isNumber(const char *line){
	uint8_t c1,c2;
	if (((c1=line[0])=='H' && (c2=line[1])=='L')||(c1=='D' && c2=='E')||
		(c1=='B' && c2=='C')||(c1=='A' && c2=='F')||(c1=='S' && c2=='P')){
		return 0;
	}
	return 1;
}

int isIrOff(const char *line){
	if (!strncmp(line,"(IX",3)){
		return 0xDD;
	} else if (!strncmp(line,"(IY",3)){
		return 0xFD;
	}
	return 0;
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
	if (c=='('){
		if (!strncmp(args+1,"HL)",3)){
			o=6;
		} else {
			o=-1;
		}
	} else if (c=='I'){
		if (args[2]=='H')      o=4;
		else if (args[2]=='L') o=5;
		if ((c=args[3])!=',' && c!=' ' && c){
			return 0xFF;
		}
	} else {
		uint8_t c2;
		if ((c2=args[1])!=',' && c2!=' ' && c2){
			return 0xFF;
		}
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

uint8_t getIrOff(const char **line){
	uint8_t c;
	if ((c=*(*line))=='+'){
		(*line)++;
		return getNumber(line);
	} else if (c=='-'){
		(*line)++;
		return -getNumber(line);
	}
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
	} else if (isNumber(*line)) {
		c=0x09;
	} else {
		invalidArgument();
		return 0xFF;
	}
	return c;
}

uint8_t *invalidArgument(void){
	ErrorCode = "Invalid Argument";
	return 0;
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

