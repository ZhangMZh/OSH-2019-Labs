#树莓派启动过程和原理
#
##启动过程 
1. 开发板加电。  
2. 加载第一启动程序，这个启动程序负责挂载在SD卡中的FAT32的文件系统。  
3. GPU从SD卡的第一个FAT32分区的根目录下寻找一个叫bootcode.bin的二进制文件，加载并执行bootcode.bin。  
4. GPU将start.elf加载到内存中，开始执行start.elf。  
5. start.elf读取config.txt来设置图像输出格式,初始化CPU的clock和串口等设备。 
6. config.txt解析完成,start.elf读取再次加载cmdline.txt和kernel.img。
7. GPU将kernel.img加载到内存中，然后唤醒CPU，这时CPU开始执行kernel.img。

##与主流的计算机的区别
1. 主流计算机启动要加载BISO。它是一组固化到计算机主板上一块ROM芯片中的程序，保存着计算机最重要的基本输入输出程序、系统设置信息、开机后自检程序和系统自启动程序。树莓派相比主流计算机的启动更加精简。
2. 计算机通过启动CPU来运行BIOS中的程序，而树莓派的程序是在GPU运行的。
##启动中用到的文件系统
FAT32：从SD卡启动Linux，必须是FAT32文件系统。