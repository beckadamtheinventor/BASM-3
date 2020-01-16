	.def _wstrcpy
	.assume adl=1
_wstrcpy:
	ld iy,15
	add iy,sp
	ld bc,(iy) ;len1
	ld hl,(iy-3) ;word1
	ld de,(iy-12) ;buffer
	ldir
	ld bc,(iy-6) ;len2
	ld hl,(iy-9) ;word2
	ldir
	ret

