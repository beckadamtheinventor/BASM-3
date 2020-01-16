	.def _cpyup
	.assume adl=1
_cpyup:
	ld hl,9
	add hl,sp
	ld bc,(hl)
	dec hl
	dec hl
	dec hl
	ld de,(hl)
	dec hl
	dec hl
	dec hl
	ld hl,(hl)
	add hl,bc
	ex de,hl
	add hl,bc
	lddr
	ret

