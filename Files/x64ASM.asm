.code

	WriteRFlags proc

		push rcx
		popfq
		ret

	WriteRFlags endp

	ReadGDTR proc

		sgdt [rcx]
		ret

	ReadGDTR endp

	WriteGDTR proc

		lgdt fword ptr [rcx]
		ret

	WriteGDTR endp

	ReadCS proc

		xor rax, rax
		mov ax, cs
		ret

	ReadCS endp

	ReadSS proc

		xor rax, rax
		mov ax, ss
		ret

	ReadSS endp

	ReadTR proc

		xor rax, rax
		str ax
		ret

	ReadTR endp

end