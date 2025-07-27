//
// Created by Li Shuang ( pseudonym ) on 2024-05-20
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use
// | Contact Author: lishuang.mk@whu.edu.cn
// --------------------------------------------------------------
//

#include "syscall/syscall_handler.hh"

// include syscall number
#include "syscall/syscall_defs.hh"
//

#include <EASTL/random.h>
#include <asm-generic/poll.h>
#include <linux/sysinfo.h>
#include <sys/ioctl.h>
#include <asm-generic/errno-base.h>
//#include <sys/resource.h>

#include <hsai_global.hh>
#include <mem/virtual_memory.hh>
#include <process_interface.hh>

#include "fs/dev/acpi_controller.hh"
#include "fs/file/device.hh"
#include "fs/file/file.hh"
#include "fs/file/normal.hh"
#include "fs/file/pipe.hh"
#include "fs/kstat.hh"
#include "fs/path.hh"
#include "hal/cpu.hh"
#include "klib/klib.hh"
#include "mm/physical_memory_manager.hh"
#include "mm/userspace_stream.hh"
#include "mm/userstack_stream.hh"
#include "mm/virtual_memory_manager.hh"
#include "pm/futex.hh"
#include "pm/ipc/signal.hh"
#include "pm/prlimit.hh"
#include "pm/process.hh"
#include "pm/process_manager.hh"
#include "pm/scheduler.hh"
#include "tm/time.hh"
#include "tm/timer_manager.hh"
namespace syscall
{
	SyscallHandler k_syscall_handler;

#define BIND_SYSCALL( sys_name )                             \
	_syscall_funcs[SYS_##sys_name] =                         \
		std::bind( &SyscallHandler::_sys_##sys_name, this ); \
	_syscall_name[SYS_##sys_name] = #sys_name

	void SyscallHandler::init()
	{
		for ( auto &func : _syscall_funcs )
		{
			func = []() -> uint64
			{
				return 0;
			};
		}
		for ( auto &n : _syscall_name ) n = nullptr;

		BIND_SYSCALL( write );
		BIND_SYSCALL( read );
		BIND_SYSCALL( exit );
		BIND_SYSCALL( clone );
		BIND_SYSCALL( fork );
		BIND_SYSCALL( clone3 );
		BIND_SYSCALL( getpid );
		BIND_SYSCALL( getppid );
		BIND_SYSCALL( brk );
		BIND_SYSCALL( execve );
		BIND_SYSCALL( wait );
		BIND_SYSCALL( poweroff );
		BIND_SYSCALL( dup );
		BIND_SYSCALL( dup2 );
		BIND_SYSCALL( getcwd );
		BIND_SYSCALL( gettimeofday );
		BIND_SYSCALL( sched_yield );
		BIND_SYSCALL( sleep );
		BIND_SYSCALL( times );
		BIND_SYSCALL( uname );
		BIND_SYSCALL( openat );
		BIND_SYSCALL( close );
		BIND_SYSCALL( fstat );
		BIND_SYSCALL( getdents );
		BIND_SYSCALL( mkdir );
		BIND_SYSCALL( chdir );
		BIND_SYSCALL( mount );
		BIND_SYSCALL( umount );
		BIND_SYSCALL( mmap );
		BIND_SYSCALL( munmap );
		BIND_SYSCALL( statx );
		BIND_SYSCALL( unlinkat );
		BIND_SYSCALL( pipe );
		BIND_SYSCALL( set_tid_address );
		BIND_SYSCALL( set_robust_list );
		BIND_SYSCALL( prlimit64 );
		BIND_SYSCALL( clock_gettime );
		BIND_SYSCALL( mprotect );
		BIND_SYSCALL( getuid );
		BIND_SYSCALL( readlinkat );
		BIND_SYSCALL( getrandom );
		BIND_SYSCALL( sigaction );
		BIND_SYSCALL( ioctl );
		BIND_SYSCALL( fcntl );
		BIND_SYSCALL( getpgid );
		BIND_SYSCALL( setpgid );
		BIND_SYSCALL( geteuid );
		BIND_SYSCALL( getegid );
		BIND_SYSCALL( ppoll );
		BIND_SYSCALL( getgid );
		BIND_SYSCALL( setgid );
		BIND_SYSCALL( setuid );
		BIND_SYSCALL( gettid );
		BIND_SYSCALL( fstatat );
		BIND_SYSCALL( sendfile );
		BIND_SYSCALL( exit_group );
		BIND_SYSCALL( statfs );
		BIND_SYSCALL( syslog );
		BIND_SYSCALL( faccessat );
		BIND_SYSCALL( madvise );
		BIND_SYSCALL( mremap );
		BIND_SYSCALL( sysinfo );
		BIND_SYSCALL( nanosleep );
		BIND_SYSCALL( getrusage );
		BIND_SYSCALL( utimensat );
		BIND_SYSCALL( lseek );
		BIND_SYSCALL( splice );
		BIND_SYSCALL( sigprocmask );
		BIND_SYSCALL( kill );
		BIND_SYSCALL( writev );
		BIND_SYSCALL( pread64 );
		BIND_SYSCALL( tgkill );
		BIND_SYSCALL( renameat2 );
		BIND_SYSCALL( readv );
		BIND_SYSCALL(rt_sigtimedwait);
		BIND_SYSCALL(futex);
		BIND_SYSCALL(socket);
		BIND_SYSCALL(bind);
		BIND_SYSCALL(listen);
		BIND_SYSCALL(accept);
		BIND_SYSCALL(connect);
		BIND_SYSCALL(getsockname);
		BIND_SYSCALL(sendto);
		BIND_SYSCALL(recvfrom);
		BIND_SYSCALL(setsockopt);
		BIND_SYSCALL(copy_file_range);
		BIND_SYSCALL(ftruncate);
	}

	uint64 SyscallHandler::invoke_syscaller( uint64 sys_num )
	{
		pm::Pcb		  *proc = pm::k_pm.get_cur_pcb();
#ifdef OS_DEBUG
		if ( sys_num != SYS_write )
		{

			if ( _syscall_name[sys_num] != nullptr )
				printf( YELLOW_COLOR_PRINT "进程:%d syscall %16s\n",
						proc->_pid,_syscall_name[sys_num] );
			else
				log_panic( "unknown syscall %d\n", sys_num );
			auto [usg, rst] = mm::k_pmm.mem_desc();
			printf( "mem-usage: %_-10ld mem-rest: %_-10ld\n" CLEAR_COLOR_PRINT,
					usg, rst );
		}
#endif
		return _syscall_funcs[sys_num]();
	}


	// ---------------- private helper functions ----------------

	int SyscallHandler::_fetch_addr( uint64 addr, uint64 &out_data )
	{
		pm::Pcb		  *p  = (pm::Pcb *) hsai::get_cur_proc();
		// if ( addr >= p->get_size() || addr + sizeof( uint64 ) > p->get_size()
		// ) 	return -1;
		mm::PageTable *pt = p->get_pagetable();
		if ( mm::k_vmm.copy_in( *pt, &out_data, addr, sizeof( out_data ) ) < 0 )
			return -1;
		return 0;
	}

	int SyscallHandler::_fetch_str( uint64 addr, void *buf, uint64 max )
	{
		pm::Pcb		  *p   = (pm::Pcb *) hsai::get_cur_proc();
		mm::PageTable *pt  = p->get_pagetable();
		int			   err = mm::k_vmm.copy_str_in( *pt, buf, addr, max );
		if ( err < 0 ) return err;
		return strlen( (const char *) buf );
	}

	int SyscallHandler::_fetch_str( uint64 addr, eastl::string &str,
									uint64 max )
	{
		pm::Pcb		  *p   = (pm::Pcb *) hsai::get_cur_proc();
		mm::PageTable *pt  = p->get_pagetable();
		int			   err = mm::k_vmm.copy_str_in( *pt, str, addr, max );
		if ( err < 0 ) return err;
		return str.size();
	}

	uint64 SyscallHandler::_arg_raw( int arg_n )
	{
		return hsai::get_arg_from_trap_frame(
			hsai::get_trap_frame_from_proc( hsai::get_cur_proc() ),
			(uint) arg_n );
	}

	int SyscallHandler::_arg_fd( int arg_n, int *out_fd, fs::file **out_f )
	{
		int		  fd;
		fs::file *f;

		if ( _arg_int( arg_n, fd ) < 0 ) return -1;
		pm::Pcb *p = (pm::Pcb *) hsai::get_cur_proc();
		f		   = p->get_open_file( fd );
		if ( f == nullptr ) return -1;
		if ( out_fd ) *out_fd = fd;
		if ( out_f ) *out_f = f;

		return 0;
	}

	// ---------------- syscall functions ----------------

	uint64 SyscallHandler::_sys_write()
	{
		fs::file			*f;
		int					 n;
		uint64				 buf_addr;
		[[maybe_unused]] int fd = 0;

		if ( _arg_fd( 0, &fd, &f ) < 0 || _arg_addr( 1, buf_addr ) < 0 ||
			 _arg_int( 2, n ) < 0 )
		{
			return -1;
		}

		// if ( fd > 2 )
		// 	printf( BLUE_COLOR_PRINT "invoke sys_write\n" CLEAR_COLOR_PRINT );

		pm::Pcb		  *proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt	= proc->get_pagetable();

		char *buf = new char[n + 10];
		{
			mm::UserspaceStream uspace( (void *) buf_addr, n + 1, pt );
			uspace.open();
			mm::UsRangeDesc urd = std::make_tuple( (u8 *) buf, (ulong) n + 1 );
			uspace >> urd;
			uspace.close();
		}
		// if ( mm::k_vmm.copy_in( *pt, (void *) buf, p, n ) < 0 ) return -1;

		// if ( buf[0] == '#' && buf[1] == '#' && buf[2] == '#' && buf[3] == '#'
		// )
		// {
		// 	printf( YELLOW_COLOR_PRINT "note : echo ####\n" CLEAR_COLOR_PRINT );
		// }

		long rc = f->write( (ulong) buf, n, f->get_file_offset(), true );
		delete[] buf;
		return rc;
	}

	uint64 SyscallHandler::_sys_writev()
	{
		fs::file			*f;
		fs::iovec           iov;
		int fd;
		int iovcnt;
		uint64 iov_ptr;
		if(_arg_fd( 0, &fd, &f ) < 0 || _arg_addr(1,iov_ptr) <0 || _arg_int(2,iovcnt) <0){
			return -1;
		}
		pm::Pcb		  *proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt	= proc->get_pagetable();
		uint64 writebytes = 0;
		uint64 expected=0;
		for(int i=0;i<iovcnt;i++){
			if(mm::k_vmm.copy_in(*pt, (void*)&iov, (uint64)(iov_ptr+ i * sizeof(fs::iovec)),sizeof(fs::iovec))<0)return -1;
			int tempLength=iov.iov_len;
			expected+=tempLength;
			char*buf=new char[tempLength+10];
			if(mm::k_vmm.copy_in(*pt,(void*)buf,(uint64)iov.iov_base,tempLength)<0){
				delete buf;
				return -1;
			}
			writebytes+= f->write( (ulong) buf, tempLength, f->get_file_offset(), true );
			delete buf;
		}
		return writebytes;
	}
	uint64 SyscallHandler::_sys_copy_file_range() {
		int fd_in, fd_out;
		uint64 off_in_addr, off_out_addr;
		uint64 off_in, off_out;
		int len;
		uint64 flags;
		fs::file *fi, *fo;
		pm::Pcb *p = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		
		// 参数验证
		if (_arg_fd(0, &fd_in, &fi) < 0 || _arg_fd(2, &fd_out, &fo) < 0 ||
			_arg_addr(1, off_in_addr) < 0 || _arg_addr(3, off_out_addr) < 0 ||
			_arg_int(4, len) < 0 || _arg_addr(5, flags) < 0) {
			return -1;
		}
		
		if (fi == nullptr || fo == nullptr) return -1;
		if (len == 0) return 0;
		
		log_info("偏移量分别是%d  %d\n", fi->get_file_offset(), fo->get_file_offset());
		log_info("长度是%d\n", len);
		log_info("copy_file_range: 文件描述符fd_in：%d ，偏移量地址_addr：%d 文件描述符fd_out：%d，偏移量地址off_out_addr %d len%d flags%d\n", 
			fd_in, off_in_addr, fd_out, off_out_addr, len, flags);
		
		// 处理输入偏移量
		bool use_in_offset = (off_in_addr != 0);
		if (use_in_offset) {
			if (mm::k_vmm.copy_in(*pt, &off_in, off_in_addr, sizeof(off_in)) < 0)
				return -1;
		} else {
			off_in = fi->get_file_offset();
		}
		
		// 处理输出偏移量
		bool use_out_offset = (off_out_addr != 0);
		if (use_out_offset) {
			if (mm::k_vmm.copy_in(*pt, &off_out, off_out_addr, sizeof(off_out)) < 0)
				return -1;
		} else {
			off_out = fo->get_file_offset();
		}
		
		// 执行文件复制
		char* buf = new char[len];
		int readlen = fi->read((uint64)buf, len, off_in, !use_in_offset);
		int writeLen = fo->write((uint64)buf, readlen, off_out, !use_out_offset);
		delete[] buf;  // 修复：应该用 delete[] 而不是 delete
		
		// 更新偏移量并写回用户空间
		if (use_in_offset) {
			off_in += readlen;
			if (mm::k_vmm.copyout(*pt, off_in_addr, &off_in, sizeof(off_in)) < 0)
				return -1;
		}
		
		if (use_out_offset) {
			off_out += writeLen;
			if (mm::k_vmm.copyout(*pt, off_out_addr, &off_out, sizeof(off_out)) < 0)
				return -1;
		}
		
		fs::normal_file *f1 = (fs::normal_file *)fi;
		fs::normal_file *f2 = (fs::normal_file *)fo;
		log_info("readlen%d， 入文件大小是%d, 出文件大小是%d\n", readlen, f1->_stat.size, f2->_stat.size);
		
		return readlen;
	}
	uint64 SyscallHandler::_sys_ftruncate()
	{
		fs::file			*f;
		int fd;
		uint64 len;
		if ( _arg_fd( 0, &fd, &f ) < 0 || _arg_addr( 1, len ) < 0 ) return -1;
		if ( f->ftruncate( len ) < 0 ) return -1;
		log_info("truncate file  to %d bytes",len);
		return 0;
	}

	uint64 SyscallHandler::_sys_read()
	{
		fs::file			*f;
		uint64				 buf;
		int					 n;
		[[maybe_unused]] int fd = -1;

		if ( _arg_fd( 0, &fd, &f ) < 0 ) return -1;
		if ( _arg_addr( 1, buf ) < 0 ) return -2;
		if ( _arg_int( 2, n ) < 0 ) return -3;

		if ( f == nullptr ) return -4;
		if ( n <= 0 ) return -5;

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();

		char *k_buf = new char[n + 1];
		int	  ret	= f->read( (uint64) k_buf, n, f->get_file_offset(), true );
		if ( ret < 0 ) return -6;
		if ( mm::k_vmm.copyout( *pt, buf, k_buf, ret ) < 0 ) return -7;

		delete[] k_buf;
		return ret;
	}

	uint64 SyscallHandler::_sys_readv()
	{
		fs::file			*f;
		fs::iovec           iov;
		int fd;
		int iovcnt;
		uint64 iov_ptr;
		if(_arg_fd( 0, &fd, &f ) < 0 || _arg_addr(1, iov_ptr) < 0 || _arg_int(2, iovcnt) < 0){
			return -1;
		}
		pm::Pcb		  *proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt	= proc->get_pagetable();
		uint64 readbytes = 0;
		uint64 expected = 0;
		for(int i = 0; i < iovcnt; i++){
			if(mm::k_vmm.copy_in(*pt, (void*)&iov, (uint64)(iov_ptr + i * sizeof(fs::iovec)), sizeof(fs::iovec)) < 0) return -1;
			int tempLength = iov.iov_len;
			expected += tempLength;
			char *buf = new char[tempLength + 1];
			// 从文件读取数据到内核缓冲区
			int ret = f->read((uint64)buf, tempLength, f->get_file_offset(), true);
			if(ret < 0) {
				delete[] buf;
				return -1;
			}
			// 将内核缓冲区数据复制到用户空间
			if(mm::k_vmm.copyout(*pt, (uint64)iov.iov_base, buf, ret) < 0) {
				delete[] buf;
				return -1;
			}
			readbytes += ret;
			delete[] buf;
			// 如果读取的字节数小于请求的字节数，说明已到达文件末尾，停止读取
			if(ret < tempLength) break;
		}
		return readbytes;
	}

	uint64 SyscallHandler::_sys_pread64()
	{
		// pread64系统调用实现 - 从指定偏移量读取文件
		// 参数: fd, buf, count, offset
		fs::file *f;
		uint64 buf;
		int count;
		uint64 offset;
		int fd = -1;
		
		// 获取参数
		if ( _arg_fd( 0, &fd, &f ) < 0 ) return -1;
		if ( _arg_addr( 1, buf ) < 0 ) return -2;
		if ( _arg_int( 2, count ) < 0 ) return -3;
		if ( _arg_addr( 3, offset ) < 0 ) return -4;
		
		if ( f == nullptr ) return -5;
		if ( count <= 0 ) return -6;
		
		pm::Pcb *p = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		
		// 分配内核缓冲区
		char *k_buf = new char[count + 1];
		
		// 从指定偏移量读取文件，不改变文件当前偏移量
		int ret = f->read( (uint64) k_buf, count, offset, false );
		if ( ret < 0 ) {
			delete[] k_buf;
			return -7;
		}
		
		// 将数据复制到用户空间
		if ( mm::k_vmm.copyout( *pt, buf, k_buf, ret ) < 0 ) {
			delete[] k_buf;
			return -8;
		}
		
		delete[] k_buf;
		return ret;
	}

	uint64 SyscallHandler::_sys_exit()
	{
		int n;
		if ( _arg_int( 0, n ) < 0 ) return -1;
		pm::k_pm.exit( n );
		return 0; // not reached
	}

	uint64 SyscallHandler::_sys_clone()
	{
		// clone系统调用用于创建线程，但内核不支持线程
		// 返回EPERM错误让pthread_create知道操作不被允许
		printf("clone系统调用: 内核不支持线程，返回EPERM\n");
		return -1; // -EPERM
	}

	// uint64 SyscallHandler::_sys_fork()
	// {
	// 	uint64 u_sp;
	// 	if ( _arg_addr( 1, u_sp ) < 0 ) return -1;

	// 	return pm::k_pm.fork( u_sp );
	// }

	uint64 SyscallHandler::_sys_fork()
	{
		// clone系统调用参数:
		// arg0: flags - 控制创建进程/线程的行为标志
		// arg1: stack - 新进程的栈指针 
		// arg2: parent_tid - 父进程TID的地址
		// arg3: child_tid - 子进程TID的地址
		// arg4: tls - 线程本地存储描述符
		//printf("hello from _sys_fork\n");
		int flags;
		uint64 stack, parent_tid, child_tid, tls;
		
		// 读取所有参数
		if (_arg_int(0, flags) < 0) {
			printf("clone系统调用: 读取flags参数失败\n");
			return -1;
		}
		
		if (_arg_addr(1, stack) < 0) {
			printf("clone系统调用: 读取stack参数失败\n");
			return -1;
		}
		
		if (_arg_addr(2, parent_tid) < 0) {
			printf("clone系统调用: 读取parent_tid参数失败\n");
			return -1;
		}
		
		if (_arg_addr(3, child_tid) < 0) {
			printf("clone系统调用: 读取child_tid参数失败\n");
			return -1;
		}
		
		if (_arg_addr(4, tls) < 0) {
			printf("clone系统调用: 读取tls参数失败\n");
			return -1;
		}
		
		// // 打印所有参数
		// printf("clone系统调用参数:\n");
		// printf("  flags = 0x%x (%d)\n", flags, flags);
		// printf("  stack = 0x%lx\n", stack);
		// printf("  parent_tid = 0x%lx\n", parent_tid);
		// printf("  child_tid = 0x%lx\n", child_tid);
		// printf("  tls = 0x%lx\n", tls);
		
		// // 解析flags的含义
		// printf("  flags解析:\n");
		// if (flags & 0x00000100) printf("    CLONE_VM: 共享虚拟内存\n");
		// if (flags & 0x00000200) printf("    CLONE_FS: 共享文件系统信息\n");
		// if (flags & 0x00000400) printf("    CLONE_FILES: 共享文件描述符表\n");
		// if (flags & 0x00000800) printf("    CLONE_SIGHAND: 共享信号处理器\n");
		// if (flags & 0x00002000) printf("    CLONE_PTRACE: 允许跟踪\n");
		// if (flags & 0x00004000) printf("    CLONE_VFORK: vfork语义\n");
		// if (flags & 0x00008000) printf("    CLONE_PARENT: 设置父进程\n");
		// if (flags & 0x00010000) printf("    CLONE_THREAD: 创建线程\n");
		// if (flags & 0x00020000) printf("    CLONE_NEWNS: 新的命名空间\n");
		// if (flags & 0x00100000) printf("    CLONE_PARENT_SETTID: 写子进程tid\n");
		// if (flags & 0x00200000) printf("    CLONE_SYSVSEM: 共享System V信号量\n");
		// if (flags & 0x00400000) printf("    CLONE_SETTLS: 设置TLS\n");
		// if (flags & 0x01000000) printf("    CLONE_PARENT_SETTID: 设置父进程TID\n");
		// if (flags & 0x02000000) printf("    CLONE_CHILD_CLEARTID: 清除子进程TID\n");
		// if (flags & 0x08000000) printf("    CLONE_CHILD_SETTID: 设置子进程TID\n");
		
		return pm::k_pm.clone(flags, stack, parent_tid, child_tid, tls);
	}

	uint64 SyscallHandler::_sys_clone3()
	{
		// clone3系统调用是clone的扩展版本，用于创建进程/线程
		// 参数: struct clone_args *cl_args, size_t size
		uint64 cl_args_addr, size;
		if ( _arg_addr( 0, cl_args_addr ) < 0 ) return -1;
		if ( _arg_addr( 1, size ) < 0 ) return -2;
		
		printf("clone3系统调用: cl_args=0x%lx, size=%lu\n", cl_args_addr, size);
		
		// 内核不支持线程，返回ENOSYS错误
		printf("clone3系统调用: 内核不支持此功能，返回ENOSYS\n");
		return -38; // -ENOSYS
	}

	uint64 SyscallHandler::_sys_getpid()
	{
		//printf("hello from _sys_getpid\n");
		return pm::k_pm.get_cur_pcb()->get_pid();
	}

	uint64 SyscallHandler::_sys_getppid()
	{
		return pm::k_pm.get_cur_pcb()->get_ppid();
	}

	uint64 SyscallHandler::_sys_brk()
	{
		ulong n;
		if ( _arg_addr( 0, n ) < 0 ) return -1;
		return pm::k_pm.brk( n );
	}

	uint64 SyscallHandler::_sys_kill()
	{
		int pid,sig;
		if ( _arg_int( 0, pid ) < 0 ) return -1;
		if ( _arg_int( 1, sig ) < 0 ) return -2;
		if ( sig < 0 || sig > 64 ) return -3; // invalid signal number
		return pm::k_pm.kill( pid, sig );

	}

	uint64 SyscallHandler::_sys_tgkill()
	{
		//printf("调用tgkill\n");
		return 0;

	}

	uint64 SyscallHandler::_sys_execve()
	{
		uint64 uargv, uenvp;

		eastl::string path;
		if ( _arg_str( 0, path, hsai::page_size ) < 0 ||
			 _arg_addr( 1, uargv ) < 0 || _arg_addr( 2, uenvp ) < 0 )
			return -1;

		log_trace( "execve fetch argv=%p", uargv );
		// printf("替换进程,可执行文件路径%s\n",path.c_str());

		eastl::vector<eastl::string> argv;
		ulong						 uarg;
		if ( uargv != 0 )
		{
			for ( ulong i = 0, puarg = uargv;; i++, puarg += sizeof( char * ) )
			{
				if ( i >= max_arg_num ) return -1;

				if ( _fetch_addr( puarg, uarg ) < 0 ) return -1;

				if ( uarg == 0 ) break;

				log_trace( "execve get arga[%d] = %p", i, uarg );

				argv.emplace_back( eastl::string() );
				if ( _fetch_str( uarg, argv[i], hsai::page_size ) < 0 ) return -1;
			}
		}

		eastl::vector<eastl::string> envp;
		ulong						 uenv;
		if ( uenvp != 0 )
		{
			for ( ulong i = 0, puenv = uenvp;; i++, puenv += sizeof( char * ) )
			{
				if ( i >= max_arg_num ) return -2;

				if ( _fetch_addr( puenv, uenv ) < 0 ) return -2;

				if ( uenv == 0 ) break;

				envp.emplace_back( eastl::string() );
				if ( _fetch_str( uenv, envp[i], hsai::page_size ) < 0 ) return -2;
			}
		}

		// 检查是否为 .sh 文件，如果是则使用 busybox sh 执行
		if (path.size() >= 3 && path.substr(path.size() - 3) == ".sh") {
			// 在 argv 前面插入 "./busybox" 和 "sh"
			argv.insert(argv.begin(), "sh");
			argv.insert(argv.begin(), "./busybox");
			// 修改 path 为 busybox 路径
			path = "./busybox";
		}
		// for(int i = 0; i < argv.size(); i++){
		// 	printf("argv[%d]: %s\n", i, argv[i].c_str());
		// }

		return pm::k_pm.execve( path, argv, envp );
	}

	uint64 SyscallHandler::_sys_wait()
	{
		int	   pid;
		uint64 wstatus_addr;
		if ( _arg_int( 0, pid ) < 0 ) return -1;
		if ( _arg_addr( 1, wstatus_addr ) < 0 ) return -1;
		return pm::k_pm.wait( pid, wstatus_addr );
	}

	uint64 SyscallHandler::_sys_poweroff()
	{
		dev::acpi::k_acpi_controller.power_off();
		return 0;
	}

	uint64 SyscallHandler::_sys_dup()
	{
		pm::Pcb				*p = pm::k_pm.get_cur_pcb();
		fs::file			*f;
		int					 fd;
		[[maybe_unused]] int oldfd = 0;

		if ( _arg_fd( 0, &oldfd, &f ) < 0 ) return -1;
		if ( ( fd = pm::k_pm.alloc_fd( p, f ) ) < 0 ) return -1;
		// fs::k_file_table.dup( f );
		f->dup();
		return fd;
	}

	uint64 SyscallHandler::_sys_dup2()
	{
		pm::Pcb				*p = pm::k_pm.get_cur_pcb();
		fs::file			*f;
		int					 fd;
		[[maybe_unused]] int oldfd = 0;

		if ( _arg_fd( 0, &oldfd, &f ) < 0 ) return -1;
		if ( _arg_int( 1, fd ) < 0 ) return -1;
		if ( fd == oldfd ) return fd;
		if(p->_ofile[fd]!=nullptr){
			pm::k_pm.close(fd);
		}
		if ( pm::k_pm.alloc_fd( p, f, fd ) < 0 ) return -1;
		// fs::k_file_table.dup( f );
		f->dup();
		return fd;
	}

	uint64 SyscallHandler::_sys_getcwd()
	{
		char   cwd[256];
		uint64 buf;
		int	   size;

		if ( _arg_addr( 0, buf ) < 0 ) return -1;
		if ( _arg_int( 1, size ) < 0 ) return -1;
		if ( size >= (int) sizeof( cwd ) ) return -1;

		pm::Pcb		  *p   = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt  = p->get_pagetable();
		uint		   len = pm::k_pm.getcwd( cwd );
		if ( mm::k_vmm.copyout( *pt, buf, (const void *) cwd, len ) < 0 )
			return -1;

		return buf;
	}

	uint64 SyscallHandler::_sys_gettimeofday()
	{
		uint64		 tv_addr;
		tmm::timeval tv;

		if ( _arg_addr( 0, tv_addr ) < 0 ) return -1;

		tv = tmm::k_tm.get_time_val();

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		if ( mm::k_vmm.copyout( *pt, tv_addr, (const void *) &tv,
								sizeof( tv ) ) < 0 )
			return -1;

		return 0;
	}

	uint64 SyscallHandler::_sys_sched_yield()
	{
		pm::k_scheduler.yield();
		return 0;
	}

	uint64 SyscallHandler::_sys_sleep()
	{
		tmm::timeval tv;
		uint64		 tv_addr;

		if ( _arg_addr( 0, tv_addr ) < 0 ) return -1;

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		if ( mm::k_vmm.copy_in( *pt, &tv, tv_addr, sizeof( tv ) ) < 0 )
			return -1;

		return tmm::k_tm.sleep_from_tv( tv );
	}

	uint64 SyscallHandler::_sys_times()
	{
		tmm::tms tms_val;
		uint64	 tms_addr;

		if ( _arg_addr( 0, tms_addr ) < 0 ) return -1;

		pm::k_pm.get_cur_proc_tms( &tms_val );

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		if ( mm::k_vmm.copyout( *pt, tms_addr, &tms_val, sizeof( tms_val ) ) <
			 0 )
			return -1;

		return tmm::k_tm.get_ticks();
	}

	struct _Utsname
	{
		char sysname[65];
		char nodename[65];
		char release[65];
		char version[65];
		char machine[65];
		char domainname[65];
	};
	static const char _SYSINFO_sysname[]	= "Linux";
	static const char _SYSINFO_nodename[]	= "(none-node)";
	static const char _SYSINFO_release[]	= "5.15.0";
	static const char _SYSINFO_version[]	= "#1 SMP Mon Jan 1 12:00:00 UTC 2024";
	static const char _SYSINFO_machine[]	= "LoongArch-2k1000";
	static const char _SYSINFO_domainname[] = "(none-domain)";

	uint64 SyscallHandler::_sys_uname()
	{
		uint64 usta;
		uint64 sysa, noda, rlsa, vsna, mcha, dmna;

		if ( _arg_addr( 0, usta ) < 0 ) return -11;
		sysa = (uint64) ( ( (_Utsname *) usta )->sysname );
		noda = (uint64) ( ( (_Utsname *) usta )->nodename );
		rlsa = (uint64) ( ( (_Utsname *) usta )->release );
		vsna = (uint64) ( ( (_Utsname *) usta )->version );
		mcha = (uint64) ( ( (_Utsname *) usta )->machine );
		dmna = (uint64) ( ( (_Utsname *) usta )->domainname );


		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();

		if ( mm::k_vmm.copyout( *pt, sysa, _SYSINFO_sysname,
								sizeof( _SYSINFO_sysname ) ) < 0 )
			return -1;
		if ( mm::k_vmm.copyout( *pt, noda, _SYSINFO_nodename,
								sizeof( _SYSINFO_nodename ) ) < 0 )
			return -1;
		if ( mm::k_vmm.copyout( *pt, rlsa, _SYSINFO_release,
								sizeof( _SYSINFO_release ) ) < 0 )
			return -1;
		if ( mm::k_vmm.copyout( *pt, vsna, _SYSINFO_version,
								sizeof( _SYSINFO_version ) ) < 0 )
			return -1;
		if ( mm::k_vmm.copyout( *pt, mcha, _SYSINFO_machine,
								sizeof( _SYSINFO_machine ) ) < 0 )
			return -1;
		if ( mm::k_vmm.copyout( *pt, dmna, _SYSINFO_domainname,
								sizeof( _SYSINFO_domainname ) ) < 0 )
			return -1;

		return 0;
	}

	uint64 SyscallHandler::_sys_openat()
	{
		int	   dir_fd;
		uint64 path_addr;
		int	   flags;

		if ( _arg_int( 0, dir_fd ) < 0 ) return -1;
		if ( _arg_addr( 1, path_addr ) < 0 ) return -1;
		if ( _arg_int( 2, flags ) < 0 ) return -1;

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		eastl::string  path;
		if ( mm::k_vmm.copy_str_in( *pt, path, path_addr, 100 ) < 0 ) return -1;
		int res = pm::k_pm.open( dir_fd, path, flags );
		log_trace( "openat return fd is %d", res );
		return res;
	}

	uint64 SyscallHandler::_sys_close()
	{
		int fd;

		if ( _arg_int( 0, fd ) < 0 ) return -1;
		return pm::k_pm.close( fd );
	}

	uint64 SyscallHandler::_sys_fstat()
	{
		int		  fd;
		fs::Kstat kst;
		uint64	  kst_addr;

		if ( _arg_int( 0, fd ) < 0 ) return -1;

		if ( _arg_addr( 1, kst_addr ) < 0 ) return -1;

		pm::k_pm.fstat( fd, &kst );
		mm::PageTable *pt = pm::k_pm.get_cur_pcb()->get_pagetable();
		if ( mm::k_vmm.copyout( *pt, kst_addr, &kst, sizeof( kst ) ) < 0 )
			return -1;

		return 0;
	}

	uint64 SyscallHandler::_sys_fstatat()
	{
		fs::Kstat kst;
		uint64	  kst_addr;
		int	   dir_fd;
		uint64 path_addr;
		int	   flags;

		if ( _arg_int( 0, dir_fd ) < 0 ) return -1;
		if ( _arg_addr( 1, path_addr ) < 0 ) return -1;
		if ( _arg_addr( 2, kst_addr ) < 0 ) return -1;

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		eastl::string  path;
		
		if ( mm::k_vmm.copy_str_in( *pt, path, path_addr, 200 ) < 0 ) return -1;
		

		if(dir_fd == AT_FDCWD)pm::k_pm.fstatat( dir_fd, path, &kst );
		else pm::k_pm.fstat( dir_fd, &kst );
		if ( mm::k_vmm.copyout( *pt, kst_addr, &kst, sizeof( kst ) ) < 0 )
			return -1;

		return 0;
	}

	uint64 SyscallHandler::_sys_getdents()
	{
		fs::file *f;
		uint64	  buf_addr;
		uint64	  buf_len;
		if ( _arg_fd( 0, nullptr, &f ) < 0 ) return -1;
		if ( _arg_addr( 1, buf_addr ) < 0 ) return -1;
		if ( _arg_addr( 2, buf_len ) < 0 ) return -1;

		if ( f->_attrs.filetype != fs::FileTypes::FT_NORMAL &&
				 f->_attrs.filetype != fs::FileTypes::FT_DIRECT )
			return -1;
		// eastl::string name = f->data.get_Entry()->rName();
		fs::normal_file *normal_f = static_cast<fs::normal_file *>( f );

		mm::PageTable *pt  = pm::k_pm.get_cur_pcb()->get_pagetable();

		mm::UserspaceStream us( (void *) buf_addr, buf_len, pt );

		us.open();
		u64 rlen = us.rest_space();
		normal_f->read_sub_dir( us );
		rlen -= us.rest_space();
		us.close();

		return rlen;
	}

	uint64 SyscallHandler::_sys_mkdir() 
	{
		int	   dir_fd;
		uint64 path_addr;
		int	   flags;

		if ( _arg_int( 0, dir_fd ) < 0 ) return -1;
		if ( _arg_addr( 1, path_addr ) < 0 ) return -1;
		if ( _arg_int( 2, flags ) < 0 ) return -1;

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		eastl::string  path;
		if ( mm::k_vmm.copy_str_in( *pt, path, path_addr, 100 ) < 0 ) return -1;

		int res = pm::k_pm.mkdir( dir_fd, path, flags );
		log_trace( "mkdir return is %d", res );
		return res;
	}

	uint64 SyscallHandler::_sys_chdir()
	{
		eastl::string path;

		if ( _arg_str( 0, path, hsai::page_size ) < 0 ) return -1;

		return pm::k_pm.chdir( path );
	}

	uint64 SyscallHandler::_sys_mount()
	{
		uint64		   dev_addr;
		uint64		   mnt_addr;
		uint64		   fstype_addr;
		eastl::string  dev;
		eastl::string  mnt;
		eastl::string  fstype;
		int			   flags;
		uint64		   data;
		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();

		if ( _arg_addr( 0, dev_addr ) < 0 ) return -1;
		if ( _arg_addr( 1, mnt_addr ) < 0 ) return -1;
		if ( _arg_addr( 2, fstype_addr ) < 0 ) return -1;

		if ( mm::k_vmm.copy_str_in( *pt, dev, dev_addr, 100 ) < 0 ) return -1;
		if ( mm::k_vmm.copy_str_in( *pt, mnt, mnt_addr, 100 ) < 0 ) return -1;
		if ( mm::k_vmm.copy_str_in( *pt, fstype, fstype_addr, 100 ) < 0 )
			return -1;	
				
		if ( _arg_int( 3, flags ) < 0 ) return -1;
		if ( _arg_addr( 4, data ) < 0 ) return -1;

		// return pm::k_pm.mount( dev, mnt, fstype, flags, data );
		fs::Path devpath( dev );
		fs::Path mntpath( mnt );

		return mntpath.mount( devpath, fstype, flags, data );
	}

	uint64 SyscallHandler::_sys_umount()
	{
		uint64		  specialaddr;
		eastl::string special;
		int			  flags;

		pm::Pcb		  *cur = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt  = cur->get_pagetable();

		if ( _arg_addr( 0, specialaddr ) < 0 ) return -1;
		if ( _arg_int( 1, flags ) < 0 ) return -1;

		if ( mm::k_vmm.copy_str_in( *pt, special, specialaddr, 100 ) < 0 )
			return -1;

		fs::Path specialpath( special );
		return specialpath.umount( flags );
	}

	uint64 SyscallHandler::_sys_mmap()
	{
		u64 addr;
		if ( _arg_addr( 0, addr ) < 0 ) return -1;

		size_t map_size;
		if ( _arg_addr( 1, map_size ) < 0 ) return -1;

		int prot;
		if ( _arg_int( 2, prot ) < 0 ) return -1;

		int flags;
		if ( _arg_int( 3, flags ) < 0 ) return -1;

		int fd;
		if ( _arg_int( 4, fd ) < 0 ) return -1;

		size_t length;
		if ( _arg_addr( 5, length ) < 0 ) return -1;
		return pm::k_pm.mmap( fd, prot, flags, map_size );
	}

	//unmap + mmap
	uint64 SyscallHandler::_sys_mremap()
	{
		uint64 oldaddr;
		int oldsize;
		int newsize;
		int flags;
		if ( _arg_addr( 0, oldaddr ) < 0 ) return -1;

		if ( _arg_int( 1, oldsize ) < 0 ) return -1;

		if ( _arg_int( 2, newsize ) < 0 ) return -1;

		if ( _arg_int( 3, flags ) < 0 ) return -1;

		return pm::k_pm.mremap( oldaddr, oldsize, newsize );
	}

	uint64 SyscallHandler::_sys_munmap() { 
		u64 start;
		if ( _arg_addr( 0, start ) < 0 ) return -1;

		size_t len;
		if ( _arg_addr( 1, len ) < 0 ) return -1;
		return pm::k_pm.munmap( start, len );
	}

	uint64 SyscallHandler::_sys_statx()
	{
		using __u16 = uint16;
		using __u32 = uint32;
		using __s64 = int64;
		using __u64 = uint64;


		struct statx_timestamp
		{
			__s64 tv_sec;  /* Seconds since the Epoch (UNIX time) */
			__u32 tv_nsec; /* Nanoseconds since tv_sec */
		};
		struct statx
		{
			__u32 stx_mask;		  /* Mask of bits indicating
									 filled fields */
			__u32 stx_blksize;	  /* Block size for filesystem I/O */
			__u64 stx_attributes; /* Extra file attribute indicators */
			__u32 stx_nlink;	  /* Number of hard links */
			__u32 stx_uid;		  /* User ID of owner */
			__u32 stx_gid;		  /* Group ID of owner */
			__u16 stx_mode;		  /* File type and mode */
			__u64 stx_ino;		  /* Inode number */
			__u64 stx_size;		  /* Total size in bytes */
			__u64 stx_blocks;	  /* Number of 512B blocks allocated */
			__u64 stx_attributes_mask;
			/* Mask to show what's supported
			   in stx_attributes */

			/* The following fields are file timestamps */
			struct statx_timestamp stx_atime; /* Last access */
			struct statx_timestamp stx_btime; /* Creation */
			struct statx_timestamp stx_ctime; /* Last status change */
			struct statx_timestamp stx_mtime; /* Last modification */

			/* If this file represents a device, then the next two
			   fields contain the ID of the device */
			__u32 stx_rdev_major; /* Major ID */
			__u32 stx_rdev_minor; /* Minor ID */

			/* The next two fields contain the ID of the device
			   containing the filesystem where the file resides */
			__u32 stx_dev_major; /* Major ID */
			__u32 stx_dev_minor; /* Minor ID */

			__u64 stx_mnt_id; /* Mount ID */

			/* Direct I/O alignment restrictions */
			__u32 stx_dio_mem_align;
			__u32 stx_dio_offset_align;
		};
// 		  uint32 stx_mask;
//   uint32 stx_blksize;
//   uint64 stx_attributes;
//   uint32 stx_nlink;
//   uint32 stx_uid;
//   uint32 stx_gid;
//   uint16 stx_mode;
//   uint16 pad1;
//   uint64 stx_ino;
//   uint64 stx_size;
//   uint64 stx_blocks;
//   uint64 stx_attributes_mask;
//   struct
//   {
//     int64 tv_sec;
//     uint32 tv_nsec;
//     int pad;
//   } stx_atime, stx_btime, stx_ctime, stx_mtime;
//   uint32 stx_rdev_major;
//   uint32 stx_rdev_minor;
//   uint32 stx_dev_major;
//   uint32 stx_dev_minor;
//   uint64 spare[14];

		int			  fd;
		eastl::string path_name;
		fs::Kstat	  kst;
		statx		  stx;
		uint64		  kst_addr;

		if ( _arg_int( 0, fd ) < 0 ) return -1;

		if ( _arg_str( 1, path_name, 128 ) < 0 ) return -1;

		if ( _arg_addr( 4, kst_addr ) < 0 ) return -1;

		if ( fd > 0 )
		{
			pm::k_pm.fstat( fd, &kst );
			stx.stx_mode	  = kst.mode;
			stx.stx_size	  = kst.size;
			stx.stx_dev_minor   =(kst.dev)>>32;
			stx.stx_dev_major   =(kst.dev)&(0xFFFFFFFF);
			// dev=(int)(stx_dev_major<<8 | stx_dev_minor&0xff)
			stx.stx_ino       =kst.ino;
			stx.stx_nlink     =kst.nlink;
			stx.stx_atime.tv_sec = kst.st_atime_sec;
			stx.stx_atime.tv_nsec = kst.st_atime_nsec;
			stx.stx_mtime.tv_sec = kst.st_mtime_sec;
			stx.stx_mtime.tv_nsec = kst.st_mtime_nsec;
			stx.stx_ctime.tv_sec = kst.st_ctime_sec;
			stx.stx_ctime.tv_nsec = kst.st_ctime_nsec;
			mm::PageTable *pt = pm::k_pm.get_cur_pcb()->get_pagetable();
			if ( mm::k_vmm.copyout( *pt, kst_addr, &stx, sizeof( stx ) ) < 0 )
				return -1;
			return 0;
		}
		//
		return -1;
	}

	uint64 SyscallHandler::_sys_unlinkat()
	{
		int	   fd, flags;
		uint64 path_addr;

		if ( _arg_int( 0, fd ) < 0 ) return -1;
		if ( _arg_addr( 1, path_addr ) < 0 ) return -1;
		if ( _arg_int( 2, flags ) < 0 ) return -1;
		eastl::string  path;
		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		if ( mm::k_vmm.copy_str_in( *pt, path, path_addr, 100 ) < 0 ) return -1;

		int res = pm::k_pm.unlink( fd, path, flags );
		return res;
	}

	uint64 SyscallHandler::_sys_pipe()
	{
		int	   fd[2];
		uint64 addr;

		if ( _arg_addr( 0, addr ) < 0 ) return -1;

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		if ( mm::k_vmm.copy_in( *pt, &fd, addr, 2 * sizeof( fd[0] ) ) < 0 )
			return -1;

		if ( pm::k_pm.pipe( fd, 0 ) < 0 ) return -1;

		if ( mm::k_vmm.copyout( *pt, addr, &fd, 2 * sizeof( fd[0] ) ) < 0 )
			return -1;

		return 0;
	}

	uint64 SyscallHandler::_sys_set_tid_address()
	{
		ulong addr;
		int	 *tidptr;

		if ( _arg_addr( 0, addr ) < 0 ) return -1;

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		tidptr = (int *) hsai::k_mem->to_vir( pt->walk_addr( addr ) );
		if ( tidptr == nullptr ) return -10;

		return pm::k_pm.set_tid_address( tidptr );
	}

	uint64 SyscallHandler::_sys_set_robust_list()
	{
		ulong				  addr;
		pm::robust_list_head *head;
		size_t				  len;

		if ( _arg_addr( 0, addr ) < 0 ) return -1;

		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		head			  = (pm::robust_list_head *) hsai::k_mem->to_vir(
			 pt->walk_addr( addr ) );

		if ( head == nullptr ) return -10;

		if ( _arg_addr( 1, len ) < 0 ) return -2;

		return pm::k_pm.set_robust_list( head, len );
	}

	uint64 SyscallHandler::_sys_prlimit64()
	{
		int pid;
		if ( _arg_int( 0, pid ) < 0 ) return -1;

		int rsrc;
		if ( _arg_int( 1, rsrc ) < 0 ) return -2;

		u64 new_limit;
		u64 old_limit;
		if ( _arg_addr( 2, new_limit ) < 0 ) return -3;
		if ( _arg_addr( 3, old_limit ) < 0 ) return -4;

		pm::rlimit64  *nlim = nullptr, *olim = nullptr;
		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		if ( new_limit != 0 )
			nlim = (pm::rlimit64 *) hsai::k_mem->to_vir(
				pt->walk_addr( new_limit ) );
		if ( old_limit != 0 )
			olim = (pm::rlimit64 *) hsai::k_mem->to_vir(
				pt->walk_addr( old_limit ) );

		return pm::k_pm.prlimit64( pid, rsrc, nlim, olim );
	}

	uint64 SyscallHandler::_sys_clock_gettime()
	{
		int clock_id;
		if ( _arg_int( 0, clock_id ) < 0 ) return -1;

		u64 addr;
		if ( _arg_addr( 1, addr ) < 0 ) return -2;

		tmm::timespec *tp = nullptr;
		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		if ( addr != 0 )
			tp = (tmm::timespec *) hsai::k_mem->to_vir( pt->walk_addr( addr ) );

		tmm::SystemClockId cid = (tmm::SystemClockId) clock_id;

		return tmm::k_tm.clock_gettime( cid, tp );
	}

	uint64 SyscallHandler::_sys_mprotect() { return 0; }

	uint64 SyscallHandler::_sys_getuid() { return 0; }

	uint64 SyscallHandler::_sys_readlinkat()
	{
		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		int			   fd;
		fs::Path	   filePath;
		size_t		   ret;

		if ( _arg_int( 0, fd ) < 0 ) return -1;

		eastl::string path;
		if ( _arg_str( 1, path, 256 ) < 0 ) return -1;

		uint64 buf;
		if ( _arg_addr( 2, buf ) < 0 ) return -1;

		size_t buf_size;
		if ( _arg_addr( 3, buf_size ) < 0 ) return -1;

		if ( fd == AT_FDCWD )
			new ( &filePath ) fs::Path( path, p->_cwd );
		else
			new ( &filePath ) fs::Path( path, p->_ofile[fd] );

		fs::dentry *dent = filePath.pathSearch();
		if ( dent == nullptr ) return -1;

		char *buffer = new char[buf_size];
		ret			 = dent->getNode()->readlinkat( buffer, buf_size );

		if ( mm::k_vmm.copyout( *pt, buf, (void *) buffer, ret ) < 0 )
		{
			delete[] buffer;
			return -1;
		}

		delete[] buffer;
		return ret;
	}

	uint64 SyscallHandler::_sys_getrandom()
	{
		uint64				 bufaddr;
		int					 buflen;
		[[maybe_unused]] int flags;
		pm::Pcb				*pcb = pm::k_pm.get_cur_pcb();
		mm::PageTable		*pt	 = pcb->get_pagetable();

		if ( _arg_addr( 0, bufaddr ) < 0 ) return -1;

		if ( _arg_int( 1, buflen ) < 0 ) return -1;

		if ( _arg_int( 2, buflen ) < 0 ) return -1;

		if ( bufaddr == 0 && buflen == 0 ) return -1;

		char *k_buf = new char[buflen];
		if ( !k_buf ) return -1;

		ulong  random	   = 0x4249'4C47'4B43'5546UL;
		size_t random_size = sizeof( random );
		for ( size_t i	= 0; i < static_cast<size_t>( buflen );
			  i		   += random_size )
		{
			size_t copy_size =
				( i + random_size ) <= static_cast<size_t>( buflen )
					? random_size
					: buflen - i;
			memcpy( k_buf + i, &random, copy_size );
		}
		if ( mm::k_vmm.copyout( *pt, bufaddr, k_buf, buflen ) < 0 ) return -1;

		delete[] k_buf;
		return buflen;
	}

	uint64 SyscallHandler::_sys_sigaction()
	{
		pm::Pcb						   *proc = pm::k_pm.get_cur_pcb();
		[[maybe_unused]] mm::PageTable *pt	 = proc->get_pagetable();
		[[maybe_unused]] pm::ipc::signal::sigaction a_newact, a_oldact;
		// a_newact = nullptr;
		// a_oldact = nullptr;
		uint64										newactaddr, oldactaddr;
		int											flag;
		int											ret = -1;

		if ( _arg_int( 0, flag ) < 0 ) return -1;

		if ( _arg_addr( 1, newactaddr ) < 0 ) return -1;

		if ( _arg_addr( 2, oldactaddr ) < 0 ) return -1;

		if ( oldactaddr != 0 ) a_oldact = pm::ipc::signal::sigaction();

		if ( newactaddr != 0 )
		{
			if ( mm::k_vmm.copy_in( *pt, &a_newact, newactaddr,
									sizeof( pm::ipc::signal::sigaction ) ) < 0 )
				return -1;
			// a_newact = ( pm::ipc::signal::sigaction *)(hsai::k_mem->to_vir(
			// pt->walk_addr( newactaddr ) ));
			ret = pm::ipc::signal::sigAction( flag, &a_newact, nullptr );
		}
		else { ret = pm::ipc::signal::sigAction( flag, &a_newact, &a_oldact ); }
		if ( ret == 0 && oldactaddr != 0 )
		{
			if ( mm::k_vmm.copyout( *pt, oldactaddr, &a_oldact,
									sizeof( pm::ipc::signal::sigaction ) ) < 0 )
				return -1;
		}
		return ret;
	}

	uint64 SyscallHandler::_sys_ioctl()
	{
		int tmp;

		fs::file *f = nullptr;
		int		  fd;
		if ( _arg_fd( 0, &fd, &f ) < 0 ) return -1;
		if ( f == nullptr ) return -1;
		fd = fd;

		if ( f->_attrs.filetype != fs::FileTypes::FT_DEVICE ) return -1;

		u32 cmd;
		if ( _arg_int( 1, tmp ) < 0 ) return -2;
		cmd = (u32) tmp;
		cmd = cmd;

		ulong arg;
		if ( _arg_addr( 2, arg ) < 0 ) return -3;

		/// @todo not implement
		printf("ioctl cmd is %x\n", cmd);
		if ( ( cmd & 0xFFFF ) == TCGETS )
		{
			fs::device_file *df = (fs::device_file *) f;
			mm::PageTable	*pt = pm::k_pm.get_cur_pcb()->get_pagetable();
			termios			*ts =
				(termios *) hsai::k_mem->to_vir( pt->walk_addr( arg ) );
			return df->tcgetattr( ts );
		}

		if ( ( cmd & 0XFFFF ) == TIOCGPGRP )
		{
			mm::PageTable *pt = pm::k_pm.get_cur_pcb()->get_pagetable();
			int *p_pgrp = (int *) hsai::k_mem->to_vir( pt->walk_addr( arg ) );
			*p_pgrp		= 1;
			return 0;
		}

		if( ( cmd & 0XFFFF ) == TIOCGWINSZ )
		{
			winsize ws;
			ws.ws_col = 80;
			ws.ws_row = 24;
			mm::PageTable *pt = pm::k_pm.get_cur_pcb()->get_pagetable();
			if (mm::k_vmm.copyout(*pt, arg, (char*)&ws, sizeof(ws)) < 0)
				return -1;
			return 0;
		}

		return 0;
	}

	uint64 SyscallHandler::_sys_fcntl()
	{
		pm::Pcb	 *p = pm::k_pm.get_cur_pcb();
		fs::file *f = nullptr;
		int		  op;
		ulong	  arg;
		int		  retfd = -1;
		int fd;

		if ( _arg_fd( 0, &fd, &f ) < 0 ) return -1;
		if ( _arg_int( 1, op ) < 0 ) return -2;

		switch ( op )
		{
			case F_SETFD:
				if ( _arg_addr( 2, arg ) < 0 ) return -3;
				if ( arg & FD_CLOEXEC ) f->_fl_cloexec = true;
				return 0;

			case F_DUPFD:
				if ( _arg_addr( 2, arg ) < 0 ) return -3;
				for ( int i = (int) arg; i < (int) pm::max_open_files; ++i )
				{
					if ( ( retfd = pm::k_pm.alloc_fd( p, f, i ) ) == i )
					{
						f->refcnt++;
						break;
					}
				}
				return retfd;

			case F_DUPFD_CLOEXEC:
				if ( _arg_addr( 2, arg ) < 0 ) return -3;
				for ( int i = (int) arg; i < (int) pm::max_open_files; ++i )
				{
					if ( ( retfd = pm::k_pm.alloc_fd( p, f, i ) ) == i )
					{
						f->refcnt++;
						break;
					}
				}
				p->get_open_file( retfd )->_fl_cloexec = true;
				return retfd;
			case F_GETFL:
				return f->_attrs.transMode();
			case F_GETFD: return 0;
			case F_SETFL:
				if ((((arg & O_NONBLOCK) == O_NONBLOCK) || ((arg & O_APPEND) == O_APPEND))) {
					if ( f->_attrs.filetype == fs::FileTypes::FT_NORMAL )
					{
						fs::normal_file *normal_f = static_cast<fs::normal_file *>( f );
						normal_f->setAppend();
					}
				}
				return 0;
			default: break;
		}

		return retfd;
	}

	uint64 SyscallHandler::_sys_getpgid()
	{
		int pid;
		if ( _arg_int( 0, pid ) < 0 ) return -1;
		return 0;
	}

	uint64 SyscallHandler::_sys_setpgid()
	{
		int pid, pgid;
		if ( _arg_int( 0, pid ) < 0 || _arg_int( 1, pgid ) < 0 ) return -1;
		return 0;
	}

	uint64 SyscallHandler::_sys_geteuid() { return 0; }

	uint64 SyscallHandler::_sys_getegid() { return 0; }

	uint64 SyscallHandler::_sys_ppoll()
	{
		uint64					  fds_addr;
		uint64					  timeout_addr;
		uint64					  sigmask_addr;
		pollfd					 *fds = nullptr;
		int						  nfds;
		[[maybe_unused]] timespec tm{ 0, 0 }; // 现在没用上
		[[maybe_unused]] sigset_t sigmask;	  // 现在没用上
		[[maybe_unused]] int	  timeout;	  // 现在没用上
		int						  ret = 0;

		pm::Pcb		  *proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt	= proc->get_pagetable();

		if ( _arg_addr( 0, fds_addr ) < 0 ) return -1;

		if ( _arg_int( 1, nfds ) < 0 ) return -1;

		if ( _arg_addr( 2, timeout_addr ) < 0 ) return -1;

		if ( _arg_addr( 3, sigmask_addr ) < 0 ) return -1;

		fds = new pollfd[nfds];
		if ( fds == nullptr ) return -2;
		for ( int i = 0; i < nfds; i++ )
		{
			if ( mm::k_vmm.copy_in( *pt, &fds[i],
									fds_addr + i * sizeof( pollfd ),
									sizeof( pollfd ) ) < 0 )
			{
				delete[] fds;
				return -1;
			}
		}

		if ( timeout_addr != 0 )
		{
			if ( ( mm::k_vmm.copy_in( *pt, &tm, timeout_addr, sizeof( tm ) ) ) <
				 0 )
			{
				delete[] fds;
				return -1;
			}
			timeout = tm.tv_sec * 1000 + tm.tv_nsec / 1'000'000;
		}
		else
			timeout = -1;

		if ( sigmask_addr != 0 )
			if ( mm::k_vmm.copy_in( *pt, &sigmask, sigmask_addr,
									sizeof( sigset_t ) ) < 0 )
			{
				delete[] fds;
				return -1;
			}

		while ( 1 )
		{
			for ( auto i = 0; i < nfds; i++ )
			{
				fds[i].revents = 0;
				if ( fds[i].fd < 0 ) { continue; }

				fs::file *f	   = nullptr;
				int		  reti = 0;

				if ( ( f = proc->get_open_file( fds[i].fd ) ) == nullptr )
				{
					fds[i].revents |= POLLNVAL;
					reti			= 1;
				}
				else
				{
					if ( fds[i].events & POLLIN )
					{
						if ( f->read_ready() )
						{
							fds[i].revents |= POLLIN;
							reti			= 1;
						}
					}
					if ( fds[i].events & POLLOUT )
					{
						if ( f->write_ready() )
						{
							fds[i].revents |= POLLOUT;
							reti			= 1;
						}
					}
				}

				ret += reti;
			}
			if ( ret != 0 ) break;
			// else
			// {
			// 	/// @todo sleep
			// }
		}

		if ( mm::k_vmm.copyout( *pt, fds_addr, fds, nfds * sizeof( pollfd ) ) <
			 0 )
		{
			delete[] fds;
			return -1;
		}

		delete[] fds;
		return ret;
	}

	uint64 SyscallHandler::_sys_getgid() { return 0; }

	uint64 SyscallHandler::_sys_setgid() { 
		int gid;
		_arg_int(0, gid);
		return pm::k_pm.set_gid(gid);
	}

	uint64 SyscallHandler::_sys_setuid() { 
		int uid;
		_arg_int(0, uid);
		return pm::k_pm.set_uid(uid);
	}

	uint64 SyscallHandler::_sys_gettid() { return pm::k_pm.get_cur_pcb()->_pid; }

	uint64 SyscallHandler::_sys_sendfile()
	{
		int		  in_fd, out_fd;
		fs::file *in_f, *out_f;
		if ( _arg_fd( 0, &out_fd, &out_f ) < 0 ) return -1;
		if ( _arg_fd( 1, &in_fd, &in_f ) < 0 ) return -2;

		ulong  addr;
		ulong *p_off = nullptr;
		p_off		 = p_off;
		if ( _arg_addr( 2, addr ) < 0 ) return -3;

		mm::PageTable *pt = pm::k_pm.get_cur_pcb()->get_pagetable();
		if ( addr != 0 )
			p_off = (ulong *) hsai::k_mem->to_vir( pt->walk_addr( addr ) );

		size_t count;
		if ( _arg_addr( 3, count ) < 0 ) return -4;

		/// @todo sendfile

		ulong start_off = in_f->get_file_offset();
		if ( p_off != nullptr ) start_off = *p_off;

		char *buf = new char[count + 1];
		if ( buf == nullptr ) return -5;

		int readcnt	 = in_f->read( (ulong) buf, count, start_off, true );
		int writecnt = 0;
		if ( out_f->_attrs.filetype == fs::FileTypes::FT_PIPE )
			writecnt = ( (fs::pipe_file *) out_f )
						   ->write_in_kernel( (ulong) buf, readcnt );
		else
			writecnt = out_f->write( (ulong) buf, readcnt,
									 out_f->get_file_offset(), true );

		delete[] buf;

		if ( p_off != nullptr ) *p_off += writecnt;

		return writecnt;
	}

	uint64 SyscallHandler::_sys_exit_group()
	{
		int status;
		if ( _arg_int( 0, status ) < 0 ) return -1;
		pm::k_pm.exit_group( status );
		return -111; // not return;
	}

	uint64 SyscallHandler::_sys_statfs()
	{
		eastl::string  path;
		uint64		   statfsaddr;
		mm::PageTable *pt = pm::k_pm.get_cur_pcb()->get_pagetable();

		if ( _arg_str( 0, path, 128 ) < 0 ) return -1;

		if ( _arg_addr( 1, statfsaddr ) < 0 ) return -2;

		fs::Path   fspath( path );
		fs::statfs statfs_ =
			fs::statfs( *fspath.pathSearch()->getNode()->getFS() );

		if ( mm::k_vmm.copyout( *pt, statfsaddr, &statfs_, sizeof( statfs_ ) ) <
			 0 )
			return -1;
		return 0;
	}

	uint64 SyscallHandler::_sys_syslog()
	{
		enum sys_log_type
		{

			SYSLOG_ACTION_CLOSE			= 0,
			SYSLOG_ACTION_OPEN			= 1,
			SYSLOG_ACTION_READ			= 2,
			SYSLOG_ACTION_READ_ALL		= 3,
			SYSLOG_ACTION_READ_CLEAR	= 4,
			SYSLOG_ACTION_CLEAR			= 5,
			SYSLOG_ACTION_CONSOLE_OFF	= 6,
			SYSLOG_ACTION_CONSOLE_ON	= 7,
			SYSLOG_ACTION_CONSOLE_LEVEL = 8,
			SYSLOG_ACTION_SIZE_UNREAD	= 9,
			SYSLOG_ACTION_SIZE_BUFFER	= 10

		};

		int			  prio;
		eastl::string fmt;
		uint64		  fmt_addr;
		eastl::string msg = "Spectre V2 : Update user space SMT mitigation: STIBP always-on\n"
							"process_manager : execve set stack-base = 0x0000_0000_9194_5000\n"
							"pm/process_manager : execve set page containing sp is 0x0000_0000_9196_4000";
		[[maybe_unused]] pm::Pcb	   *p  = pm::k_pm.get_cur_pcb();
		[[maybe_unused]] mm::PageTable *pt = p->get_pagetable();

		if ( _arg_int( 0, prio ) < 0 ) return -1;

		if ( _arg_addr( 1, fmt_addr ) < 0 ) return -1;


		if ( prio == SYSLOG_ACTION_SIZE_BUFFER )
			return msg.size(); // 返回buffer的长度
		else if ( prio == SYSLOG_ACTION_READ_ALL )
		{
			mm::k_vmm.copyout( *pt, fmt_addr, msg.c_str(), msg.size() );
			return msg.size();
		}

		return 0;
	}

	uint64 SyscallHandler::_sys_faccessat()
	{
		int			  _dirfd;
		uint64		  _pathaddr;
		eastl::string _pathname;
		int			  _mode;
		int			  _flags;

		if ( _arg_int( 0, _dirfd ) < 0 ) return -1;

		if ( _arg_addr( 1, _pathaddr ) < 0 ) return -1;

		if ( _arg_int( 2, _mode ) < 0 ) return -1;

		if ( _arg_int( 3, _flags ) < 0 ) return -1;
		pm::Pcb		  *cur_proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt		= cur_proc->get_pagetable();

		if ( mm::k_vmm.copy_str_in( *pt, _pathname, _pathaddr, 100 ) < 0 )
			return -1;
		if ( _pathname.empty() ) return -1;

		[[maybe_unused]] int flags = 0;
		// if( ( _mode & ( R_OK | X_OK )) && ( _mode & W_OK ) )
		// 	flags = 6;    	//O_RDWR;
		// else if( _mode & W_OK )
		// 	flags = 2;		//O_WRONLY + 1;
		// else if( _mode & ( R_OK | X_OK ))
		// 	flags = 4		//O_RDONLY + 1;

		if ( _mode & R_OK ) flags |= 4;
		if ( _mode & W_OK ) flags |= 2;
		if ( _mode & X_OK ) flags |= 1;

		fs::Path path;
		if ( _dirfd == -100 ) // AT_CWD
			new ( &path ) fs::Path( _pathname, cur_proc->_cwd );
		else
			new ( &path ) fs::Path( _pathname, cur_proc->_ofile[_dirfd] );

		int fd = path.open( fs::FileAttrs( flags ), flags );

		if ( fd < 0 )
			return -1;
		pm::k_pm.close( fd ); // 只检查权限，不需要打开文件

		return 0;
	}

	uint64 SyscallHandler::_sys_madvise() { return 0; }

	uint64 SyscallHandler::_sys_sysinfo()
	{
		// struct sysinfo {
		// 	__kernel_long_t uptime;		/* Seconds since boot */
		// 	__kernel_ulong_t loads[3];	/* 1, 5, and 15 minute load averages */
		// 	__kernel_ulong_t totalram;	/* Total usable main memory size */
		// 	__kernel_ulong_t freeram;	/* Available memory size */
		// 	__kernel_ulong_t sharedram;	/* Amount of shared memory */
		// 	__kernel_ulong_t bufferram;	/* Memory used by buffers */
		// 	__kernel_ulong_t totalswap;	/* Total swap space size */
		// 	__kernel_ulong_t freeswap;	/* swap space still available */
		// 	__u16 procs;		   	/* Number of current processes */
		// 	__u16 pad;		   	/* Explicit padding for m68k */
		// 	__kernel_ulong_t totalhigh;	/* Total high memory size */
		// 	__kernel_ulong_t freehigh;	/* Available high memory size */
		// 	__u32 mem_unit;			/* Memory unit size in bytes */
		// 	char _f[20-2*sizeof(__kernel_ulong_t)-sizeof(__u32)];	/* Padding:
		// libc5 uses this.. */
		// };


		uint64					 sysinfoaddr;
		[[maybe_unused]] sysinfo sysinfo_;

		if ( _arg_addr( 0, sysinfoaddr ) < 0 ) return -1;

		pm::Pcb		  *cur_proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt		= cur_proc->get_pagetable();

		memset( &sysinfo_, 0, sizeof( sysinfo_ ) );
		sysinfo_.uptime	   = 0;
		sysinfo_.loads[0]  = 0; // 负载均值  1min 5min 15min
		sysinfo_.loads[1]  = 0;
		sysinfo_.loads[2]  = 0;
		sysinfo_.totalram  = 0; // 总内存
		sysinfo_.freeram   = 0;
		sysinfo_.sharedram = 0;
		sysinfo_.bufferram = 0;
		sysinfo_.totalswap = 0;
		sysinfo_.freeswap  = 0;
		sysinfo_.procs	   = 0;
		sysinfo_.pad	   = 0;
		sysinfo_.totalhigh = 0;
		sysinfo_.freehigh  = 0;
		sysinfo_.mem_unit  = 1; // 内存单位为 1 字节

		if ( mm::k_vmm.copyout( *pt, sysinfoaddr, &sysinfo_,
								sizeof( sysinfo_ ) ) < 0 )
			return -1;

		return 0;
	}

	uint64 SyscallHandler::_sys_nanosleep()
	{
		int		 clockid;
		int		 flags;
		timespec dur;
		uint64	 dur_addr;
		timespec rem;
		uint64	 rem_addr;

		if ( _arg_int( 0, clockid ) < 0 ) return -1;

		if ( _arg_int( 1, flags ) < 0 ) return -1;

		if ( _arg_addr( 2, dur_addr ) < 0 ) return -1;

		if ( _arg_addr( 3, rem_addr ) < 0 ) return -2;

		pm::Pcb		  *cur_proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt		= cur_proc->get_pagetable();

		if ( dur_addr != 0 )
			if ( mm::k_vmm.copy_in( *pt, &dur, dur_addr, sizeof( dur ) ) < 0 )
				return -1;

		if ( rem_addr != 0 )
			if ( mm::k_vmm.copy_in( *pt, &rem, rem_addr, sizeof( rem ) ) < 0 )
				return -1;

		tmm::timeval tm_;
		tm_.tv_sec	= dur.tv_sec;
		tm_.tv_usec = dur.tv_nsec / 1000;

		tmm::k_tm.sleep_from_tv( tm_ );

		return 0;
	}

	uint64 SyscallHandler::_sys_getrusage()
	{
		struct rusage
		{
			/* Total amount of user time used.  */
			struct timeval ru_utime;
			/* Total amount of system time used.  */
			struct timeval ru_stime;
			/* Maximum resident set size (in kilobytes).  */
			__extension__ union
			{
			long int ru_maxrss;
			__syscall_slong_t __ru_maxrss_word;
			};
			/* Amount of sharing of text segment memory
			with other processes (kilobyte-seconds).  */
			__extension__ union
			{
			long int ru_ixrss;
			__syscall_slong_t __ru_ixrss_word;
			};
			/* Amount of data segment memory used (kilobyte-seconds).  */
			__extension__ union
			{
			long int ru_idrss;
			__syscall_slong_t __ru_idrss_word;
			};
			/* Amount of stack memory used (kilobyte-seconds).  */
			__extension__ union
			{
			long int ru_isrss;
			__syscall_slong_t __ru_isrss_word;
			};
			/* Number of soft page faults (i.e. those serviced by reclaiming
			a page from the list of pages awaiting reallocation.  */
			__extension__ union
			{
			long int ru_minflt;
			__syscall_slong_t __ru_minflt_word;
			};
			/* Number of hard page faults (i.e. those that required I/O).  */
			__extension__ union
			{
			long int ru_majflt;
			__syscall_slong_t __ru_majflt_word;
			};
			/* Number of times a process was swapped out of physical memory.  */
			__extension__ union
			{
			long int ru_nswap;
			__syscall_slong_t __ru_nswap_word;
			};
			/* Number of input operations via the file system.  Note: This
			and `ru_oublock' do not include operations with the cache.  */
			__extension__ union
			{
			long int ru_inblock;
			__syscall_slong_t __ru_inblock_word;
			};
			/* Number of output operations via the file system.  */
			__extension__ union
			{
			long int ru_oublock;
			__syscall_slong_t __ru_oublock_word;
			};
			/* Number of IPC messages sent.  */
			__extension__ union
			{
			long int ru_msgsnd;
			__syscall_slong_t __ru_msgsnd_word;
			};
			/* Number of IPC messages received.  */
			__extension__ union
			{
			long int ru_msgrcv;
			__syscall_slong_t __ru_msgrcv_word;
			};
			/* Number of signals delivered.  */
			__extension__ union
			{
			long int ru_nsignals;
			__syscall_slong_t __ru_nsignals_word;
			};
			/* Number of voluntary context switches, i.e. because the process
			gave up the process before it had to (usually to wait for some
			resource to be available).  */
			__extension__ union
			{
			long int ru_nvcsw;
			__syscall_slong_t __ru_nvcsw_word;
			};
			/* Number of involuntary context switches, i.e. a higher priority process
			became runnable or the current process used up its time slice.  */
			__extension__ union
			{
			long int ru_nivcsw;
			__syscall_slong_t __ru_nivcsw_word;
			};
		};

		enum __rusage_who
		{
			/* The calling process.  */
			RUSAGE_SELF = 0,
			#define RUSAGE_SELF RUSAGE_SELF

			/* All of its terminated child processes.  */
			RUSAGE_CHILDREN = -1
			#define RUSAGE_CHILDREN RUSAGE_CHILDREN

			#ifdef __USE_GNU
			,
			/* The calling thread.  */
			RUSAGE_THREAD = 1
			# define RUSAGE_THREAD RUSAGE_THREAD
			/* Name for the same functionality on Solaris.  */
			# define RUSAGE_LWP RUSAGE_THREAD
			#endif
		};

		int who;
		uint64 rusage_addr;

		if( _arg_int( 0, who ) < 0 ) 
			return -1;
		
		if( _arg_addr( 1, rusage_addr ) < 0 ) 
			return -1;
		
		
		rusage rusage_;
		pm::Pcb *cur_proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = cur_proc->get_pagetable();
		tmm::tms tms;
		pm::k_pm.get_cur_proc_tms( &tms );

		 switch (who) {
			case RUSAGE_SELF:
				rusage_.ru_utime.tv_sec = tms.tms_utime;
				rusage_.ru_utime.tv_usec = tms.tms_cutime;
				rusage_.ru_maxrss = cur_proc->get_max_rss();
				break;
			case RUSAGE_CHILDREN:
				// 添加对 RUSAGE_CHILDREN 的处理逻辑
				// 假设有一个函数 get_children_rusage() 来获取子进程的资源使用情况
				//rusage_ = cur_proc->get_children_rusage();
				break;
			default:
				log_error("get_rusage: invalid who\n");
				return -1;
			
		}
		rusage_.ru_ixrss = 0;
		rusage_.ru_idrss = 0;
		rusage_.ru_isrss = 0;
		rusage_.ru_minflt = 0;   /// @todo: 缺页中断的次数
		rusage_.ru_majflt = 0;   /// @todo: 页错误次数
		rusage_.ru_nswap = 0;
		rusage_.ru_inblock = 0;
		rusage_.ru_oublock = 0;
		rusage_.ru_msgsnd = 0;
		rusage_.ru_msgrcv = 0;
		rusage_.ru_nsignals = 0;
		rusage_.ru_nvcsw = 0;
		rusage_.ru_nivcsw = 0;

		if( mm::k_vmm.copyout( *pt, rusage_addr, &rusage_, sizeof( rusage_ ) ) < 0 )
			return -1;
		
		return 0;
	}

	uint64 SyscallHandler::_sys_utimensat()
	{
		int dirfd;
		uint64 pathaddr;
		eastl::string pathname;
		uint64 timespecaddr;
		timespec atime;
		timespec mtime;
		int flags;

		if( _arg_int( 0, dirfd ) < 0 )
			return -1;
		
		if( _arg_addr( 1, pathaddr ) < 0 )
			return -1;
		
		if( _arg_addr( 2, timespecaddr ) < 0 )
			return -1;

		if( _arg_int( 3, flags ) < 0 )
			return -1;
		
		pm::Pcb * cur_proc = pm::k_pm.get_cur_pcb();	
		mm::PageTable *pt = cur_proc->get_pagetable();
		fs::dentry *base;
		
		if( dirfd == AT_FDCWD )
			base = cur_proc->_cwd;
		else
			base = static_cast<fs::normal_file *>( cur_proc->_ofile[dirfd] )->getDentry();

		if( pathaddr==0 || mm::k_vmm.copy_str_in( *pt, pathname, pathaddr, 128 ) < 0 )
			return -1;

		if( timespecaddr == 0 )
		{
			// @todo: 设置为当前时间
			//atime = NOW;	
			//mtime = NOw;
		}	
		else
		{
			if( mm::k_vmm.copy_in( *pt, &atime, timespecaddr, sizeof( atime ) ) < 0 )
				return -1;
			
			if( mm::k_vmm.copy_in( *pt, &mtime, timespecaddr + sizeof( atime ), sizeof( mtime ) ) < 0 )
				return -1;
		}
		
		if( _arg_int( 3, flags ) < 0 )
			return -1;
		
		fs::Path path( pathname, base );	
		fs::dentry *den = path.pathSearch();	
		if( den == nullptr )
			return -ENOENT;
		
		//int fd = path.open();
		
		return 0;
	}

	uint64 SyscallHandler::_sys_lseek()
	{
		int fd;
		int offset;
		int whence;

		if( _arg_int( 0, fd ) < 0 )
			return -1;
		
		if( _arg_int( 1, offset ) < 0 )
			return -1;
		
		if( _arg_int( 2, whence ) < 0 )
			return -1;
		if( fd < 0 || fd >= pm::max_open_files )return -1;
		
		pm::Pcb *cur_proc = pm::k_pm.get_cur_pcb();
		fs::file *f = cur_proc->_ofile[ fd ];

		if( f == nullptr )
			return -1;
		log_info("文件fd: %d, offset: %d, whence: %d\n", fd, offset, whence);
		return f->lseek( offset, whence );
	}

	// uint64 SyscallHandler::_sys_splice()
	// {
	// 	int fd_in;
	// 	uint64 off_in_addr;
	// 	int fd_out;
	// 	uint64 off_out_addr;
	// 	[[maybe_unused]] int len;
	// 	[[maybe_unused]] int flags;

	// 	if( _arg_int( 0, fd_in ) < 0 )
	// 		return -1;
		
	// 	if( _arg_addr( 1, off_in_addr ) < 0 )
	// 		return -1;
		
	// 	if( _arg_int( 2, fd_out ) < 0 )
	// 		return -1;
		
	// 	if( _arg_addr( 3, off_out_addr ) < 0 )
	// 		return -1;

	// 	if( _arg_int( 4, len ) < 0 )
	// 		return -1;
		
	// 	if( _arg_int( 5, flags ) < 0 )
	// 		return -1;
		
	// 	// pm::Pcb *cur_proc = pm::k_pm.get_cur_pcb();
	// 	// mm::PageTable *pt = cur_proc->get_pagetable();
	// 	// [[maybe_unused]] int off_in = 0;
	// 	// [[maybe_unused]] int off_out = 0;

	// 	// if( off_in_addr != 0 )
	// 	// {
	// 	// 	if( mm::k_vmm.copy_in( *pt, &off_in, off_in_addr, sizeof( int ) ) < 0 )
	// 	// 		return -1; 
	// 	// }
		
	// 	// if( off_out_addr != 0 )
	// 	// {
	// 	// 	if( mm::k_vmm.copy_in( *pt, &off_out, off_out_addr, sizeof( int ) ) < 0 )
	// 	// 		return -1; 
	// 	// }

	// 	// if( off_in < 0 || off_out < 0 )
	// 	// 	return -1;
		
	// 	// /// @todo 处理 offin > fd_in.size	
	// 	// if( len == 0 ) // don't need to copy
	// 	// 	return len;
		
	// 	// char *buf = new char[ len ];
		 
	// 	// int ret = 0;

	// 	// if( fs::normal_file * normal_in = static_cast<fs::normal_file *>(cur_proc->_ofile[ fd_in ] ) )
	// 	// 	if( static_cast<uint64>(off_in) > normal_in->_stat.size )
	// 	// 		return 0;
	// 	// //[[maybe_unused]]int rdbytes = cur_proc->_ofile[ fd_in ]->read( (uint64) buf, len, off_in, false );
	// 	// cur_proc->_ofile[ fd_in ]->read( (uint64) buf, len, off_in, false );

	// 	// // if( rdbytes < len )
	// 	// // 	len = rdbytes;
			
	// 	// ret = cur_proc->_ofile[ fd_out ]->write( (uint64) buf, len, off_out, false );
		
	// 	// return ret;
	// pm::Pcb *cur_proc = pm::k_pm.get_cur_pcb();
	// mm::PageTable *pt = cur_proc->get_pagetable();

	// fs::file *file_in = cur_proc->_ofile[fd_in];
	// fs::file *file_out = cur_proc->_ofile[fd_out];
	// 	//FileTypes::FT_PIPE
	// bool in_is_pipe = (file_in->_attrs.filetype == fs::FileTypes::FT_PIPE);
	// bool out_is_pipe = (file_out->_attrs.filetype == fs::FileTypes::FT_PIPE);

	// // 必须一个是管道，一个是普通文件
	// if (!(in_is_pipe ^ out_is_pipe))
	// 	return -1;

	// int off_in = 0, off_out = 0;
	// if (!in_is_pipe) {
	// 	if (off_in_addr == 0) return -1;
	// 	if (mm::k_vmm.copy_in(*pt, &off_in, off_in_addr, sizeof(int)) < 0) return -1;
	// 	if (off_in < 0) return -1;
	// 	if (off_in >= file_in->size()) return 0;
	// } else {
	// 	if (off_in_addr != 0) return -1;
	// }
	// if (!out_is_pipe) {
	// 	if (off_out_addr == 0) return -1;
	// 	if (mm::k_vmm.copy_in(*pt, &off_out, off_out_addr, sizeof(int)) < 0) return -1;
	// 	if (off_out < 0) return -1;
	// 	if (off_out >= file_out->size()) return -1;
	// } else {
	// 	if (off_out_addr != 0) return -1;
	// }

	// // 计算实际可读写长度
	// int actual_len = len;
	// if (!in_is_pipe) {
	// 	int remain = file_in->size() - off_in;
	// 	if (remain < actual_len) actual_len = remain;
	// }
	// if (actual_len <= 0) return 0;

	// char *buf = new char[actual_len];
	// int read_bytes = 0, write_bytes = 0;

	// if (!in_is_pipe) {
	// 	read_bytes = file_in->read((uint64)buf, actual_len, off_in, false);
	// 	if (read_bytes <= 0) { delete[] buf; return read_bytes; }
		
	// 	// 写入管道，阻塞直到管道有空间
	// 	write_bytes = file_out->write((uint64)buf, read_bytes, 0, true); // true表示阻塞
		
	// 	// 更新off_in
	// 	off_in += write_bytes;
	// 	mm::k_vmm.copyout(*pt, off_in_addr, &off_in, sizeof(int));
	// } else {
	// 	// 读取管道，阻塞直到有数据
	// 	read_bytes = file_in->read((uint64)buf, actual_len, 0, true); // true表示阻塞
	// 	if (read_bytes <= 0) { delete[] buf; return read_bytes; }
		
	// 	// 写入文件
	// 	write_bytes = file_out->write((uint64)buf, read_bytes, off_out, false);
		
	// 	// 更新off_out
	// 	off_out += write_bytes;
	// 	mm::k_vmm.copyout(*pt, off_out_addr, &off_out, sizeof(int));
	// }

	// delete[] buf;
	// return write_bytes;
	// }

	uint64 SyscallHandler::_sys_splice()
	{
		//printf("sys_splice called\n");
		int fd_in;
		uint64 off_in_addr;
		int fd_out;
		uint64 off_out_addr;
		int len;
		int flags;

		// 获取参数
		if (_arg_int(0, fd_in) < 0)
			return -1;
		
		if (_arg_addr(1, off_in_addr) < 0)
			return -1;
		
		if (_arg_int(2, fd_out) < 0)
			return -1;
		
		if (_arg_addr(3, off_out_addr) < 0)
			return -1;

		if (_arg_int(4, len) < 0)
			return -1;
		
		if (_arg_int(5, flags) < 0)
			return -1;
		
		// 获取进程和文件描述符
		pm::Pcb *cur_proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = cur_proc->get_pagetable();
		
		fs::file *file_in = cur_proc->_ofile[fd_in];
		fs::file *file_out = cur_proc->_ofile[fd_out];
		
		if (!file_in || !file_out)
			return -1;

		// 判断文件类型
		bool in_is_pipe = (file_in->_attrs.filetype == fs::FileTypes::FT_PIPE);
		bool out_is_pipe = (file_out->_attrs.filetype == fs::FileTypes::FT_PIPE);
		
		// 必须一个是管道，一个是普通文件
		if (!(in_is_pipe ^ out_is_pipe))
			return -1;
			
		// 检查NULL/非NULL约束
		if (in_is_pipe && off_in_addr != 0)
			return -1;  // 管道的偏移必须为NULL
		if (!in_is_pipe && off_in_addr == 0)
			return -1;  // 普通文件的偏移必须非NULL
			
		if (out_is_pipe && off_out_addr != 0)
			return -1;  // 管道的偏移必须为NULL
		if (!out_is_pipe && off_out_addr == 0)
			return -1;  // 普通文件的偏移必须非NULL

		// 读取偏移量
		int off_in = 0, off_out = 0;
		
		if (!in_is_pipe) {
			if (mm::k_vmm.copy_in(*pt, &off_in, off_in_addr, sizeof(int)) < 0)
				return -1;
			if (off_in < 0)
				return -1;
				
			// 检查偏移是否超过文件大小
			int file_size = 0;
			if (file_in->_attrs.filetype == fs::FileTypes::FT_NORMAL && file_in->_stat.size > 0)
				file_size = file_in->_stat.size;
			else if (fs::normal_file *normal_in = static_cast<fs::normal_file *>(file_in))
				file_size = normal_in->_stat.size;
				
			if (off_in >= file_size)
				return 0;  // 偏移超过文件大小，直接返回0
		}
		
		if (!out_is_pipe) {
			if (mm::k_vmm.copy_in(*pt, &off_out, off_out_addr, sizeof(int)) < 0)
				return -1;
			if (off_out < 0)
				return -1;
		}

		// 计算实际可读写长度
		int actual_len = len;
		if (!in_is_pipe) {
			// 获取文件大小
			int file_size = 0;
			if (file_in->_attrs.filetype == fs::FileTypes::FT_NORMAL && file_in->_stat.size > 0)
				file_size = file_in->_stat.size;
			else if (fs::normal_file *normal_in = static_cast<fs::normal_file *>(file_in))
				file_size = normal_in->_stat.size;
				
			// 如果文件剩余部分小于len，调整实际读取长度
			int remain = file_size - off_in;
			if (remain < actual_len)
				actual_len = remain;
		}
		
		if (actual_len <= 0)
			return 0;

		// 分配缓冲区
		char *buf = new char[actual_len];
		if (!buf)
			return -1;
			
		int read_bytes = 0, write_bytes = 0;

		// 根据输入类型执行不同操作
		if (in_is_pipe) {
			// 从管道读取数据（阻塞等待）
			read_bytes = file_in->read((uint64)buf, actual_len, 0, true);
			if (read_bytes <= 0) {
				delete[] buf;
				return read_bytes;
			}
			
			// 写入普通文件
			write_bytes = file_out->write((uint64)buf, read_bytes, off_out, false);
			
			// 更新文件偏移
			if (write_bytes > 0) {
				off_out += write_bytes;
				mm::k_vmm.copyout(*pt, off_out_addr, &off_out, sizeof(int));
			}
		} else {
			// 从普通文件读取数据
			read_bytes = file_in->read((uint64)buf, actual_len, off_in, false);
			if (read_bytes <= 0) {
				delete[] buf;
				return read_bytes;
			}
			
			// 写入管道（阻塞等待）
			write_bytes = file_out->write((uint64)buf, read_bytes, 0, true);
			
			// 更新文件偏移
			if (write_bytes > 0) {
				off_in += write_bytes;
				mm::k_vmm.copyout(*pt, off_in_addr, &off_in, sizeof(int));
			}
		}

		// 释放缓冲区
		delete[] buf;
		
		// 返回实际复制的字节数
		return write_bytes;
	}

	uint64 SyscallHandler::_sys_sigprocmask()
	{
		int how;
		signal::sigset_t set;
		signal::sigset_t old_set;
		uint64 setaddr;
		uint64 oldsetaddr;
		int sigsize;

		if( _arg_int( 0, how ) < 0 )
			return -1;
		if( _arg_addr( 1, setaddr ) < 0 )
			return -1;
		if( _arg_addr( 2, oldsetaddr ) < 0 )
			return -1;
		if( _arg_int( 3, sigsize ) < 0 )
			return -1;
		
		pm::Pcb *cur_proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = cur_proc->get_pagetable();

		// 只有当setaddr不为0时才从用户空间读取新的信号掩码
		signal::sigset_t *newset_ptr = nullptr;
		if( setaddr != 0 ) {
			if( mm::k_vmm.copy_in( *pt, &set, setaddr, sizeof( signal::sigset_t) ) < 0 )
				return -1;
			newset_ptr = &set;
		}
		
		// 调用内核的sigprocmask函数
		int ans = signal::sigprocmask( how, newset_ptr, &old_set, sigsize );
		
		// 如果调用成功且oldsetaddr不为0，将旧的信号掩码写回用户空间
		if( ans >= 0 && oldsetaddr != 0 ) {
			if( mm::k_vmm.copyout( *pt, oldsetaddr, &old_set, sizeof( signal::sigset_t) ) < 0 )
				return -1;
		}
		
		return ans;
	}

	uint64 SyscallHandler::_sys_renameat2()
	{
		int		  flags;
		int		  old_fd, new_fd;
		fs::file *old_file, *new_file;
		uint64	  old_path_addr, new_path_addr;
		if ( _arg_int( 0, old_fd ) < 0 || _arg_addr( 1, old_path_addr ) < 0 ||
			 _arg_int( 2, new_fd ) < 0 || _arg_addr( 3, new_path_addr ) < 0 ||
			 _arg_int( 4, flags ) )
		{
			// 参数不对
			return -1;
		}
		eastl::string  path;
		pm::Pcb		  *p  = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = p->get_pagetable();
		if ( mm::k_vmm.copy_str_in( *pt, path, old_path_addr, 100 ) < 0 ) return -1;
		if ( path == "/proc/interrupts" ) return -1;
		return 0;
	}

	uint64 SyscallHandler::_sys_rt_sigtimedwait(){
		printf("系统调用rt_sigtimedwait\n");
		return 0;
	}
	/**
	 * @ Parmeters:
	 * @ uaddr: 指向用户空间中整型变量的指针，作为同步原语和等待队列的“键”使用。
	 * @ op: 操作码，低 8 位指定具体操作（如 FUTEX_WAIT、FUTEX_WAKE 等），高位为附加标志位（如 FUTEX_PRIVATE_FLAG）。
	 * @ val: 	对于 FUTEX_WAIT：期望 *uaddr == val 时才会阻塞；对于 FUTEX_WAKE：表示要唤醒的线程数量。
	 * @ timeout_addr: 	（可选）等待的超时时间，仅用于 FUTEX_WAIT 类型的操作。
	 * @ uaddr2: 	用于某些复合操作（如 FUTEX_REQUEUE），表示另一个同步地址。
	 * @ val3: 用于某些扩展操作（如 FUTEX_CMP_REQUEUE），作为额外比较或限制值。
	 */
	uint64 SyscallHandler::_sys_futex() {
		uint64 uaddr;
		int op;
		int val;
		uint64 timeout_addr;
		uint64 uaddr2;
		int val3;
		
		// 获取参数
		if (_arg_addr(0, uaddr) < 0) return -1;
		if (_arg_int(1, op) < 0) return -1;
		if (_arg_int(2, val) < 0) return -1;
		if (_arg_addr(3, timeout_addr) < 0) return -1;
		if (_arg_addr(4, uaddr2) < 0) return -1;
		if (_arg_int(5, val3) < 0) return -1;
		pm::Pcb *cur_proc = pm::k_pm.get_cur_pcb();
		mm::PageTable *pt = cur_proc->get_pagetable();
		int expected_val;
		int base_op = op & 0x7f; // 获取操作码的基本部分
		switch (base_op) {
			case FUTEX_WAIT:
				if( mm::k_vmm.copy_in(*pt, &expected_val, uaddr, sizeof(int)) < 0 )
					return -1;
				if(expected_val != val)
				{
					log_error("futex系统调用：FUTEX_WAIT失败，期望值不匹配\n");
					return -EAGAIN; // 期望值不匹配
				}else{
					log_info("futex系统调用：FUTEX_WAIT成功，阻塞进程%d\n", cur_proc->_pid);
					// 阻塞当前进程
					cur_proc->_lock.acquire();
					pm::k_pm.sleep((void*)uaddr,&cur_proc->_lock);
					return 0; // 成功阻塞
				}
			case FUTEX_PRIVATE_FLAG:
				printf("futex系统调用：FUTEX_PRIVATE_FLAG\n");
				return 0;
			default:
				log_error("futex系统调用：unkown opcode %d\n", op);
				break;
		}
		return 0;
	}

	uint64 SyscallHandler::_sys_socket()
	{
		// socket系统调用的伪实现 - 内核不支持网络功能
		// 参数: domain, type, protocol
		int domain;
		int type;
		int protocol;
		
		// 获取参数
		if( _arg_int( 0, domain ) < 0 )
			return -1;
		if( _arg_int( 1, type ) < 0 )
			return -1;
		if( _arg_int( 2, protocol ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("socket系统调用: domain=%d, type=%d, protocol=%d\n", domain, type, protocol);
		
		// 返回错误码 -EAFNOSUPPORT (地址族不支持)
		// 或者返回 -ENOSYS (功能未实现)
		return -97; // -EAFNOSUPPORT
	}

	uint64 SyscallHandler::_sys_bind()
	{
		// bind系统调用的伪实现 - 内核不支持网络功能
		// 参数: sockfd, addr, addrlen
		int sockfd;
		uint64 addr;
		int addrlen;
		
		// 获取参数
		if( _arg_int( 0, sockfd ) < 0 )
			return -1;
		if( _arg_addr( 1, addr ) < 0 )
			return -1;
		if( _arg_int( 2, addrlen ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("bind系统调用: sockfd=%d, addr=0x%lx, addrlen=%d\n", sockfd, addr, addrlen);
		
		// 返回错误码 -EBADF (无效的文件描述符)
		// 因为socket()已经返回错误，所以sockfd无效
		return -9; // -EBADF
	}

	uint64 SyscallHandler::_sys_listen()
	{
		// listen系统调用的伪实现 - 内核不支持网络功能
		// 参数: sockfd, backlog
		int sockfd;
		int backlog;
		
		// 获取参数
		if( _arg_int( 0, sockfd ) < 0 )
			return -1;
		if( _arg_int( 1, backlog ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("listen系统调用: sockfd=%d, backlog=%d\n", sockfd, backlog);
		
		// 返回错误码 -EBADF (无效的文件描述符)
		// 因为socket()已经返回错误，所以sockfd无效
		return -9; // -EBADF
	}

	uint64 SyscallHandler::_sys_accept()
	{
		// accept系统调用的伪实现 - 内核不支持网络功能
		// 参数: sockfd, addr, addrlen
		int sockfd;
		uint64 addr;
		uint64 addrlen_ptr;
		
		// 获取参数
		if( _arg_int( 0, sockfd ) < 0 )
			return -1;
		if( _arg_addr( 1, addr ) < 0 )
			return -1;
		if( _arg_addr( 2, addrlen_ptr ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("accept系统调用: sockfd=%d, addr=0x%lx, addrlen_ptr=0x%lx\n", sockfd, addr, addrlen_ptr);
		
		// 返回错误码 -EBADF (无效的文件描述符)
		// 因为socket()已经返回错误，所以sockfd无效
		return -9; // -EBADF
	}

	uint64 SyscallHandler::_sys_connect()
	{
		// connect系统调用的伪实现 - 内核不支持网络功能
		// 参数: sockfd, addr, addrlen
		int sockfd;
		uint64 addr;
		int addrlen;
		
		// 获取参数
		if( _arg_int( 0, sockfd ) < 0 )
			return -1;
		if( _arg_addr( 1, addr ) < 0 )
			return -1;
		if( _arg_int( 2, addrlen ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("connect系统调用: sockfd=%d, addr=0x%lx, addrlen=%d\n", sockfd, addr, addrlen);
		
		// 返回错误码 -EBADF (无效的文件描述符)
		// 因为socket()已经返回错误，所以sockfd无效
		return -9; // -EBADF
	}

	uint64 SyscallHandler::_sys_getsockname()
	{
		// getsockname系统调用的伪实现 - 内核不支持网络功能
		// 参数: sockfd, addr, addrlen
		int sockfd;
		uint64 addr;
		uint64 addrlen_ptr;
		
		// 获取参数
		if( _arg_int( 0, sockfd ) < 0 )
			return -1;
		if( _arg_addr( 1, addr ) < 0 )
			return -1;
		if( _arg_addr( 2, addrlen_ptr ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("getsockname系统调用: sockfd=%d, addr=0x%lx, addrlen_ptr=0x%lx\n", sockfd, addr, addrlen_ptr);
		
		// 返回错误码 -EBADF (无效的文件描述符)
		// 因为socket()已经返回错误，所以sockfd无效
		return -9; // -EBADF
	}

	uint64 SyscallHandler::_sys_sendto()
	{
		// sendto系统调用的伪实现 - 内核不支持网络功能
		// 参数: sockfd, buf, len, flags, dest_addr, addrlen
		int sockfd;
		uint64 buf;
		int len;
		int flags;
		uint64 dest_addr;
		int addrlen;
		
		// 获取参数
		if( _arg_int( 0, sockfd ) < 0 )
			return -1;
		if( _arg_addr( 1, buf ) < 0 )
			return -1;
		if( _arg_int( 2, len ) < 0 )
			return -1;
		if( _arg_int( 3, flags ) < 0 )
			return -1;
		if( _arg_addr( 4, dest_addr ) < 0 )
			return -1;
		if( _arg_int( 5, addrlen ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("sendto系统调用: sockfd=%d, buf=0x%lx, len=%d, flags=%d, dest_addr=0x%lx, addrlen=%d\n", 
			   sockfd, buf, len, flags, dest_addr, addrlen);
		
		// 返回错误码 -EBADF (无效的文件描述符)
		// 因为socket()已经返回错误，所以sockfd无效
		return -9; // -EBADF
	}

	uint64 SyscallHandler::_sys_recvfrom()
	{
		// recvfrom系统调用的伪实现 - 内核不支持网络功能
		// 参数: sockfd, buf, len, flags, src_addr, addrlen_ptr
		int sockfd;
		uint64 buf;
		int len;
		int flags;
		uint64 src_addr;
		uint64 addrlen_ptr;
		
		// 获取参数
		if( _arg_int( 0, sockfd ) < 0 )
			return -1;
		if( _arg_addr( 1, buf ) < 0 )
			return -1;
		if( _arg_int( 2, len ) < 0 )
			return -1;
		if( _arg_int( 3, flags ) < 0 )
			return -1;
		if( _arg_addr( 4, src_addr ) < 0 )
			return -1;
		if( _arg_addr( 5, addrlen_ptr ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("recvfrom系统调用: sockfd=%d, buf=0x%lx, len=%d, flags=%d, src_addr=0x%lx, addrlen_ptr=0x%lx\n", 
			   sockfd, buf, len, flags, src_addr, addrlen_ptr);
		
		// 返回错误码 -EBADF (无效的文件描述符)
		// 因为socket()已经返回错误，所以sockfd无效
		return -9; // -EBADF
	}

	uint64 SyscallHandler::_sys_setsockopt()
	{
		// setsockopt系统调用的伪实现 - 内核不支持网络功能
		// 参数: sockfd, level, optname, optval, optlen
		int sockfd;
		int level;
		int optname;
		uint64 optval;
		int optlen;
		
		// 获取参数
		if( _arg_int( 0, sockfd ) < 0 )
			return -1;
		if( _arg_int( 1, level ) < 0 )
			return -1;
		if( _arg_int( 2, optname ) < 0 )
			return -1;
		if( _arg_addr( 3, optval ) < 0 )
			return -1;
		if( _arg_int( 4, optlen ) < 0 )
			return -1;
		
		// 打印调试信息
		printf("setsockopt系统调用: sockfd=%d, level=%d, optname=%d, optval=0x%lx, optlen=%d\n", 
			   sockfd, level, optname, optval, optlen);
		
		// 返回错误码 -EBADF (无效的文件描述符)
		// 因为socket()已经返回错误，所以sockfd无效
		return -9; // -EBADF
	}
} // namespace syscall
