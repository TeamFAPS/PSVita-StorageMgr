.arch armv7a

.section .vitalink.fstubs.StorageMgrForKernel,"ax",%progbits

	.align 4
	.global shellKernelIsUx0Redirected
	.type shellKernelIsUx0Redirected, %function
shellKernelIsUx0Redirected:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0x2032C4D7
	.word 0x890A5F0B
	.align 4

