extern OldCS : word
extern OldSS : word
extern OldTR : word
extern OldGDTR : fword

.code

	GPHandler proc

		add rsp, 8h
		add qword ptr [rsp], 3h
		shl rax, 16
		mov ax, 0cafeh						; For demo purposes
		iretq

	GPHandler endp

	PFHandler proc

		add rsp, 8h
		add qword ptr [rsp], 7h

		push rax
		push rcx

		lea rax, [oldGDTR]
		lgdt fword ptr [rax]

		mov rcx, qword ptr [rax + 2h]
		xor rax, rax
		mov ax, OldTR
		shr ax, 3
		btr qword ptr [rcx + rax * 8h], 41	; Remove TSS busy flag (prevents #GP)

		xor rax, rax
		mov ax, OldCS
		mov qword ptr [rsp + 18h], rax		; Restore CS value

		mov ax, OldSS
		mov qword ptr [rsp + 30h], rax		; Restore SS value

		mov ax, OldTR
		ltr ax								; Restore TR value

		pop rcx
		pop rax

		shl rax, 16
		mov ax, 0dadeh						; For demo purposes

		iretq

	PFHandler endp

end