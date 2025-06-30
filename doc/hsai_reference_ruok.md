<font face="Maple Mono SC NF">

###### OS大赛 - 内核设计- RuOK队

---

[`<= 回到目录`](../README.md)

# HSAI 参考文档

本文档详细梳理 HSAI（Hardware Service Abstract Interface）模块的全部核心内容，涵盖所有接口、全局对象、关键类、宏定义、典型实现要点及其在内核与 HAL 之间的分工。适用于开发者查阅、平台适配、架构移植和调试。

---

## 目录

- [HSAI 参考文档](#hsai-参考文档)
  - [目录](#目录)
  - [hsai\_defs.h](#hsai_defsh)
  - [hsai\_global.hh](#hsai_globalhh)
    - [全局对象与初始化](#全局对象与初始化)
    - [全局对象](#全局对象)
  - [hsai\_log.hh](#hsai_loghh)
    - [日志与断言](#日志与断言)
    - [日志宏](#日志宏)
    - [颜色宏](#颜色宏)
  - [process\_interface.hh](#process_interfacehh)
    - [进程与上下文管理](#进程与上下文管理)
    - [陷阱帧与上下文操作](#陷阱帧与上下文操作)
  - [syscall\_interface.hh](#syscall_interfacehh)
  - [timer\_interface.hh](#timer_interfacehh)
  - [virtual\_cpu.hh](#virtual_cpuhh)
    - [类 VirtualCpu](#类-virtualcpu)
  - [device\_manager.hh](#device_managerhh)
    - [类 DeviceManager](#类-devicemanager)
    - [结构体 DeviceTableEntry](#结构体-devicetableentry)
  - [virtual\_device.hh](#virtual_devicehh)
    - [类 VirtualDevice](#类-virtualdevice)
    - [枚举 DeviceType](#枚举-devicetype)
  - [block\_device.hh](#block_devicehh)
    - [结构体 BufferDescriptor](#结构体-bufferdescriptor)
    - [类 BlockDevice : public VirtualDevice](#类-blockdevice--public-virtualdevice)
  - [char\_device.hh](#char_devicehh)
    - [类 CharDevice : public VirtualDevice](#类-chardevice--public-virtualdevice)
  - [stream\_device.hh](#stream_devicehh)
    - [类 StreamDevice : public CharDevice](#类-streamdevice--public-chardevice)
  - [console.hh](#consolehh)
    - [类 ConsoleStdin/Stdout/Stderr : public StreamDevice](#类-consolestdinstdoutstderr--public-streamdevice)
  - [memory\_interface.hh](#memory_interfacehh)
  - [mem/page.hh](#mempagehh)
  - [mem/virtual\_page\_table.hh](#memvirtual_page_tablehh)
  - [mem/virtual\_memory.hh](#memvirtual_memoryhh)
  - [smp/spin\_lock.hh](#smpspin_lockhh)
  - [uart/uart\_ns16550.hh](#uartuart_ns16550hh)
  - [uart/virtual\_uart.hh](#uartvirtual_uarthh)
  - [intr/virtual\_interrrupt\_manager.hh](#intrvirtual_interrrupt_managerhh)
  - [ata/ahci\_driver.hh, ahci\_port\_driver.hh](#ataahci_driverhh-ahci_port_driverhh)

---

## hsai_defs.h

- `__hsai_kernel` / `__hsai_hal`  
  仅作标记，分别标注“应由内核实现”或“应由HAL实现”的接口。
- `NUMCPU`  
  最大CPU核心数，默认2（可通过Makefile覆盖）。
- `DEV_TBL_LEN`  
  设备表最大长度，默认64。
- `DEV_FIRST_NOT_RSV`  
  第一个非保留设备号，默认3。
- `DEV_STDIN_NUM` / `DEV_STDOUT_NUM` / `DEV_STDERR_NUM`  
  标准输入/输出/错误流设备号，分别为0/1/2。
- `DEFAULT_DEBUG_CONSOLE_NAME`  
  默认调试控制台设备名。

---

## hsai_global.hh

### 全局对象与初始化

- `void hardware_abstract_init()`  
  HAL初始化入口，完成硬件抽象层的全局初始化（如CPU、内存、外设等）。
- `void hardware_secondary_init()`  
  HAL二阶段初始化，适用于多核或外设后期初始化。
- `void hsai_internal_init()`  
  HSAI内部对象初始化（如设备管理器、标准流等）。
- `void* get_ra()`  
  获取当前返回地址（架构相关）。

### 全局对象

- `VirtualCpu* k_cpus[NUMCPU]`  
  全局CPU对象数组。
- `VirtualCpu* get_cpu()`  
  获取当前CPU对象。
- `VirtualMemory* k_mem`  
  全局内存对象指针。
- `VirtualInterruptManager* k_im`  
  全局中断管理器指针。
- `DeviceManager k_devm`  
  全局设备管理器对象。
- `ConsoleStdin k_stdin` / `ConsoleStdout k_stdout` / `ConsoleStderr k_stderr`  
  全局标准输入/输出/错误流对象。

---

## hsai_log.hh

### 日志与断言

- `enum HsaiLogLevel { log_trace, log_info, log_warn, log_error, log_panic }`
- `void (*p_hsai_logout)(HsaiLogLevel, const char*, uint, const char*, ...)`  
  日志输出函数指针。
- `void (*p_hsai_assert)(const char*, uint, const char*, const char*, ...)`  
  断言输出函数指针。
- `void (*p_hsai_printf)(const char*, ...)`  
  格式化输出函数指针。

### 日志宏

- `hsai_trace/info/warn/error/panic(fmt, ...)`  
  不同级别日志输出，自动带文件名和行号。
- `hsai_assert(expr, detail, ...)`  
  条件断言失败时输出详细信息。
- `hsai_printf(fmt, ...)`  
  格式化输出。

### 颜色宏

- `RED_COLOR_PINRT`、`GREEN_COLOR_PRINT`、`YELLOW_COLOR_PRINT`、`BLUE_COLOR_PRINT`、`MAGANTA_COLOR_PRINT`、`CYAN_COLOR_PINRT`、`CLEAR_COLOR_PRINT`  
  终端彩色输出，支持高亮和清除。

---

## process_interface.hh

### 进程与上下文管理

- `constexpr uint proc_pool_size`  
  进程池大小。
- `void* get_cur_proc()`  
  获取当前进程对象。
- `void* get_trap_frame_from_proc(void*)`  
  获取进程陷阱帧。
- `ulong get_trap_frame_vir_addr()`  
  获取陷阱帧虚拟地址。
- `uint get_pid(void*)`  
  获取进程ID。
- `void exit_proc(void*, int)`  
  终止进程。
- `void proc_kill(void*)`  
  杀死进程。
- `char* get_proc_name(void*)`  
  获取进程名。
- `SpinLock& get_proc_lock(void*)`  
  获取进程锁。
- `bool proc_is_killed(void*)`  
  进程是否被杀死。
- `bool proc_is_running(void*)`  
  进程是否运行中。
- `void sched_proc(void*)`  
  调度进程。
- `ulong get_kstack_from_proc(void*)`  
  获取内核栈地址。
- `ulong get_kstack_size(void*)`  
  获取内核栈大小。
- `ulong get_pgd_addr(void*)`  
  获取页目录地址。
- `VirtualPageTable* get_pt_from_proc(void*)`  
  获取页表对象。
- `void sleep_at(void*, SpinLock&)`  
  进程休眠。
- `void wakeup_at(void*)`  
  唤醒进程。
- `void user_proc_init(void*)`  
  用户进程初始化。
- `void proc_init(void*)`  
  进程初始化。
- `void proc_free(void*)`  
  释放进程。

### 陷阱帧与上下文操作

- `void set_trap_frame_return_value(void*, ulong)`  
  设置trap返回值。
- `void set_trap_frame_entry(void*, void*)`  
  设置trap入口。
- `void set_trap_frame_user_sp(void*, ulong)`  
  设置用户栈指针。
- `void set_trap_frame_user_tp(void*, ulong)`  
  设置用户线程指针。
- `void set_trap_frame_arg(void*, uint, ulong)`  
  设置trap参数。
- `void copy_trap_frame(void*, void*)`  
  复制trap帧。
- `ulong get_arg_from_trap_frame(void*, uint)`  
  获取trap参数。
- `void user_trap_return()`  
  用户态trap返回。
- `const uint context_size`  
  上下文结构体大小。
- `void set_context_entry(void*, void*)`  
  设置上下文入口。
- `void set_context_sp(void*, ulong)`  
  设置上下文栈指针。
- `void* get_context_address(uint)`  
  获取上下文地址。

---

## syscall_interface.hh

- `long kernel_syscall(long n, ulong a0, ulong a1, ulong a2, ulong a3, ulong a4, ulong a5)`  
  内核系统调用分发函数。
- `int get_syscall_max_num()`  
  获取支持的最大系统调用号。

---

## timer_interface.hh

- `int handle_tick_intr()`  
  处理时钟中断（内核实现）。
- `ulong get_ticks()`  
  获取当前tick计数（内核实现）。
- `ulong get_main_frequence()`  
  获取主频（HAL实现）。
- `ulong cycles_per_tick()`  
  每tick周期数（HAL实现）。
- `ulong get_hw_time_stamp()`  
  获取硬件时间戳（HAL实现）。
- `ulong time_stamp_to_usec(ulong)`  
  时间戳转微秒（HAL实现）。
- `ulong usec_to_time_stamp(ulong)`  
  微秒转时间戳（HAL实现）。

---

## virtual_cpu.hh

### 类 VirtualCpu

- `_num_off`  
  中断关闭嵌套计数。
- `_int_ena`  
  进入临界区前的中断使能状态。
- `_cur_proc`  
  当前进程指针（pm::Pcb*）。
- `_context`  
  上下文指针。
- `VirtualCpu()`  
  构造函数。
- `static int register_cpu(VirtualCpu*, int)`  
  注册CPU对象。
- `virtual uint get_cpu_id()`  
  获取CPU编号。
- `virtual int is_interruptible()`  
  是否可中断。
- `virtual void _interrupt_on()` / `_interrupt_off()`  
  开/关中断。
- `int get_num_off()` / `get_int_ena()`  
  获取嵌套计数/中断状态。
- `pm::Pcb* get_cur_proc()` / `void set_cur_proc(pm::Pcb*)`  
  获取/设置当前进程。
- `void* get_context()` / `void set_context(void*)`  
  获取/设置上下文。
- `void set_int_ena(int)`  
  设置中断状态。
- `void push_interrupt_off()` / `pop_intterupt_off()`  
  中断嵌套管理。

---

## device_manager.hh

### 类 DeviceManager

- `DeviceTableEntry _device_table[DEV_TBL_LEN]`  
  设备表，存储所有注册设备。
- `static const char* _device_default_name`  
  默认设备名。
- `DeviceManager()`  
  构造函数。
- `int register_device(VirtualDevice*, const char*)`  
  注册通用设备。
- `int register_block_device(BlockDevice*, const char*)`  
  注册块设备。
- `int register_char_device(CharDevice*, const char*)`  
  注册字符设备。
- `VirtualDevice* get_device(const char*)`  
  按名查找设备。
- `BlockDevice* get_block_device(const char*)`  
  查找块设备。
- `CharDevice* get_char_device(const char*)`  
  查找字符设备。
- `int search_device(const char*)` / `search_block_device(const char*)` / `search_char_device(const char*)`  
  查找设备索引。
- `int remove_block_device(const char*)` / `remove_char_device(const char*)`  
  移除设备。
- `void traversal_dev_table(char** dev_table)`  
  遍历设备表。
- `VirtualDevice* get_device(uint dev_num)`  
  按编号查找设备。

### 结构体 DeviceTableEntry

- `VirtualDevice* device_ptr`  
  设备指针。
- `const char* device_name`  
  设备名。

---

## virtual_device.hh

### 类 VirtualDevice

- `virtual DeviceType type()`  
  设备类型（块/字符/其他）。
- `virtual int handle_intr() = 0`  
  处理中断。
- `virtual bool read_ready() = 0` / `write_ready() = 0`  
  读/写就绪。

### 枚举 DeviceType

- `dev_unknown`、`dev_block`、`dev_char`、`dev_other`

---

## block_device.hh

### 结构体 BufferDescriptor

- `u64 buf_addr`  
  缓冲区物理地址。
- `u32 buf_size`  
  缓冲区大小。

### 类 BlockDevice : public VirtualDevice

- `virtual long get_block_size() = 0`  
  块大小。
- `virtual int read_blocks_sync(...) = 0` / `read_blocks(...) = 0`  
  同步/异步读块。
- `virtual int write_blocks_sync(...) = 0` / `write_blocks(...) = 0`  
  同步/异步写块。
- `virtual int handle_intr() = 0`  
  处理中断。

---

## char_device.hh

### 类 CharDevice : public VirtualDevice

- `virtual long write(void*, long) = 0` / `read(void*, long) = 0`  
  读写接口。
- `virtual int handle_intr() = 0`  
  处理中断。

---

## stream_device.hh

### 类 StreamDevice : public CharDevice

- 适用于流式设备（如终端、串口等），继承自 CharDevice。

---

## console.hh

### 类 ConsoleStdin/Stdout/Stderr : public StreamDevice

- `long write(void*, long)` / `read(void*, long)`  
  标准流读写。
- `bool read_ready()` / `write_ready()`  
  流状态。
- 绑定底层Stream对象，支持流式输入输出。

---

## memory_interface.hh

- `void* alloc_pages(uint cnt)` / `int free_pages(void*)`  
  分配/释放页。
- `int copy_to_user(VirtualPageTable*, uint64 va, const void*, uint64 len)`  
  内核到用户空间拷贝。
- `int copy_from_user(VirtualPageTable*, void*, uint64 src_va, uint64 len)`  
  用户到内核空间拷贝。

---

## mem/page.hh

- `PG_SIZE`：页大小（4KB）。
- `PN_MASK`：页号掩码（0x1FF）。
- `PT_LEVEL`：页表级数（4）。
- `class Pte`：页表项抽象，支持有效性、权限、物理地址等操作。
- `page_size`、`page_size_shift`、`page_number_mask`、`page_number_mask_width`：页相关常量。
- `page_round_up/down()`、`is_page_align()`：页对齐工具。
- `pgd_mask/pud_mask/pmd_mask/pt_mask` 及其 shift/num 工具函数。

---

## mem/virtual_page_table.hh

- `class VirtualPageTable`
  - `_pt_base`：页表基址。
  - `virtual Pte walk(ulong va, bool alloc) = 0`：查找/分配页表项。
  - `virtual ulong walk_addr(ulong va) = 0` / `kwalk_addr(uint64 va) = 0`：查找虚拟/内核地址。

---

## mem/virtual_memory.hh

- `class VirtualMemory`
  - `virtual ulong mem_start() = 0` / `mem_size() = 0`：物理内存起始/大小。
  - `virtual ulong to_vir(ulong)` / `to_phy(ulong)` / `to_io(ulong)` / `to_dma(ulong)`：地址转换。
  - `virtual void config_pt(ulong)`：配置全局页表。
  - `static int register_memory(VirtualMemory*)`：注册全局内存对象。

---

## smp/spin_lock.hh

- `class SpinLock`
  - `_name`：锁名。
  - `_locked`：锁状态。
  - `SpinLock()`：构造。
  - `void init(const char*)`：初始化。
  - `void acquire()` / `release()`：加锁/解锁。
  - `bool is_held()`：锁是否被持有。

---

## uart/uart_ns16550.hh

- `class UartNs16550 : public VirtualUartController`
  - `_buf_size`：缓冲区大小（1024）。
  - `_lock`：自旋锁。
  - `_reg_base`：寄存器基址。
  - `_buf`、`_wr_idx`、`_rd_idx`：发送缓冲区及索引。
  - `_read_buf`、`_read_front`、`_read_tail`：接收缓冲区及索引。
  - 构造函数、初始化、同步/异步收发、处理中断等接口。
  - `enum RegOffset`：寄存器偏移定义。
  - `struct regIER/regFCR/regISR/regLCR/regMCR/regLSR/regMSR/regPSD`：寄存器位域结构体。
  - `_write_reg()` / `_read_reg()`：寄存器操作。

---

## uart/virtual_uart.hh

- `class VirtualUartController`  
  虚拟串口控制器抽象，定义串口收发等接口。

---

## intr/virtual_interrrupt_manager.hh

- `class VirtualInterruptManager`
  - `virtual int handle_dev_intr() = 0`：设备中断处理。
  - `static int register_interrupt_manager(VirtualInterruptManager*)`：注册全局中断管理器。

---

## ata/ahci_driver.hh, ahci_port_driver.hh

- `class AhciDriver`  
  AHCI主控制器驱动，负责端口初始化、设备识别、寄存器配置、MBR检测、分区管理等。
- `class AhciPortDriver`  
  AHCI端口驱动，实现块设备接口，支持同步/异步读写、DMA、命令队列、PRDT、FIS等。
- 详细寄存器位定义（如cap、ghc、is、ie、cmd、tfd、ssts、serr等），参见`ahci.hh`。
- 支持调试打印、命令表解析、端口状态跟踪、错误处理等。

---

> **补充说明：**
> - HSAI 设计目标是为内核与 HAL 提供统一、可扩展的硬件抽象接口，便于多平台适配和模块解耦。
> - 所有设备、内存、进程、系统调用等均以虚基类和全局对象方式暴露，便于内核通过统一接口访问硬件资源。
> - 具体实现需参考各平台 HAL 子目录和 kernel 目录下的适配代码。
> - 本文档已覆盖 HSAI 目录下所有核心接口、对象、结构体、宏定义及典型实现要点，适合开发者查阅和二次开发。
