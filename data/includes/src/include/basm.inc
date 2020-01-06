
macro entry name, value
	_:
		db name
	db (entry_len+_-$) dup 0
	dl value
end macro


