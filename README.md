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
`TIOSKBEQ.8xv` -> Keyboard scan codes



The opcodes of BASM are the same as when using "eZ80.inc" with fasmg.
BASM allows comment lines, trailing comments, leading spaces, and extra spaces between words. (hopefuly)

Example program:
```
FORMAT ASM "BIN"
 JP MAIN
 DB 2
 DB "BASM-3.0A Example program",0
//This is a comment
//Lbl is an alternative label define. Useful for jumping around the program using Cesium's in-editor Label Goto feature
//Include appvar TI84PCEG with the prefix TI.
INCLUDE "TI84PCEG" TI.
Lbl MAIN:
 CALL TI.RUNINDICOFF
 CALL TI.BOOTθCLEARVRAM
 XOR A,A
 LD (TI.CURROW),A
 LD (TI.CURCOL),A
 CALL TI.BOOTθPUTS
 CALL GETKEY
 CALL TI.RUNINDICON
 JP TI.DRAWSTATUSBAR
GETKEY:
 CALL TI.GETCSC
 OR A,A
 JR Z,.
RET
```


# Numbers in BASM:
Any opcode/word/define that requires arguments can take any expression that resolves to a static value. This is what the "Filling addresses" step is for. Expressions are standard, but do not follow order of operations. The order is linearily forward.
Immediate numeric values can be any of the 4 common number bases. Decimal (default), Hexadecimal, Octal, and Binary.
Number bases are denoted with the exponentation 'E'.
`669` -> Decimal 669.
`EX0ff` -> Hexadecimal FF.
`EO007` -> Octal 7.
`EB0011` -> Binary 11.
`ED996` -> Decimal 996.


# Offset numbers in BASM:
For any opcode/word/define that requires an argument you may use `+X` and `-X` to denote that the number is an offset of the current executing address.
Ex:
`JR +3`


# Labels in BASM:
Any alphanumeric text, including dot '.' and theta 'θ', starting with a letter, followed by a colon ':' will define a label.
You may also start this with the `Lbl ` token (or the letters, but that would defeat the purpose) so that you may use Cesium's in-editor label jumping feature.
If this text begins with a dot '.' then it will be appended to the current namespace, creating a local label.
As long as you are within the namespace that it is defined, you can use this text as a shorthand for this label, without labels of the same name elsewhere conflicting with it.
If the label name is only a dot '.' then it refers to the current namespace.
Ex.
```
 LD A,2
LOOP:
 LD B,5
.LOOP:
 DJNZ .LOOP
 DEC A
 JR NZ,.
```
From outside that namespace, you will need to use it's full name. The namespace appended by the local label name.
Following the colon can come the end of line, which will set the label value to it's offset from the output origin plus the origin. This also sets the current namespace. Local labels do not set the namespace.
There are three other methods of setting labels, but these also do not set the current namespace.
- '='	set the label value to the following definite expression.
- '-'	set the label value to the current offset plus the current origin minus the following definite expression.
- '+'	set the label value to the current offset plus the current origin plus the following definite expression.
All labels, words, and opcodes are case-insensitive for the time being, this might change soon.


# BASM built-in words:
`FORMAT ASM "PROGRAM"`-> The output is an Asm, protected program.
`FORMAT ASM ARCHIVED "PROGRAM"`-> The output is an Archived, Asm, Protected program.
`FORMAT APPVAR "APPVAR"` -> The output is an appvar.
`FORMAT PRGM "PROGRAM"` -> The output is a protected program.
`ORIGIN 0` -> Set the current origin to 0.
`INCLUDE "APPVAR" PREFIX`-> Includes "APPVAR" with the prefix PREFIX. Prefixes are useful for speeding up assembly time, but are optional.
`DB 0,"Hi"`-> Write bytes into the output file.
`DW 0,1,2`-> Write words (2 byte expressions) into the output file.
`DL 0,100,.XFF0010` -> Write longs (3 byte expressions) into the output file.


# BASM code control:
`SECTION` -> starts a new code section. This allows the current code execution address to advance as the code is written to the output.
`SECTION AT addr` -> start a new code section and set it's code origin to `addr`.
`END SECTION` -> End a code section.
`VIRTUAL` -> start a new virtual code section. This does not affect the current code execution address as this code is written to the file. Useful for stubs that can be copied into their execution base address at runtime.
`VIRTUAL AT addr` -> Start a new virtual code section and set it's code origin to `addr`.
`END VIRTUAL` -> End a virtual code section.
`REPEAT n` -> Repeat a block of code `n` times
`END REPEAT` -> End a repeating block of code
`RESERVE n` -> Reserve `n` bytes for a block of code if that block of code is smaller than `n` bytes. This pads the remaining space with zeroes.
`END RESERVE` -> End a reserved block


# BASM preprocessors:
Any numeric expression can contain a preprocessor.
These words must start with the 'pi' character.
`πPUSH` -> push the following value to the stack, also sets the current value in the expression to the following value.
Ex. `1 + πPUSH 1` -> push 1 to the stack, add 1+1, return 2.
`πPOP` -> pop the stack into the current value in the expression.
Ex. `1 + πPOP` -> pop the stack and add 1, return the result.

These can be useful for setting and restoring the `ORIGIN` variable.
Ex. `ORIGIN πPUSH 0` -> push the current origin, set the origin to 0.
    `//some code`
    `ORIGIN πPOP` -> pop the stack to set the origin to what it was before.

`πECHO` -> Print the following numeric value
`πPUTS` -> Print the following string value


# Opcode prefixes
There are four opcode prefixes that can be used in BASM at this moment.
They are usually called suffixes, but I couldn't get that kind of syntax to work.
In BASM the opcode prefixes can be used on a line alone, or they can be put before the opcode.
Either way, the next opcode will be affected by the prefix.
`SIS`, `SIL`, `LIS`, and `LIL` are the four supported prefixes.
ex:
`lis add hl,hl` assembles to $49,$29.


