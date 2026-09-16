.syntax unified
.cpu cortex-m3
.thumb

.section .isr_vector, "a", %progbits
.word _estack
.word Reset_Handler
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word SysTick_Handler
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word EXTI0_IRQHandler

.section .text.Reset_Handler, "ax", %progbits
.thumb_func
.global Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
1:
    cmp r1, r2
    bcc 2f
    b 3f
2:
    ldr r3, [r0]
    str r3, [r1]
    adds r0, #4
    adds r1, #4
    b 1b
3:
    ldr r1, =_sbss
    ldr r2, =_ebss
    movs r3, #0
4:
    cmp r1, r2
    bcc 5f
    b 6f
5:
    str r3, [r1]
    adds r1, #4
    b 4b
6:
    bl main
7:
    b 7b

.size Reset_Handler, .-Reset_Handler
.weak Default_Handler
.thumb_func
Default_Handler:
    b Default_Handler
