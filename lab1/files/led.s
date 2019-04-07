.section .init
.global _start
_start:

ldr r0, =0x3F200000

mov r1, #1
lsl r1, #27
str r1, [r0, #8]
mov r1, #1
lsl r1, #29
str r1, [r0, #28]

mov r6, #61
lsl r6, #13
mov r1, #9
lsl r1, #5
add r6,r1,r6

ldr r2, =0x3F003000
ldr r3, [r2, #4]
add r4,r3,r6

on:
ldr r3, [r2, #4]
cmp r3, r4
bne on
add r4,r3,r6
add r4,r4,r6
add r4,r4,r6
mov r1, #1
lsl r1, #29
str r1, [r0, #40]
b off

off:
ldr r3, [r2, #4]
cmp r3, r4
bne off
add r4,r3,r6
mov r1, #0
str r1, [r0, #40]
mov r1, #1
lsl r1, #27
str r1, [r0, #8]
mov r1, #1
lsl r1, #29
str r1, [r0, #28]
b on
