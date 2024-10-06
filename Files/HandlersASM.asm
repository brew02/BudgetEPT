extern OldCS : word
extern OldSS : word
extern OldTR : word
extern OldGDTR : fword

.code

	DBHandler proc

		btr qword ptr [rsp + 10h], 18		; Reset access check
		btr qword ptr [rsp + 10h], 8		; Reset trap flag
											
											; Just restoring stuff for the demo
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
		mov ax, 0feedh						; For demo purposes

		iretq

	DBHandler endp

	GPHandler proc

		add rsp, 8h
		add qword ptr [rsp], 3h				; Skip instruction (mov rcx, cr3)
		shl rax, 16
		mov ax, 0cafeh						; For demo purposes
		iretq

	GPHandler endp

	PFHandler proc

		add rsp, 8h
		bts qword ptr [rsp + 10h], 18		; Set access check
		bts qword ptr [rsp + 10h], 8		; Set trap flag
		btr qword ptr [rsp + 10h], 16		; Reset resume flag

		shl rax, 16
		mov ax, 0dadeh						; For demo purposes

		iretq

	PFHandler endp

end