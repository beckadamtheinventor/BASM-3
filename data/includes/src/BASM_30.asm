
include 'include/ez80.inc'
include 'include/tiformat.inc'
include 'include/basm.inc'

entry_len:=17                   ;this is the length of all the constant name fields in this file

header:
db "BASM3.0 INC",0             ;the file header. Required
db (32 + header - $) dup 0     ;first 32 bytes of the file are ignored. Useful for metadata.
dl entry_len                   ;this is the length of all the constant name fields in this file
                              ;jump table to each letter, offset in the file.
dl tbl_A
dl tbl_B
dl tbl_C
dl tbl_D
dl tbl_E
dl tbl_F
dl tbl_G
dl tbl_H
dl tbl_I
dl tbl_None
dl tbl_None
dl tbl_L
dl tbl_M
dl tbl_N
dl tbl_O
dl tbl_P
dl tbl_None
dl tbl_R
dl tbl_S
dl tbl_None
dl tbl_None
dl tbl_V
dl tbl_W
dl tbl_None
dl tbl_None
dl tbl_None

tbl_None:
db 0

tbl_A:
entry "ADDHLANDA",ti.AddHLandA
entry "ARCCHK",ti.ArcChk
db 0

tbl_B:
entry "BLDIY",ti.bldiy
entry "BSHL",ti.bshl
entry "BSHRU",ti.bshru
entry "BSTIX",ti.bstix
entry "BSTIY",ti.bstiy
db 0

tbl_C:
entry "CHJBCIS0",ti.ChkBCIs0
entry "CHKDEIS0",ti.ChkDEIs0
entry "CHKHLIS0",ti.ChkHLIs0
entry "CHKINRAM",ti.ChkInRam
entry "CLEANUPCERT",ti.CleanupCertificate
entry "CLRHEAP",ti.ClrHeap
entry "CPHLDE",ti.CpHLDE
entry "CPHLDEBC",ti.CpHLDEBC
entry "CPHLDES",ti.CpHLDE_s
db 0

tbl_D:
entry "DELETEAPP",ti.DeleteApp
entry "DELETETEMPEDITEQU",ti.DeleteTempEditEqu
entry "DISPHL",ti.DispHL
entry "DISPHLS",ti.DispHL_s
entry "DIVHLBY10S",ti.DivHLBy10_s
entry "DIVHLBYA",ti.DivHLByA
entry "DIVHLBYAS",ti.DivHLByA_s
entry "DRAWAXES",ti.DrawAxes
entry "DRAWLINEANDENTRY",ti.DrawLineEndEntry
entry "DRAWTILOGO",ti.DrawTILogo
db 0

tbl_E:
entry "EMPTYHOOK",ti.EmptyHook
entry "ERASEFLASH",ti.EraseFlash
entry "ERASEFLASHSECTOR",ti.EraseFlashSector
db 0

tbl_F:
entry "FINDFIELD",ti.FindField
entry "FINDFIRSTCERTFIELD",ti.FindFirstCertField
db 0

tbl_G:
entry "GETCERTCALCID",ti.GetCertCalcID
entry "GETCERTCALCSTRING",ti.GetCertCalcString
entry "GETCSC",ti.GetCSC
entry "GETFIELDSIZE",ti.GetFieldSize
entry "GETSERIAL",ti.GetSerial
entry "GETSTRINGINPUT",ti.GetStringInput
entry "GRAPHBGTODRAWBG",ti.GraphBGColorToDrawBGColor
db 0

tbl_H:
entry "HLMINUS5",ti.HLMinus5
entry "HLMINUS9",ti.HLMinus9
db 0

tbl_I:
entry "IAND",ti.iand
entry "IDIVS",ti.idivs
entry "IDIVU",ti.idivu
entry "IDVRMU",ti.idvrmu
entry "ILDIX",ti.ildix
db 0

tbl_L:
entry "LOADDEIND",ti.LoadDEInd
entry "LOADDEINDS",ti.LoadDEInd_s
entry "LOADHLINDS",ti.LoadHLInd_s
db 0

tbl_M:
entry "MARKOSINVALID",ti.MarkOSInvalid
entry "MEMCLEAR",ti.MemClear
entry "MEMSET",ti.MemSet
db 0

tbl_N:
entry "NAMETOOP1",ti.NameToOP1
entry "NEGBC",ti.NegBC
entry "NEGDE",ti.NegDE
db 0

tbl_O:
entry "OPSET0",ti.OPSet0
db 0

tbl_P
entry "PUTC",ti.PutC
entry "PUTMAP",ti.PutMap
entry "PUTPS",ti.PutPS
entry "PUTS",ti.PutS
db 0

tbl_R:
entry "RCLVARTOEDIT",ti.RclVarToEdit
entry "RCLVARTOEDITPTR",ti.RclVarToEditPtr
entry "RESTORELCDBRIGHTNESS",ti.RestoreLCDBrightness
entry "RUNINDICOFF",ti.RunIndicOff
entry "RUNINDICON",ti.RunIndicOn
db 0

tbl_S:
entry "SETATOBCU",ti.SetAtoBCU
entry "SETATOHLU",ti.SetAtoHLU
entry "SETATODEU",ti.SetAtoDEU
entry "SETATOHLU",ti.SetAtoHLU
entry "SETBCUTO0",ti.SetBCUto0
entry "SETBCUTOA",ti.SetBCUtoA
entry "SETBCUTOB",ti.SetBCUtoB
entry "SETDEUTOA",ti.SetDEUtoA
entry "SETDEUTOB",ti.SetDEUtoB
entry "SETHLUTO0",ti.SetHLUto0
entry "SETHLUTOA",ti.SetHLUtoA
entry "SETHLUTOB",ti.SetHLUtoB
entry "SETTEXTBGCOLOR",ti.SetTextBGcolor
entry "SETTEXTFGBGCOLORS",ti.SetTextFGBGColors
entry "SETWHITEDRAWCOLOR",ti.SetWhiteDrawBGColor
entry "SIGNEXTENDBC",ti.SignExtendBC
entry "SIGNEXTENDDE",ti.SignExtendDE
entry "SIGNEXTENDHL",ti.SignExtendHL
entry "SSTIX",ti.sstix
entry "SSTIY",ti.sstiy
entry "STRCMPRE",ti.StrCmpre
entry "STRLENGTH",ti.StrLength
entry "SWAPHLENDIAN",ti.SwapHLEndian
entry "SXOR",ti.sxor
db 0

tbl_V:
entry "VARNAMETOOP1HL",ti.VarNameToOP1HL
entry "VPUTMAP",ti.vPutMap
entry "VPUTS",ti.vPutS
db 0

tbl_W:
entry "WRITEFLASH",ti.WriteFlash
entry "WRITEFLASHA",ti.WriteFlashA
entry "WRITEFLASHBYTE",ti.WriteFlashByte
entry "WRITEFLASHUNSAFE",ti.WriteFlashUnsafe
db 0

