
include 'include/ez80.inc'
include 'include/tiformat.inc'
include 'include/basm.inc'

entry_len:=8                   ;this is the length of all the constant name fields in this file

header:
db "BASM3.0 INC",0             ;the file header. Required
db (32 + header - $) dup 0     ;first 32 bytes of the file are ignored. Useful for metadata.
dl entry_len                   ;this is the length of all the constant name fields in this file
tbl:                           ;jump table to each letter, offset in the file.
dl tbl_A
dl tbl_B
dl tbl_C
dl tbl_D
dl tbl_E
dl tbl_F
dl tbl_G
dl tbl_H
dl tbl_I
dl tbl_J
dl tbl_K
dl tbl_L
dl tbl_M
dl tbl_N
dl tbl_O
dl tbl_P
dl tbl_Q
dl tbl_R
dl tbl_S
dl tbl_T
dl tbl_U
dl tbl_V
dl tbl_W
dl tbl_X
dl tbl_Y
dl tbl_Z


