.code

	writeRFlags proc

		push rcx
		popfq
		ret

	writeRFlags endp

	readGDTR proc

		sgdt [rcx]
		ret

	readGDTR endp

	writeGDTR proc

		lgdt fword ptr [rcx]
		ret

	writeGDTR endp

	readCS proc

		xor rax, rax
		mov ax, cs
		ret

	readCS endp

	readSS proc

		xor rax, rax
		mov ax, ss
		ret

	readSS endp

	readTR proc

		xor rax, rax
		str ax
		ret

	readTR endp

end