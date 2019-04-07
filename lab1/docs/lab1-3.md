#利用汇编实现 LED 闪烁
#
##问题探究
###在这一部分实验中 /dev/sdc1 中除了 kernel7.img （即 led.img）以外的文件哪些是重要的？他们的作用是什么？
bootcode.bin：引导GPU从SD卡上加载start.elf。  
start.elf：允许GPU启动CPU，加载之后的一些文件，如config.txt、cmdline.txt。  
config.txt：配置文件，各种系统配置参数。  
fixup.dat：配置文件，用来设置GPU和CPU之间的sdram分区。  
cmdline.txt：树莓派启动时将所有配置项参数传递给Linux内核。

###在这一部分实验中 /dev/sdc2 是否被用到？为什么？
不会被用到，sdc2是与Linux系统有关的文件，而汇编程序需要用到的文件都在sdc1中，实验中尝试将sdc2格式化之后led灯仍能正常闪烁。

###生成 led.img 的过程中用到了 as, ld, objcopy 这三个工具，他们分别有什么作用，我们平时编译程序会用到其中的哪些？
as将汇编文件编译成后缀为.o的目标文件，这些目标文件同其它目标模块或函数库易于定位和链接。  
ld根据用户的链接选项负责将多个*.o的目标文件和各种库链接成elf可执行文件。
objcopy 把一种目标文件中的内容复制到另一种类型的目标文件中。  
在编译程序时会用到ld与objcopy，ld用于链接文件生成elf可执行文件，objcopy用于将elf可执行文件压缩精简成bin二进制文件。

##汇编代码
这段汇编代码控制绿led灯亮0.5秒，暗1.5秒，从而实现2秒一闪烁的目的，r6寄存器中的值为500000，相当于0.5秒。r3中存放计时器低4字节的数，通过计算计时器0.5秒或1.5秒后低4字节的值r4与当前值r3进行比较来判断是否点亮或熄灭。

    .section .init
    .global _start
    _start:
    
    ldr r0, =0x3F200000
    //初始化
    mov r1, #1
    lsl r1, #27
    str r1, [r0, #8]
    mov r1, #1
    lsl r1, #29
    str r1, [r0, #28]
    //置r6的值为500000
    mov r6, #61
    lsl r6, #13
    mov r1, #9
    lsl r1, #5
    add r6,r1,r6
    
    ldr r2, =0x3F003000
    ldr r3, [r2, #4]
    add r4,r3,r6
    //灯亮循环，持续0.5秒
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
    //灯灭循环，持续1.5秒
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