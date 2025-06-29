# OSKernel2025 开发问题记录

本文档记录了开发过程中遇到的主要问题及其解决方案，按照时间顺序排列，旨在为后续开发提供参考。

---

## BUG07: 管道读取字符乱码

**时间：** 2025年6月3日  
**提交链接：** [cd9f7b39](https://gitlab.eduxiji.net/T202510486995232/oskernel2025/-/commit/cd9f7b399a66233d8bf941e1c47a498b343cd4d1)

### 问题描述

`busybox` 读取管道字符时出现乱码，并提示语法错误。

### 问题分析

通过调试发现，问题出在第三方库 `queue` 的实现上，导致数据读取异常。

### 解决方案

重新实现一个简单的循环数组队列，替代第三方库。

---

## BUG03: 标准输出被关闭

**时间：** 2025年6月17日  
**提交链接：** [22e88e20](https://gitlab.eduxiji.net/T202510486995232/oskernel2025/-/commit/22e88e20e43c14a40d466dcfdd44059c7731ce50)

### 问题描述

在 `busybox` 执行完后，标准输出流被关闭，导致无法输出内容。

### 问题分析

调试发现，`execve` 过程中未正确维护标准输出流，导致其被关闭。进一步检查发现，标准输出流被错误释放，甚至输出被重定向到标准输入流。

### 解决方案

在文件结构体中增加一个标志变量 `always_on`，确保标准输入、输出和错误流不会被释放。

```cpp
// kernel/include/fs/file/file.hh
bool always_on = false;

// kernel/pm/process_manager.cc
for (int i = 0; i < max_open_files; i++) {
    fs::file* temp = proc->_ofile[i];
    if (temp != nullptr && temp->_fl_cloexec && !temp->always_on) {
        close(i);
    }
}
```

---

## BUG06: `uname` 返回格式不正确

**时间：** 2025年6月17日  
**提交链接：** [0b2c6ec1](https://gitlab.eduxiji.net/T202510486995232/oskernel2025/-/commit/0b2c6ec1eb5beef7343829dd6202c63d2c8081d5)

### 问题描述

`uname` 返回的系统信息格式不符合 Linux 标准，导致 `busybox` 报错 `fatal, kernel too old`。

### 问题分析

`uname` 的实现未遵循 Linux 标准格式，导致兼容性问题。

### 解决方案

按照 Linux 标准实现 `uname` 的返回值。

```cpp
static const char _SYSINFO_sysname[]    = "Linux";
static const char _SYSINFO_nodename[]   = "(none-node)";
static const char _SYSINFO_release[]    = "5.15.0";
static const char _SYSINFO_version[]    = "#1 SMP Mon Jan 1 12:00:00 UTC 2024";
static const char _SYSINFO_machine[]    = "LoongArch-2k1000";
static const char _SYSINFO_domainname[] = "(none-domain)";
```

---

## BUG04: 递归释放导致栈溢出

**时间：** 2025年6月22日  
**提交链接：** [47a290e5](https://gitlab.eduxiji.net/T202510486995232/oskernel2025/-/commit/47a290e53a3d0fb2399eb243197275ef3f42a10e)

### 问题描述

在释放 `dentry` 时出现递归调用，导致栈溢出。

### 问题分析

初步认为是栈空间不足，于是尝试增加栈空间，但问题依然存在。进一步分析发现，`dentry` 的循环引用未被正确处理，导致递归释放时陷入死循环。

### 解决方案

暂时通过增加 `dentry` 的数量缓解问题，后续需优化 `dentry` 的引用计数逻辑。

```cpp
void dentryCache::releaseDentryCache(dentry* de) {
    if (de != nullptr) {
        if (de->isRoot() || de->isMntPoint()) return;
        dentry* parent = de->getParent();
        if (parent) {
            parent->children.erase(de->rName());
            if (parent->children.empty() && parent->refcnt <= 0)
                releaseDentryCache(parent);
        }
        _lock.acquire();
        freeList_.push_back(de);
        _lock.release();
    }
}
```

---

## BUG05: Busybox 文件重复打开

**时间：** 2025年6月22日  
**提交链接：** [47a290e5](https://gitlab.eduxiji.net/T202510486995232/oskernel2025/-/commit/47a290e53a3d0fb2399eb243197275ef3f42a10e)  
**补充提交：** [48003d8d](https://gitlab.eduxiji.net/T202510486995232/oskernel2025/-/commit/48003d8d78a7b61be018678ae79a03a709d812cb)

### 问题描述

`busybox` 测试时，某个文件被重复打开，尽管返回了有效的文件描述符，但仍然不断尝试打开。

### 问题分析

通过 `debugfs` 工具和 GDB 调试发现，`ext4` 文件系统未正确实现哈希目录项的读取，导致文件路径解析错误。

### 解决方案

补充实现 `ext4` 的哈希目录项读取逻辑。

---

## BUG01: Lua 脚本测试失败

**时间：** 2025年6月23日  
**提交链接：** [31e893d5](https://gitlab.eduxiji.net/T202510486995232/oskernel2025/-/commit/31e893d50b4b658bed3ec828b8c9b2e7afbf0968)

### 问题描述

在使用 `busybox` 测试 Lua 脚本时，执行失败，报错 `can't execve .sh`。

### 问题分析

通过调试发现，`execve` 将 `.sh` 文件直接当作可执行文件处理，导致执行失败。

### 解决方案

在 `execve` 中增加对文件后缀的判断逻辑。如果文件以 `.sh` 结尾，则在参数前插入 `busybox` 和 `sh`，通过 `busybox` 来执行该脚本。

```cpp
// 检查是否为 .sh 文件，如果是则使用 busybox sh 执行
if (path.size() >= 3 && path.substr(path.size() - 3) == ".sh") {
    // 在 argv 前面插入 "./busybox" 和 "sh"
    argv.insert(argv.begin(), "sh");
    argv.insert(argv.begin(), "./busybox");
    // 修改 path 为 busybox 路径
    path = "./busybox";
}
```

---

## BUG02: VMA 区域不足

**时间：** 2025年6月23日  
**提交链接：** [31e893d5](https://gitlab.eduxiji.net/T202510486995232/oskernel2025/-/commit/31e893d50b4b658bed3ec828b8c9b2e7afbf0968)

### 问题描述

在运行过程中，出现与内存映射相关的错误，提示 VMA（虚拟内存区域）不足。

### 问题分析

通过 RISC-V 的 GDB 调试发现，每个进程仅分配了 10 个 VMA，无法满足实际需求。

### 解决方案

将 VMA 的数量从 10 增加到 64。尽管仍可能存在不足，但可以根据内存需求动态调整。

```cpp
// 增加 VMA 数量
vma vm[max_vma_num]; // virtual memory area
```

---

以上为开发过程中遇到的主要问题及解决方案，后续开发需持续优化代码质量，避免类似问题再次发生。