#!/usr/bin/python3
import os,sys

try:
	os.makedirs("bin")
except:
	pass

try:
	os.makedirs("obj")
except:
	pass

try:
	fname = sys.argv[1]
except:
	fname = input("include file to convert?")

try:
	with open(fname+".inc","r") as f:
		lines = f.read().splitlines()
except:
	quit()

letters = {l:[] for l in "ABCDEFGHIJKLMNOPQRSTUVWXYZ["}

for line in lines:
	if not len(line):
		continue
	if line[0]==';' or not len(line):
		continue
	elif line[0]=='?':
		k = line.find(':=')
		i = k+2
		while line[i] in " \t":
			i+=1
			if i>=len(line): break
		if i<len(line):
			j = i
			while line[j] in "0123456789ABCDEF":
				j+=1
				if j>=len(line): break
			base=10
			if j<len(line):
				if line[j]=='h': base=16
				if line[j]=='b': base=2
				if line[j]=='o': base=8
			if j>i:
				word = line[1:k].upper().replace("_","[").replace(".","[").replace("\t","").replace(" ","")
				try:
					dt = [word,int(line[i:j],base)]
					letters[word[0]].append(dt)
				except:
					pass

with open("obj/"+fname+".bin","wb") as f:
	s=b"BASM3.0 INC"
	f.write(s)
	f.write(bytes([0]*(32-len(s))))
	tbloff = 32
	f.write(bytes([0]*54))
	tbl = []
	for letter in letters.keys():
		tbl.append(f.tell())
		for word in letters[letter]:
			f.write(bytes(word[0],'UTF-8'))
			arg=[word[1]&0xFF,(word[1]//0x100)&0xFF,(word[1]//0x10000)&0xFF]
			a=0
			for i in range(3):
				if not arg[i]:
					break
				else:
					a+=1
			f.write(bytes([0,a]+arg[:a]))
		f.write(bytes([0]))

	size=f.tell()
	if size>0xFFE8:
		print("File too large!",size)
		quit()

	f.seek(tbloff)
	for i in range(27):
		f.write(bytes([tbl[i]&0xFF,(tbl[i]//0x100)&0xFF]))

with open("obj/"+fname+".asm","w") as f:
	if len(fname)>8: name = fname[:8]
	else: name=fname
	f.write("include 'include/tiformat.inc'\nformat ti archived appvar '"+name+"'\nfile 'obj/"+fname+".bin'")

os.system("fasmg obj/"+fname+".asm bin/"+fname+".8xv")

