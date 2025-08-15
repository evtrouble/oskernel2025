# 系统调用的说明以及调用方式
系统调用方式遵循RISC-V ABI, 即调用号存放在a7寄存器中, 6个参数分别储存在a0-a5寄存器中, 返回值保存在a0中。

主要参考了Linux 5.10 syscalls，详细请参见：https://man7.org/linux/man-pages/man2/syscalls.2.html

---

## 文件系统相关

### #define SYS_getcwd 17
功能：获取当前工作目录。
输入：
- char *buf：缓存区，用于保存当前工作目录字符串。
- size：buf缓存区的大小。
返回值：成功返回当前工作目录字符串指针，失败返回NULL。
```c
char *buf; size_t size;
long ret = syscall(SYS_getcwd, buf, size);
```

### #define SYS_dup 23
功能：复制文件描述符。
输入：
- fd：被复制的文件描述符。
返回值：成功返回新的文件描述符，失败返回-1。
```c
int fd;
int ret = syscall(SYS_dup, fd);
```

### #define SYS_dup2 24
功能：复制文件描述符到指定的新描述符。
输入：
- oldfd：被复制的文件描述符。
- newfd：新的文件描述符。
返回值：成功返回新的文件描述符，失败返回-1。
```c
int oldfd, newfd;
int ret = syscall(SYS_dup2, oldfd, newfd);
```

### #define SYS_fcntl 25
功能：操作文件描述符。
输入：
- fd：文件描述符。
- cmd：操作命令。
- arg：可选参数。
返回值：根据cmd不同返回不同，失败返回-1。
```c
int fd, cmd, arg;
int ret = syscall(SYS_fcntl, fd, cmd, arg);
```

### #define SYS_ioctl 29
功能：设备控制。
输入：
- fd：文件描述符。
- request：请求码。
- argp：参数指针。
返回值：成功返回0，失败返回-1。
```c
int fd, request; void *argp;
int ret = syscall(SYS_ioctl, fd, request, argp);
```

### #define SYS_mkdir 34
功能：创建目录。
输入：
- path：目录路径。
- mode：权限。
返回值：成功返回0，失败返回-1。
```c
const char *path; mode_t mode;
int ret = syscall(SYS_mkdir, path, mode);
```

### #define SYS_unlinkat 35
功能：删除文件或目录。
输入：
- dirfd：目录文件描述符。
- path：路径。
- flags：标志。
返回值：成功返回0，失败返回-1。
```c
int dirfd; const char *path; int flags;
int ret = syscall(SYS_unlinkat, dirfd, path, flags);
```

### #define SYS_umount 39
功能：卸载文件系统。
输入：
- special：卸载目标。
返回值：成功返回0，失败返回-1。
```c
const char *special;
int ret = syscall(SYS_umount, special);
```

### #define SYS_mount 40
功能：挂载文件系统。
输入：
- special：设备。
- dir：挂载点。
- fstype：文件系统类型。
- flags：挂载参数。
- data：额外参数。
返回值：成功返回0，失败返回-1。
```c
const char *special, *dir, *fstype; unsigned long flags; const void *data;
int ret = syscall(SYS_mount, special, dir, fstype, flags, data);
```

### #define SYS_statfs 43
功能：获取文件系统状态。
输入：
- path：路径。
- buf：状态结构体指针。
返回值：成功返回0，失败返回-1。
```c
const char *path; struct statfs *buf;
int ret = syscall(SYS_statfs, path, buf);
```

### #define SYS_faccessat 48
功能：测试文件访问权限。
输入：
- dirfd：目录fd。
- pathname：路径。
- mode：访问模式。
- flags：标志。
返回值：成功返回0，失败返回-1。
```c
int dirfd; const char *pathname; int mode, flags;
int ret = syscall(SYS_faccessat, dirfd, pathname, mode, flags);
```

### #define SYS_chdir 49
功能：切换工作目录。
输入：
- path：目录路径。
返回值：成功返回0，失败返回-1。
```c
const char *path;
int ret = syscall(SYS_chdir, path);
```

### #define SYS_openat 56
功能：打开或创建文件。
输入：
- dirfd：目录fd。
- pathname：文件名。
- flags：打开标志。
- mode：权限。
返回值：成功返回文件描述符，失败返回-1。
```c
int dirfd; const char *pathname; int flags; mode_t mode;
int ret = syscall(SYS_openat, dirfd, pathname, flags, mode);
```

### #define SYS_close 57
功能：关闭文件描述符。
输入：
- fd：文件描述符。
返回值：成功返回0，失败返回-1。
```c
int fd;
int ret = syscall(SYS_close, fd);
```

### #define SYS_pipe 59
功能：创建管道。
输入：
- fd[2]：保存两个文件描述符。
返回值：成功返回0，失败返回-1。
```c
int fd[2];
int ret = syscall(SYS_pipe, fd);
```

### #define SYS_getdents 61
功能：读取目录项。
输入：
- fd：目录文件描述符。
- dirp：目录项缓冲区。
- count：缓冲区大小。
返回值：成功返回读取字节数，失败返回-1。
```c
int fd; struct dirent *dirp; size_t count;
int ret = syscall(SYS_getdents, fd, dirp, count);
```

### #define SYS_lseek 62
功能：移动文件读写指针。
输入：
- fd：文件描述符。
- offset：偏移量。
- whence：起始位置。
返回值：成功返回新位置，失败返回-1。
```c
int fd; off_t offset; int whence;
off_t ret = syscall(SYS_lseek, fd, offset, whence);
```

### #define SYS_read 63
功能：读取文件。
输入：
- fd：文件描述符。
- buf：缓冲区。
- count：读取字节数。
返回值：成功返回读取字节数，失败返回-1。
```c
int fd; void *buf; size_t count;
ssize_t ret = syscall(SYS_read, fd, buf, count);
```

### #define SYS_write 64
功能：写入文件。
输入：
- fd：文件描述符。
- buf：缓冲区。
- count：写入字节数。
返回值：成功返回写入字节数，失败返回-1。
```c
int fd; const void *buf; size_t count;
ssize_t ret = syscall(SYS_write, fd, buf, count);
```

### #define SYS_writev 66
功能：分散写入。
输入：
- fd：文件描述符。
- iov：iovec数组。
- iovcnt：数组长度。
返回值：成功返回写入字节数，失败返回-1。
```c
int fd; const struct iovec *iov; int iovcnt;
ssize_t ret = syscall(SYS_writev, fd, iov, iovcnt);
```

### #define SYS_pread64 67
功能：带偏移量读取文件。
输入：
- fd：文件描述符。
- buf：缓冲区。
- count：读取字节数。
- offset：偏移量。
返回值：成功返回读取字节数，失败返回-1。
```c
int fd; void *buf; size_t count; off_t offset;
ssize_t ret = syscall(SYS_pread64, fd, buf, count, offset);
```

### #define SYS_sendfile 71
功能：文件间拷贝。
输入：
- out_fd：输出文件描述符。
- in_fd：输入文件描述符。
- offset：偏移量指针。
- count：拷贝字节数。
返回值：成功返回拷贝字节数，失败返回-1。
```c
int out_fd, in_fd; off_t *offset; size_t count;
ssize_t ret = syscall(SYS_sendfile, out_fd, in_fd, offset, count);
```

### #define SYS_ppoll 73
功能：带信号掩码的poll。
输入：
- fds：pollfd数组。
- nfds：数组长度。
- tmo_p：超时。
- sigmask：信号掩码。
返回值：成功返回就绪fd数，失败返回-1。
```c
struct pollfd *fds; nfds_t nfds; const struct timespec *tmo_p; const sigset_t *sigmask;
int ret = syscall(SYS_ppoll, fds, nfds, tmo_p, sigmask);
```

### #define SYS_splice 76
功能：在两个文件描述符之间移动数据。
输入：
- fd_in：输入fd。
- off_in：输入偏移。
- fd_out：输出fd。
- off_out：输出偏移。
- len：长度。
- flags：标志。
返回值：成功返回移动字节数，失败返回-1。
```c
int fd_in, fd_out; off_t *off_in, *off_out; size_t len; unsigned int flags;
ssize_t ret = syscall(SYS_splice, fd_in, off_in, fd_out, off_out, len, flags);
```

### #define SYS_readlinkat 78
功能：读取符号链接内容。
输入：
- dirfd：目录fd。
- pathname：路径。
- buf：缓冲区。
- bufsiz：缓冲区大小。
返回值：成功返回读取字节数，失败返回-1。
```c
int dirfd; const char *pathname; char *buf; size_t bufsiz;
ssize_t ret = syscall(SYS_readlinkat, dirfd, pathname, buf, bufsiz);
```

### #define SYS_fstatat 79
功能：获取文件状态。
输入：
- dirfd：目录fd。
- pathname：路径。
- statbuf：状态结构体指针。
- flags：标志。
返回值：成功返回0，失败返回-1。
```c
int dirfd; const char *pathname; struct stat *statbuf; int flags;
int ret = syscall(SYS_fstatat, dirfd, pathname, statbuf, flags);
```

### #define SYS_fstat 80
功能：获取文件状态。
输入：
- fd：文件描述符。
- statbuf：状态结构体指针。
返回值：成功返回0，失败返回-1。
```c
int fd; struct stat *statbuf;
int ret = syscall(SYS_fstat, fd, statbuf);
```

### #define SYS_utimensat 88
功能：设置文件时间。
输入：
- dirfd：目录fd。
- pathname：路径。
- times：时间数组。
- flags：标志。
返回值：成功返回0，失败返回-1。
```c
int dirfd; const char *pathname; const struct timespec times[2]; int flags;
int ret = syscall(SYS_utimensat, dirfd, pathname, times, flags);
```

---

## 进程管理相关

### #define SYS_exit 93
功能：终止进程。
输入：
- status：退出状态。
无返回值。
```c
int status;
syscall(SYS_exit, status);
```

### #define SYS_exit_group 94
功能：终止线程组。
输入：
- status：退出状态。
无返回值。
```c
int status;
syscall(SYS_exit_group, status);
```

### #define SYS_set_tid_address 96
功能：设置线程ID地址。
输入：
- tidptr：线程ID指针。
返回值：线程ID。
```c
int *tidptr;
int ret = syscall(SYS_set_tid_address, tidptr);
```

### #define SYS_set_robust_list 99
功能：设置robust list。
输入：
- head：robust list头指针。
- len：长度。
返回值：成功返回0，失败返回-1。
```c
struct robust_list_head *head; size_t len;
int ret = syscall(SYS_set_robust_list, head, len);
```

### #define SYS_sleep 101
功能：进程睡眠。
输入：
- seconds：睡眠秒数。
返回值：成功返回0，失败返回-1。
```c
unsigned int seconds;
int ret = syscall(SYS_sleep, seconds);
```

### #define SYS_clock_gettime 113
功能：获取时钟时间。
输入：
- clk_id：时钟ID。
- tp：timespec结构体指针。
返回值：成功返回0，失败返回-1。
```c
clockid_t clk_id; struct timespec *tp;
int ret = syscall(SYS_clock_gettime, clk_id, tp);
```

### #define SYS_nanosleep 115
功能：纳秒级睡眠。
输入：
- req：请求时间。
- rem：剩余时间。
返回值：成功返回0，失败返回-1。
```c
const struct timespec *req; struct timespec *rem;
int ret = syscall(SYS_nanosleep, req, rem);
```

### #define SYS_syslog 116
功能：内核日志。
输入：
- type：日志类型。
- bufp：缓冲区。
- len：长度。
返回值：成功返回写入字节数，失败返回-1。
```c
int type; char *bufp; int len;
int ret = syscall(SYS_syslog, type, bufp, len);
```

### #define SYS_sched_yield 124
功能：让出CPU。
无输入参数。
返回值：成功返回0，失败返回-1。
```c
int ret = syscall(SYS_sched_yield);
```

### #define SYS_kill 129
功能：发送信号。
输入：
- pid：进程ID。
- sig：信号编号。
返回值：成功返回0，失败返回-1。
```c
int pid, sig;
int ret = syscall(SYS_kill, pid, sig);
```

### #define SYS_sigaction 134
功能：信号处理。
输入：
- signum：信号编号。
- act：新处理方式。
- oldact：旧处理方式。
返回值：成功返回0，失败返回-1。
```c
int signum; const struct sigaction *act; struct sigaction *oldact;
int ret = syscall(SYS_sigaction, signum, act, oldact);
```

### #define SYS_sigprocmask 135
功能：信号屏蔽字。
输入：
- how：操作方式。
- set：新屏蔽字。
- oldset：旧屏蔽字。
返回值：成功返回0，失败返回-1。
```c
int how; const sigset_t *set; sigset_t *oldset;
int ret = syscall(SYS_sigprocmask, how, set, oldset);
```

### #define SYS_setgid 144
功能：设置组ID。
输入：
- gid：组ID。
返回值：成功返回0，失败返回-1。
```c
gid_t gid;
int ret = syscall(SYS_setgid, gid);
```

### #define SYS_setuid 146
功能：设置用户ID。
输入：
- uid：用户ID。
返回值：成功返回0，失败返回-1。
```c
uid_t uid;
int ret = syscall(SYS_setuid, uid);
```

### #define SYS_times 153
功能：获取进程时间。
输入：
- tms：tms结构体指针。
返回值：成功返回滴答数，失败返回-1。
```c
struct tms *tms;
clock_t ret = syscall(SYS_times, tms);
```

### #define SYS_setpgid 154
功能：设置进程组ID。
输入：
- pid：进程ID。
- pgid：进程组ID。
返回值：成功返回0，失败返回-1。
```c
int pid, pgid;
int ret = syscall(SYS_setpgid, pid, pgid);
```

### #define SYS_getpgid 155
功能：获取进程组ID。
输入：
- pid：进程ID。
返回值：成功返回进程组ID，失败返回-1。
```c
int pid;
int ret = syscall(SYS_getpgid, pid);
```

### #define SYS_uname 160
功能：获取系统信息。
输入：
- uts：utsname结构体指针。
返回值：成功返回0，失败返回-1。
```c
struct utsname *uts;
int ret = syscall(SYS_uname, uts);
```

### #define SYS_getrusage 165
功能：获取资源使用情况。
输入：
- who：资源类型。
- usage：rusage结构体指针。
返回值：成功返回0，失败返回-1。
```c
int who; struct rusage *usage;
int ret = syscall(SYS_getrusage, who, usage);
```

### #define SYS_gettimeofday 169
功能：获取时间。
输入：
- tv：timeval结构体指针。
- tz：时区指针。
返回值：成功返回0，失败返回-1。
```c
struct timeval *tv; struct timezone *tz;
int ret = syscall(SYS_gettimeofday, tv, tz);
```

### #define SYS_getpid 172
功能：获取进程ID。
无输入参数。
返回值：进程ID。
```c
pid_t ret = syscall(SYS_getpid);
```

### #define SYS_getppid 173
功能：获取父进程ID。
无输入参数。
返回值：父进程ID。
```c
pid_t ret = syscall(SYS_getppid);
```

### #define SYS_getuid 174
功能：获取用户ID。
无输入参数。
返回值：用户ID。
```c
uid_t ret = syscall(SYS_getuid);
```

### #define SYS_geteuid 175
功能：获取有效用户ID。
无输入参数。
返回值：有效用户ID。
```c
uid_t ret = syscall(SYS_geteuid);
```

### #define SYS_getgid 176
功能：获取组ID。
无输入参数。
返回值：组ID。
```c
gid_t ret = syscall(SYS_getgid);
```

### #define SYS_getegid 177
功能：获取有效组ID。
无输入参数。
返回值：有效组ID。
```c
gid_t ret = syscall(SYS_getegid);
```

### #define SYS_gettid 178
功能：获取线程ID。
无输入参数。
返回值：线程ID。
```c
pid_t ret = syscall(SYS_gettid);
```

### #define SYS_sysinfo 179
功能：获取系统信息。
输入：
- info：sysinfo结构体指针。
返回值：成功返回0，失败返回-1。
```c
struct sysinfo *info;
int ret = syscall(SYS_sysinfo, info);
```

---

## 内存管理相关

### #define SYS_brk 214
功能：设置数据段末尾。
输入：
- brk：新末尾地址。
返回值：成功返回0，失败返回-1。
```c
void *brk;
int ret = syscall(SYS_brk, brk);
```

### #define SYS_munmap 215
功能：解除内存映射。
输入：
- addr：起始地址。
- length：长度。
返回值：成功返回0，失败返回-1。
```c
void *addr; size_t length;
int ret = syscall(SYS_munmap, addr, length);
```

### #define SYS_mremap 216
功能：重新映射内存区域。
输入：
- old_address：原地址。
- old_size：原大小。
- new_size：新大小。
- flags：标志。
- new_address：新地址（可选）。
返回值：成功返回新地址，失败返回-1。
```c
void *old_address; size_t old_size, new_size; int flags; void *new_address;
void *ret = syscall(SYS_mremap, old_address, old_size, new_size, flags, new_address);
```

### #define SYS_clone 220
功能：创建子进程。
输入：
- flags：标志。
- stack：新进程栈。
- ptid：父线程ID。
- tls：线程本地存储。
- ctid：子线程ID。
返回值：成功返回子进程ID，失败返回-1。
```c
unsigned long flags; void *stack; int *ptid; void *tls; int *ctid;
pid_t ret = syscall(SYS_clone, flags, stack, ptid, tls, ctid);
```

### #define SYS_fork 220
功能：创建子进程（同clone）。
输入、返回值同上。
```c
unsigned long flags; void *stack; int *ptid; void *tls; int *ctid;
pid_t ret = syscall(SYS_fork, flags, stack, ptid, tls, ctid);
```

### #define SYS_execve 221
功能：执行新程序。
输入：
- filename：程序路径。
- argv：参数数组。
- envp：环境变量数组。
返回值：成功不返回，失败返回-1。
```c
const char *filename; char *const argv[]; char *const envp[];
int ret = syscall(SYS_execve, filename, argv, envp);
```

### #define SYS_mmap 222
功能：内存映射。
输入：
- addr：起始地址。
- length：长度。
- prot：保护标志。
- flags：映射标志。
- fd：文件描述符。
- offset：偏移量。
返回值：成功返回映射地址，失败返回-1。
```c
void *addr; size_t length; int prot, flags, fd; off_t offset;
void *ret = syscall(SYS_mmap, addr, length, prot, flags, fd, offset);
```

### #define SYS_mprotect 226
功能：更改内存保护。
输入：
- addr：起始地址。
- len：长度。
- prot：保护标志。
返回值：成功返回0，失败返回-1。
```c
void *addr; size_t len; int prot;
int ret = syscall(SYS_mprotect, addr, len, prot);
```

### #define SYS_madvise 233
功能：内存使用建议。
输入：
- addr：起始地址。
- length：长度。
- advice：建议类型。
返回值：成功返回0，失败返回-1。
```c
void *addr; size_t length; int advice;
int ret = syscall(SYS_madvise, addr, length, advice);
```

---

## 其他

### #define SYS_wait 260
功能：等待子进程。
输入：
- pid：进程ID。
- status：状态指针。
- options：选项。
返回值：成功返回进程ID，失败返回-1。
```c
pid_t pid; int *status; int options;
pid_t ret = syscall(SYS_wait, pid, status, options);
```

### #define SYS_prlimit64 261
功能：获取/设置资源限制。
输入：
- pid：进程ID。
- resource：资源类型。
- new_limit：新限制。
- old_limit：旧限制。
返回值：成功返回0，失败返回-1。
```c
pid_t pid; int resource; const struct rlimit64 *new_limit; struct rlimit64 *old_limit;
int ret = syscall(SYS_prlimit64, pid, resource, new_limit, old_limit);
```

### #define SYS_renameat2 276
功能：重命名文件。
输入：
- olddirfd：原目录fd。
- oldpath：原路径。
- newdirfd：新目录fd。
- newpath：新路径。
- flags：标志。
返回值：成功返回0，失败返回-1。
```c
int olddirfd; const char *oldpath; int newdirfd; const char *newpath; unsigned int flags;
int ret = syscall(SYS_renameat2, olddirfd, oldpath, newdirfd, newpath, flags);
```

### #define SYS_getrandom 278
功能：获取随机数。
输入：
- buf：缓冲区。
- buflen：长度。
- flags：标志。
返回值：成功返回读取字节数，失败返回-1。
```c
void *buf; size_t buflen; unsigned int flags;
ssize_t ret = syscall(SYS_getrandom, buf, buflen, flags);
```

### #define SYS_statx 291
功能：获取扩展文件状态。
输入：
- dirfd：目录fd。
- pathname：路径。
- flags：标志。
- mask：掩码。
- statxbuf：状态结构体指针。
返回值：成功返回0，失败返回-1。
```c
int dirfd; const char *pathname; int flags, mask; struct statx *statxbuf;
int ret = syscall(SYS_statx, dirfd, pathname, flags, mask, statxbuf);
```

### #define SYS_poweroff 2024
功能：关机。
无输入参数。
无返回值。
```c
syscall(SYS_poweroff);
```

### #define SYS_tgkill 131
功能：向指定线程发送信号。
输入：
- tgid：线程组ID。
- tid：线程ID。
- sig：信号编号。
返回值：成功返回0，失败返回-1。
```c
int tgid, tid, sig;
int ret = syscall(SYS_tgkill, tgid, tid, sig);
```

### #define SYS_readv 65
功能：分散读取。
输入：
- fd：文件描述符。
- iov：iovec数组。
- iovcnt：数组长度。
返回值：成功返回读取字节数，失败返回-1。
```c
int fd; const struct iovec *iov; int iovcnt;
ssize_t ret = syscall(SYS_readv, fd, iov, iovcnt);
```

### #define SYS_rt_sigtimedwait 137
功能：带超时的信号等待。
输入：
- set：信号集。
- info：siginfo结构体指针。
- timeout：超时时间。
- sigsetsize：信号集大小。
返回值：成功返回信号编号，失败返回-1。
```c
const sigset_t *set; siginfo_t *info; const struct timespec *timeout; size_t sigsetsize;
int ret = syscall(SYS_rt_sigtimedwait, set, info, timeout, sigsetsize);
```

### #define SYS_futex 98
功能：快速用户空间互斥体。
输入：
- uaddr：用户空间地址。
- futex_op：操作码。
- val：值。
- timeout：超时时间。
- uaddr2：第二地址。
- val3：第三值。
返回值：根据操作不同返回不同，失败返回-1。
```c
int *uaddr, futex_op, val; const struct timespec *timeout; int *uaddr2, val3;
int ret = syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
```

### #define SYS_socket 198
功能：创建套接字。
输入：
- domain：协议族。
- type：类型。
- protocol：协议。
返回值：成功返回套接字fd，失败返回-1。
```c
int domain, type, protocol;
int ret = syscall(SYS_socket, domain, type, protocol);
```

### #define SYS_bind 200
功能：绑定套接字。
输入：
- sockfd：套接字fd。
- addr：地址。
- addrlen：地址长度。
返回值：成功返回0，失败返回-1。
```c
int sockfd; const struct sockaddr *addr; socklen_t addrlen;
int ret = syscall(SYS_bind, sockfd, addr, addrlen);
```

### #define SYS_listen 201
功能：监听套接字。
输入：
- sockfd：套接字fd。
- backlog：最大连接数。
返回值：成功返回0，失败返回-1。
```c
int sockfd, backlog;
int ret = syscall(SYS_listen, sockfd, backlog);
```

### #define SYS_accept 202
功能：接受连接。
输入：
- sockfd：套接字fd。
- addr：地址。
- addrlen：地址长度指针。
返回值：成功返回新套接字fd，失败返回-1。
```c
int sockfd; struct sockaddr *addr; socklen_t *addrlen;
int ret = syscall(SYS_accept, sockfd, addr, addrlen);
```

### #define SYS_connect 203
功能：连接套接字。
输入：
- sockfd：套接字fd。
- addr：地址。
- addrlen：地址长度。
返回值：成功返回0，失败返回-1。
```c
int sockfd; const struct sockaddr *addr; socklen_t addrlen;
int ret = syscall(SYS_connect, sockfd, addr, addrlen);
```

### #define SYS_getsockname 204
功能：获取本地地址。
输入：
- sockfd：套接字fd。
- addr：地址结构体指针。
- addrlen：地址长度指针。
返回值：成功返回0，失败返回-1。
```c
int sockfd; struct sockaddr *addr; socklen_t *addrlen;
int ret = syscall(SYS_getsockname, sockfd, addr, addrlen);
```

### #define SYS_sendto 206
功能：发送数据。
输入：
- sockfd：套接字fd。
- buf：数据缓冲区。
- len：长度。
- flags：标志。
- dest_addr：目标地址。
- addrlen：地址长度。
返回值：成功返回发送字节数，失败返回-1。
```c
int sockfd; const void *buf; size_t len; int flags; const struct sockaddr *dest_addr; socklen_t addrlen;
ssize_t ret = syscall(SYS_sendto, sockfd, buf, len, flags, dest_addr, addrlen);
```

### #define SYS_recvfrom 207
功能：接收数据。
输入：
- sockfd：套接字fd。
- buf：数据缓冲区。
- len：长度。
- flags：标志。
- src_addr：源地址。
- addrlen：地址长度指针。
返回值：成功返回接收字节数，失败返回-1。
```c
int sockfd; void *buf; size_t len; int flags; struct sockaddr *src_addr; socklen_t *addrlen;
ssize_t ret = syscall(SYS_recvfrom, sockfd, buf, len, flags, src_addr, addrlen);
```

### #define SYS_setsockopt 208
功能：设置套接字选项。
输入：
- sockfd：套接字fd。
- level：选项级别。
- optname：选项名。
- optval：选项值。
- optlen：选项长度。
返回值：成功返回0，失败返回-1。
```c
int sockfd, level, optname; const void *optval; socklen_t optlen;
int ret = syscall(SYS_setsockopt, sockfd, level, optname, optval, optlen);
```

### #define SYS_clone3 435
功能：创建进程（新接口）。
输入：
- cl_args：clone_args结构体指针。
- size：结构体大小。
返回值：成功返回子进程ID，失败返回-1。
```c
struct clone_args *cl_args; size_t size;
pid_t ret = syscall(SYS_clone3, cl_args, size);
```

---

## 调用模板

```c
static inline _u64 internal_syscall(long n, _u64 _a0, _u64 _a1, _u64 _a2, _u64 _a3, _u64 _a4, _u64 _a5) {
    register _u64 a0 asm("a0") = _a0;
    register _u64 a1 asm("a1") = _a1;
    register _u64 a2 asm("a2") = _a2;
    register _u64 a3 asm("a3") = _a3;
    register _u64 a4 asm("a4") = _a4;
    register _u64 a5 asm("a5") = _a5;
    register long syscall_id asm("a7") = n;
    asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(syscall_id));
    return a0;
}
```