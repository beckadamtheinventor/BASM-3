# BASM-3
Beck's Assembler v3 - "On-calc" Assembler for the TI84+CE family of graphing calculators.

# TODO
- wring out bugs as they appear

# How-to

Send `BASM.8xp`, and `BASMdata.8xv` to the calculator.
Type in the program name you wish to compile from the TI-OS homescreen like so:
`"PROGRAM"`
Press enter, then run BASM.
If all is successful, you should see the output binary. Otherwise BASM will tell you what went wrong.

# includes
The rest of the appvars included with BASM are include files.
`include "TI84PCEG" TIθ`
This will allow your program to reference the standard ti84pce.inc defines, using the namespace `TIθ`

# include files and their contents
`TI84PCEG.8xv` -> Most of ti84pce.inc
`TIOSRTNS.8xv` -> OS routine calls
`TIOSRAMA.8xv` -> RAM areas and ports
`TIOSFLAG.8xv` -> OS Flags


The opcodes of BASM are the same as when using "eZ80.inc" with fasmg.
Local labels are not yet implemented. Stay tuned.
Numbers and expressions are standard, but do not follow order of operations.
If a label name starts at the beginning of a line and the name ends with a colon (':') this will define a label.
Following such a colon can come one of four things.
- ''	set the label value to the current offset plus the current origin. (end of line)
- '='	set the label value to the following definite expression.
- '-'	set the label value to the current offset plus the current origin minus the following definite expression.
- '+'	set the label value to the current offset plus the current origin plus the following definite expression.
All labels, words, and opcodes are case-insensitive.


Example program:
```
FORMAT ASM "BIN"
JP MAIN
DB 2
DB "BASM-3.0A Example program",0
//This is a comment
//Lbl is an alternative label define. Useful for jumping around the program using Cesium's in-editor Label Goto feature
//Include appvar TI84PCEG with the namespace TIθ
INCLUDE "TI84PCEG" TIθ
Lbl MAIN:
CALL TIθRUNINDICOFF
CALL TIθBOOTθCLEARVRAM
XOR A,A
LD (TIθCURROW),A
LD (TIθCURCOL),A
CALL TIθBOOTθPUTS
CALL GETKEY
CALL TIθRUNINDICON
JP TIθDRAWSTATUSBAR
GETKEY:
CALL TIθGETCSC
OR A,A
JR Z,GETKEY
RET
```

# BASM built-in words:
`FORMAT ASM "PROGRAM"`-> The output is an Asm, protected program.
`FORMAT ASM ARCHIVED "PROGRAM"`-> The output is an Archived, Asm, Protected program.
`FORMAT APPVAR "APPVAR"` -> The output is an appvar.
`FORMAT PRGM "PROGRAM"` -> The output is a protected program.
`ORIGIN 0` -> The output's label origin is 0.
`INCLUDE "APPVAR" NAMESPACE`-> Includes "APPVAR" with the namespace prefix NAMESPACE. Namepaces are useful for speeding up assembly time, but are optional.
`DB 0,"Hi"`-> Write bytes into the output file.
`DW 0,1,2`-> Write words (2 byte expressions) into the output file.
`DL 0,100,.XFF0010` -> Write longs (3 byte expressions) into the output file.


