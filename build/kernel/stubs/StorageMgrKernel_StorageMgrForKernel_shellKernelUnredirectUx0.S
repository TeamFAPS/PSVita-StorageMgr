.arch armv7a

.section .vitalink.fstubs.StorageMgrForKernel,"ax",%progbits

	.align 4
	.global shellKernelUnredirectUx0
	.type shellKernelUnredirectUx0, %function
shellKernelUnredirectUx0:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0x2032C4D7
	.word 0xE144352C
	.align 4

