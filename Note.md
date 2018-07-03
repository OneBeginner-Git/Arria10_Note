# Linux Note

[toc]

## VMware 
---

- 桥接模式可以直接使用主机的网络连接访问

## QSPI启动
---

### 更新和编译硬件设计

This section presents how to update the hardware design to boot from QSPI and then how to build it. This is part of the generic flow, and is required for any A10 @SoC System booting from QSPI.

The input to this flow is the GHRD delivered with SoC EDS, while the output is an ==mkimage wrapped rbf file and the handoff information for generating the Bootloader==.

1. Make a copy of the Hardware Design, from the archive delivered with @SoC EDS.

```
~/altera/15.0/embedded/embedded_command_shell.sh
cd ~
mkdir a10_soc_devkit_ghrd_qspi
cd a10_soc_devkit_ghrd_qspi/
tar xvzf ~/altera/15.0/embedded/examples/hardware/a10_soc_devkit_ghrd/tgz/*.tar.gz
```

2. Update the Hardware Design makefile to enable booting from QSPI:

```
sed -i 's/HPS_BOOT_DEVICE := SDMMC/HPS_BOOT_DEVICE := QSPI/g' Makefile
```

3. Update the Hardware Design makefile to create just a single RBF file instead of two.

```
sed -i 's/QUARTUS_CPF_ENABLE_SPLIT_RBF := 1/QUARTUS_CPF_ENABLE_SPLIT_RBF := 0/g' Makefile
```

4. Update the hardware design:

```
make scrub_clean
make quartus_generate_top quartus_generate_qsf_qpf qsys_generate_qsys
```

5. Compile the hardware design:

```
make sof
```

This will create the hardware configuration file **~/a10_soc_devkit_ghrd/output_files/ghrd_10as066n2.sof** and also the handoff information in the folder **hps_isw_handoff**.
Note that compiling the hardware desing can also be done using the instructions from http://rocketboards.org/foswiki/view/Documentation/A10GSRDV1501CompilingHardwareDesign.

6. Generate the rbf file:

```
make rbf
```

This will create the following file: **~/a10_soc_devkit_ghrd/output_files/ghrd_10as066n2.rbf.**
Creating the rbf file can also be achieved by running the command:
```
quartus_cpf -c -o bitstream_compression=on ~/a10_soc_devkit_ghrd/output_files/ghrd_10as066n2.sof ~/a10_soc_devkit_ghrd/output_files/ghrd_10as066n2.rbf
```
Note that we are == not using the "--hps"== option, since for booting from QSPI we are using **just a single combined rbf file**, containing both the I/O configuration and the FPGA fabric configuration.

`quartus_cpf -c --hps -o bitstream_compression=on output_files/ghrd_10as066n2.sof output_files/ghrd_10as066n2.rbf`
使用这个命令将会生成两个分离的rbf文件：*.core.rbf + *.periph.rbf

7. 在rbf文件的头部添加U-Boot mkimage头文件(U-Boot mkimge header):

```
mkimage -A arm -T firmware -C none -O u-boot -a 0 -e 0 -n "RBF" \
   -d output_files/ghrd_10as066n2.rbf \
   output_files/ghrd_10as066n2.rbf.mkimage
```

This will create the following file: **~/a10_soc_devkit_ghrd/output_files/ghrd_10as066n2.rbf.mkimage.**

### 生成U-Boot 和 U-Boot Device Tree

>U-Boot Device Tree 和 U-Boot 镜像都要在**embedded_command_shell**下才能生成

1. 启动embedded_command_shell.sh，打开bsp-editor生成U-Boot Device Tree

启动SoC EDS的Embedded Command Shell，然后打开bsp-editor

```
dark@dark-virtual-machine:~/altera/15.1/embedded$ ./embedded_command_shell.sh 
WARNING: DS-5 install not detected. SoC EDS may not function correctly without a DS-5 install.
------------------------------------------------
Altera Embedded Command Shell

Version 15.1 [Build 185]
------------------------------------------------
dark@dark-virtual-machine:~/altera/15.1/embedded$ bsp-editor 

```
2. 按照需要选择文件，配置，点击Generate，在**Embedded Command Shell**下使用```make```编译，在以下目录可以得到镜像文件：**~/a10_soc_devkit_ghrd/software/uboot_bsp/uboot_w_dtb-mkpimage.bin**

> 和SDMMC启动模式区别不大：The only difference is that the boot_device needs to be set to "Boot from QSPI", and that the peripheral_rbf_filename and core_rbf_filename do not have any meaning in this scenario.

3. 将U-Boot和硬件的rbf文件镜像烧写到QSPI　Flash：

    a. 连接JTAG口J22，和J10（用于串口）

    b. 打开串口助手，波特率等参数设置为 115200-8-N-1

    c. 重新上电开发板，在U-Boot准备启动boot时，按下任意键停止boot。**This is required because the HPS Flash programmer can have problems connecting to the board ==when Linux is running==.**

4. **在Windows下**，使用Embedded Command Shell烧写Flash

使用micro USB 连接PC和开发板，设备管理器显示Altera USB-BLASTER II说明已经正确连接。如果没有反应可以考虑重新安装驱动，或者换一根micro USB线。

运行Command Shell，在Embedded Command Shell下执行烧写的命令（注意各文件的路径）：

```
~/altera/15.0/embedded/embedded_command_shell.sh
cd ~/a10_soc_devkit_ghrd_qspi
quartus_hps --cable=1 --operation=PV --addr=0x0 uboot_w_dtb-mkpimage.bin
quartus_hps --cable=1 --operation=PV --addr=0x720000 ghrd_10as066n2.rbf.mkimage
```

**注意**：如果出现错误告警：==“The quartus_hps tool reports: “Quad SPI Flash silicon ID is 0x00000000” “Error: Not able to map flash ID from flash database”==，可以使用下列命令：

```quartus_hps --boot=18 -c 1 -o PV -a 0x0 uboot_w_dtb-mkpimage.bin```

这会触发开发板的冷重启(cold reset)。

### Linux kernel和文件系统

1. adfsadfasdfasdf
ffffff kkkkkk

## SDMMC启动模式
---

### Linux内核

编译步骤：

>每次打开控制台都要配置环境变量，或者也可以写环境变量文件然后使用```source```命令更新

1.打开控制台
2.跳转到linux-socfpga的目录，内核编译所需的文件来自github上的更新

```$ cd ~/linux-socfpga```

3.写入环境变量：交叉编译工具链的路径和ARCH
```
$ export CROSS_COMPILE=~/linux
$ export CROSS_COMPILE=~/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin/arm-linux-gnueabihf-
```
4.编译内核
```
$ make socfpga_defconfig
$ make zImage
```

### Linux文件系统

### 更新SD卡

1. 更新系统所需的文件(zImage rbf_file dtb)

>创建一个文件夹来挂载sd卡(在其他目录也可以？），这样sd卡的内容就可以在文件夹中显示并且可以进行操作

可以先在插入前后使用```$ cat /proc/partitions```确定sd卡的设备名"sdx"，其中sdx1表示系统区，sdx2表示文件区

例如：

``` cpp
dark@dark-virtual-machine:/dev$ cat /proc/partitions 
major minor  #blocks  name

   8        0   52428800 sda
   8        1   50331648 sda1
   8        2          1 sda2
   8        5    2094080 sda5
  11        0    1048575 sr0
   8       16    3887104 sdb
   8       17     512000 sdb1
   8       18    1536000 sdb2
   8       19      10240 sdb3
```

更新步骤：
``` cpp
$ sudo mkdir sdcard
$ sudo mount /dev/sdx1 sdcard/
$ sudo cp <file_name> sdcard/
$ sudo umount sdcard
```

2. 更新文件系统中的内容(root filsystem)

和上面类似，将sdx1替换为sdx2，挂载并更新

3. 更新U-Boot

`$ sudo dd if=uboot_w_dtb-mkpimage.bin of=/dev/sdx3 bs=64k seek=0`



## Git
---

### 如何克隆特定的分支到本地
 
首先，你需要使用

```
 $ git clone
```

这个命令克隆一个本地库。
之后它会自动克隆一个master分支（这个貌似是必须的）。
之后不会克隆任何一个分支下来的。
假定你需要一个dev（此处假定远程库中已经存在此分支，也就是你需要克隆的）分支用于开发的话，你需要在dev分支上开发，就必须创建远程origin的dev分支到本地，于是他用这个命令创建本地dev分支：

```
$ git checkout -b dev origin/dev
```

再同步下：

```
$ git pull
```

这样就实现了克隆dev分支。创建的本地分支``` dev ```让你可以push这个分支的修改(当然，要有创建者许可)

### Git错误汇总

1. Please move or remove them before you can switch branches

```
error: Your local changes to the following files would be overwritten by checkout:
.
.
.
.
Please move or remove them before you can switch branches.
```

出现这个错误时：可以通过以下的命令处理：

```git clean  -d  -fx ""```

注： 
1. x：表示删除忽略文件已经对git来说不识别的文件 
2. d: 删除未被添加到git的路径中的文件 
3. f: 强制执行

## Linux
---

### CLI

#### mount命令
1. 先在/dev/路径下创建新文件夹 sdcard ``` mkdir sdcard```
2. 将sdcard/和U盘或其他存储关联起来 ``` mount ```

#### file命令

```$ file <file-name>```

可以用来确定文件在什么平台上运行,ARM 或者x86

#### ls命令

使用ls命令时，各种文件的颜色的定义如下：

白色：表示普通文件
蓝色：表示目录
绿色：表示可执行文件
红色：表示压缩文件
浅蓝色：链接文件
红色闪烁：表示链接的文件有问题
黄色：表示设备文件
灰色：表示其他文件
这是linux系统约定的默认颜色

### TFTP服务

#### 在Windows平台搭建tftp服务器

使用小软件tftpd32.exe(Tftpd32 by Ph.Jouin) 即可自动搭建tftp服务器，需要注意Current Directory必须是存放使用tftp服务的文件目录，可以使用Show Dir来检查文件目录的内容。选择网卡，不用任何操作即可提供tftp服务（也许需要打开tftp服务？ 控制面板 -> 程序和功能 -> 启用或关闭Windows功能 -> [x] TFTP客户端

#### 在Linux平台搭建tftp服务器



#### 使用busybox的tftp服务

在开发板的命令行中输入
          ``` tftp -l a.txt -r a.txt -g 192.168.1.21 69 ```

          参数说明：-l   是local的缩写，后跟本地或下载到本地后重命名的文件名。    
                    -r  是remote的缩写，后跟远程即PC机tftp服务器根目录中的文件名，或上传到PC机后重命名后的文件名。
                    -g  是get的缩写，下载文件时用，后跟PC机的IP地址
                    -p  是put的缩写，上传文件时用，后跟PC机的IP地址
                    tftp 默认占用的是69端口。

注意：下载或上传文件时，文件名最好相同，否则会出现得到了文件，但文件大小却是 0k 的尴尬现象。如果出现了这种现象，请多敲几次命令，在 -l   -r  和 -g 前面多放几个空格，例如
                   ``` tftp  -l a.txt  -r a.txt  -g 192.168.1.21 69 ```

默认下载到开发板的当前目录。

上传文件：将开发板当前目录的某个文件上传到tftp服务器根目录。
命令：     ```tftp -l a.txt -r a.txt -p 192.168.1.21 69```

当然我们假设tftp服务器根目录不存在a.txt这个文件，当运行这个命令后，就会在tftp服务器根目录中发现这个文件。
