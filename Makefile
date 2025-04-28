# configure 
CONF_CPU_NUM = 1
export CONF_ARCH ?= loongarch
export CONF_PLATFORM ?= qemu_2k1000
# CONF_LINUX_BUILD = 1

# export CONF_ARCH ?= riscv
# export CONF_PLATFORM ?= qemu
# export CONF_PLATFORM ?= k210

ifeq ($(CONF_ARCH), loongarch)
QEMU = qemu-system-loongarch64
else ifeq ($(CONF_ARCH), riscv)
QEMU = qemu-system-riscv64
endif

# export CONF_PLATFORM ?= k210

# make variable define 

HOST_OS = $(shell uname)
HAL_LIB_NAME = hal_${CONF_ARCH}_${CONF_PLATFORM}.a

# 带有export的变量会在递归调用子目录的Makefile时传递下去
ifeq ($(CONF_ARCH), loongarch)
# gcc version 13.2.0 (GCC) 
export TOOLPREFIX = loongarch64-linux-gnu-
export ASFLAGS = -ggdb -march=loongarch64 -mabi=lp64d -O0
export CFLAGS = -march=loongarch64 -mabi=lp64d -DLOONGARCH
else ifeq ($(CONF_ARCH), riscv)
# gcc version 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04) 
export TOOLPREFIX = riscv64-linux-gnu-
export ASFLAGS = -ggdb -march=rv64gc -mabi=lp64d -O0
export CFLAGS = -march=rv64gc -mabi=lp64d -mcmodel=medany
export LDFLAGS = -static
endif

ifeq ($(CONF_PLATFORM), qemu)
export CFLAGS += -DQEMU
endif

LD_SCRIPT = hal/$(CONF_ARCH)/$(CONF_PLATFORM)/ld.script
export CFLAGS += -Wno-deprecated-declarations

export DEFAULT_CXX_INCLUDE_FLAG = \
	-include $(WORKPATH)/kernel/include/xn6_config.h \
	-include $(WORKPATH)/kernel/include/klib/virtual_function.hh \
	-include $(WORKPATH)/kernel/include/klib/global_operator.hh \
	-include $(WORKPATH)/kernel/include/types.hh \
	-include $(WORKPATH)/hsai/include/hsai_defs.h

export CC  = ${TOOLPREFIX}gcc
export CXX = ${TOOLPREFIX}g++
export CPP = ${TOOLPREFIX}cpp
export AS  = ${TOOLPREFIX}g++
export LD  = ${TOOLPREFIX}g++
export OBJCOPY = ${TOOLPREFIX}objcopy
export OBJDUMP = ${TOOLPREFIX}objdump
export AR  = ${TOOLPREFIX}ar

export ASFLAGS += -I include
export ASFLAGS += -MD
export CFLAGS += -ggdb -Wall -O0 -fno-omit-frame-pointer
export CFLAGS += -I include
export CFLAGS += -MD -static 
export CFLAGS += -DNUMCPU=$(CONF_CPU_NUM)
export CFLAGS += -DARCH=$(CONF_ARCH)
export CFLAGS += -DPLATFORM=$(CONF_PLATFORM)
export CFLAGS += -DOPEN_COLOR_PRINT=1
# open debug output
export CFLAGS += -DOS_DEBUG
ifeq ($(HOST_OS),Linux)
export CFLAGS += -DLINUX_BUILD=1
endif
export CFLAGS += -ffreestanding -fno-common -nostdlib -fno-stack-protector 
export CFLAGS += -fno-pie -no-pie 
# export CFLAGS += -static-libstdc++ -lstdc++
export CXXFLAGS = $(CFLAGS)
export CXXFLAGS += -std=c++23
export CXXFLAGS += $(DEFAULT_CXX_INCLUDE_FLAG)
export LDFLAGS += -z max-page-size=4096

export WORKPATH = $(shell pwd)
export BUILDPATH = $(WORKPATH)/build


STATIC_MODULE = \
	$(BUILDPATH)/kernel.a \
	$(BUILDPATH)/user/user.a \
	$(BUILDPATH)/thirdparty/EASTL/libeastl.a \
	$(BUILDPATH)/hsai.a \
	$(BUILDPATH)/$(HAL_LIB_NAME)

COMPILE_START_TIME := $(shell cat /proc/uptime | awk -F '.' '{print $$1}')


# .PHONY 是一个伪规则，其后面依赖的规则目标会成为一个伪目标，使得规则执行时不会实际生成这个目标文件
.PHONY: all clean test initdir probe_host compile_all load_kernel cp_to_bin EASTL EASTL_test config_platform print_compile_time fs run

# rules define 

all: probe_host compile_all load_kernel cp_to_bin
	@current_time=`cat /proc/uptime | awk -F '.' '{print $$1}'`; \
	echo "######## 生成结束时间戳: $${current_time} ########"; \
	time_interval=`expr $${current_time} - $(COMPILE_START_TIME)`; \
	runtime=`date -u -d @$${time_interval} "+%Mm %Ss"`; \
	echo "######## 生成用时 : $${runtime} ########"
	@echo "__________________________"
	@echo "-------- 生成成功 --------"

probe_host:
	@echo "********************************"
	@echo "当前主机操作系统：$(HOST_OS)"
	@echo "编译目标平台：$(CONF_ARCH)-$(CONF_PLATFORM)"
	@echo "当前时间戳: $(COMPILE_START_TIME)"
	@echo "********************************"

compile_all:
	$(MAKE) all -C thirdparty/EASTL
	$(MAKE) all -C user
	$(MAKE) all -C hsai
	$(MAKE) all -C hal/$(CONF_ARCH)
	$(MAKE) all -C kernel

load_kernel: $(BUILDPATH)/kernel.elf

$(BUILDPATH)/kernel.elf: $(STATIC_MODULE) $(LD_SCRIPT)
	$(LD) $(LDFLAGS) -T $(LD_SCRIPT) -o $@ -Wl,--whole-archive $(STATIC_MODULE) -Wl,--no-whole-archive \
	-Wl,--gc-sections

cp_to_bin: kernel.bin

kernel.bin: $(BUILDPATH)/kernel.elf
	$(OBJCOPY) -O binary ./build/kernel.elf ./kernel.bin

test:
	@echo $(COMPILE_START_TIME); sleep 3; \
	current_time=`cat /proc/uptime | awk -F '.' '{print $$1}'`; echo $${current_time}; \
	echo `expr $${current_time} - $(COMPILE_START_TIME)`
# $(MAKE) test -C hsai
#	$(MAKE) test -C kernel

print_inc:
	$(CPP) $(DEFAULT_CXX_INCLUDE_FLAG) -v /dev/null -o /dev/null

print_compile_time:
	@current_time=`cat /proc/uptime | awk -F '.' '{print $$1}'`; \
	time_interval=`expr $${current_time} - $(COMPILE_START_TIME)`; \
	runtime=`date -u -d @$${time_interval} "+%Mm %Ss"`; \
	echo "######## 生成用时 : $${runtime} ########"

clean:
	$(MAKE) clean -C kernel
	$(MAKE) clean -C user
	$(MAKE) clean -C thirdparty/EASTL
	$(MAKE) clean -C hsai
	$(MAKE) clean -C hal/$(CONF_ARCH)

.PHONY+= clean_module
clean_module:
	$(MAKE) clean -C $(mod)

EASTL_test:
	$(MAKE) test -C thirdparty/EASTL

ifndef CPUS
CPUS := 1
endif

ifeq ($(platform), k210)
RUSTSBI = hal/$(CONF_ARCH)/SBI/sbi-k210
else
RUSTSBI = hal/$(CONF_ARCH)/SBI/sbi-qemu
endif

# config_platform:
# 	@cd hal/$(CONF_ARCH)/$(CONF_PLATFORM); \
# 		cp config.mk $(WORKPATH)/hsai/Makedefs.mk
# 	@echo "******** 配置成功 ********"
# 	@echo "- 架构 : ${CONF_ARCH}"
# 	@echo "- 平台 : ${CONF_PLATFORM}"
# 	@echo "**************************"

ifeq ($(CONF_ARCH), loongarch)
# QEMUOPTS = -M ls2k -serial stdio -kernel build/kernel.elf -m 1G -k ./share/qemu/keymaps/en-us -serial vc \
# -net nic -net user,net=10.0.2.0/24,tftp=/srv/tftp -vnc :0 -S -s -hda sdcard.img
# 加载内核镜像，分配内存，不要图形化
QEMUOPTS = -kernel build/kernel.elf -m 1G -nographic
# use multi-core 
QEMUOPTS += -smp $(CPUS)
# 挂载第一个磁盘
QEMUOPTS += -drive file=sdcard.img,if=none,format=raw,id=x0 -s -S
# QEMUOPTS += -device virtio-blk-pci,drive=x0,bus=virtio-mmio-bus.0 -s -S
# 用virtio-blk设备接上第一个磁盘
QEMUOPTS += -device virtio-blk-pci,drive=x0
# 运行后不要重启，加载第一个网卡
QEMUOPTS += -no-reboot -device virtio-net-pci
# 配置用户模式网络（netdev=net0）
#QEMUOPTS += -netdev=net0 -netdev user,id=net0,hostfwd=tcp::5555-:5555,hostfwd=udp::5555-:5555
else 
QEMUOPTS = -machine virt -kernel build/kernel.elf -m 128M -nographic
# use multi-core 
QEMUOPTS += -smp $(CPUS)

QEMUOPTS += -bios $(RUSTSBI)

# import virtual disk image
QEMUOPTS += -drive file=sdcard.img,if=none,format=raw,id=x0 -s -S
# QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -s -S
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
endif

run: build
ifeq ($(CONF_PLATFORM), qemu_2k1000)
	@$(QEMU) $(QEMUOPTS)
else ifeq ($(CONF_PLATFORM), k210)
	@$(OBJCOPY) $T/kernel --strip-all -O binary $(image)
	@$(OBJCOPY) $(RUSTSBI) --strip-all -O binary $(k210)
	@dd if=$(image) of=$(k210) bs=128k seek=1
	@$(OBJDUMP) -D -b binary -m riscv $(k210) > $T/k210.asm
	@sudo chmod 777 $(k210-serialport)
	@python3 ./tools/kflash.py -p $(k210-serialport) -b 1500000 -t $(k210)
else
	@$(QEMU) $(QEMUOPTS)
endif

fs:
	@if [ ! -f "sdcard.img" ]; then \
		echo "Making fs image..."; \
		dd if=/dev/zero of=sdcard.img bs=512k count=512; \
		echo "Creating MBR partition table..."; \
		echo -e "o\nn\np\n1\n\n\nt\n83\nw" | fdisk sdcard.img; \
		echo "Setting up loop device..."; \
		sudo losetup -fP sdcard.img; \
		LOOP_DEVICE=$$(losetup -j sdcard.img | awk -F: '{print $$1}'); \
		echo "Formatting partition as ext4..."; \
		sudo mkfs.ext4 $${LOOP_DEVICE}p1; \
		echo "Cleaning up loop device..."; \
		sudo losetup -d $${LOOP_DEVICE}; \
		echo "sdcard.img created with MBR and ext4 partition."; \
	fi

