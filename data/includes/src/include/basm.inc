
macro entry name, type, value
	_:
		db name
	db entry_len + $-_
	db type
	dl value
end macro


v_emit_byte      :=  1
v_emit_word      :=  2
v_emit_long      :=  3

f_add_r_arg      :=  1 shl 2
f_add_rr_arg     :=  1 shl 3
f_has_byte_arg   :=  1 shl 2 + 1 shl 4
f_has_long_arg   :=  1 shl 3 + 1 shl 4
f_add_byte_arg   :=  1 shl 5
f_ir_arg         :=  1 shl 6
f_irr_arg        :=  1 shl 7

