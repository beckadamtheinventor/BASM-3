# BASM-3
Beck's Assembler v3 - "On-calc" Assembler for the TI84+CE family of graphing calculators.

# TODO
- make more include files
- wring out bugs as they appear

# How-to

The opcodes of BASM are the same as with fasmg. (eZ80 version)
Local labels are not yet implemented. Stay tuned.
Numbers and expressions are standard, but do not follow order of operations.
If a label name starts at the beginning of a line and the name ends with a colon (':') this will define a label.
Following such a colon can come one of four things.
- EOL	set the label value to the current offset plus the current origin.
- '='	set the label value to the following definite expression.
- '-'	set the label value to the current offset plus the current origin minus the following definite expression.
- '+'	set the label value to the current offset plus the current origin plus the following definite expression.


Example program:
```
FORMAT ASM "BIN"
JP MAIN
DB 2
DB "BASM-3.0A Example program",0
//This is a comment
//Lbl is an alternative label define. Useful for jumping around the program using Cesium's in-editor Label Goto feature
Lbl MAIN:
CALL RUNINDICOFF
CALL BOOT[CLEARVRAM
XOR A,A
LD (CURROW),A
LD (CURCOL),A
CALL BOOT[PUTS
CALL GETKEY
CALL RUNINDICON
JP DRAWSTATUSBAR
GETKEY:
CALL GETCSC
OR A,A
JR Z,GETKEY
RET
KCLEAR:=15
```

Note: the '[' in the examples should be Theta in your program.

