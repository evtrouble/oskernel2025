## 连接串口
sudo minicom -D /dev/ttyUSB0 -b 115200

## 查看文件内容
ext4ls scsi 0:1 /

## 扫描硬盘
scsi scan

## 扫描usb
usb reset

## 查看usb分区
fatls usb 0:1 / 

## 内存地址
0x9000000090000000

## 
load usb 0:1 0x9000000090000000 /kernel-la.bin

## 启动
go 0x9000000090000000

## 加载文件系统镜像到内存
load usb 0:1 0x9000000090000000 /filename

# 写入硬盘
scsi write 0x9000000090000000 0 262144



