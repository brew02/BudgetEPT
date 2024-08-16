.code 

	BudgetEPTTest proc

		xor rax, rax
		mov ax, r8w
		ltr ax

		movzx rdx, dx
		push rdx

		lea rax, [rsp + 8]
		push rax

		pushfq

		movzx rcx, cx
		push rcx

		push r9
		iretq

	BudgetEPTTest endp

end