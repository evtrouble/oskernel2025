//
// Created by Li shuang ( pseudonym ) on 2024-03-29
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use
// | Contact Author: lishuang.mk@whu.edu.cn
// --------------------------------------------------------------
//

#pragma once

#include "mm/page_table.hh"
// #include "pm/context.hh"
#include <EASTL/string.h>

#include <process_interface.hh>
#include <smp/spin_lock.hh>

#include "pm/ipc/signal.hh"
#include "pm/prlimit.hh"
#include "pm/sharemem.hh"

namespace fs
{
	class dentry;
	class file;
} // namespace fs
namespace pm
{
	struct TrapFrame;

	constexpr uint num_process		 = 32;
	constexpr int  default_proc_prio = 10;
	constexpr int  lowest_proc_prio	 = 19;
	constexpr int  highest_proc_prio = 0;
	constexpr uint max_open_files	 = 512;

	constexpr int default_proc_kstack_pages = 31; // 默认进程内核栈大小（按页面数计算）
	constexpr int default_proc_ustack_pages = 32; // 默认进程用户栈大小（按页面数计算）

	enum ProcState
	{
		unused,
		used,
		sleeping,
		runnable,
		running,
		zombie
	};

	class ProcessManager;
	class ShmManager;
	class Scheduler;

	struct robust_list_head;

	struct program_section_desc
	{
		void	   *_sec_start	= nullptr; // virtual address
		ulong		_sec_size	= 0;
		const char *_debug_name = nullptr;
	};
	constexpr int max_program_section_num = 16;

	class Pcb
	{
		friend ProcessManager;
		friend ShmManager;
		friend Scheduler;

	public:

		hsai::SpinLock _lock;
		int			   _gid = num_process; // global ID in pool

		program_section_desc _prog_sections[max_program_section_num];
		int					 _prog_section_cnt = 0;

		fs::dentry	  *_cwd; // current working directory
		eastl::string  _cwd_name;
		eastl::string  exe; // absolute path of the executable file
		// p->lock must be held when using these:
		enum ProcState _state;	// Process state
		void		  *_chan;	// If non-zero, sleeping on chan
		int			   _killed; // If non-zero, have been killed
		int			   _xstate; // Exit status to be returned to parent's wait
		int			   _pid;	// Process ID

		// wait_lock must be held when using this:
		Pcb *parent; // Parent process

		// these are private to the process, so p->lock need not be held.
		uint64		  _kstack = 0; // Virtual address of kernel stack
		uint64		  _sz	  = 0; // 进程在内存中占用的大小（包括栈，堆，数据和代码）
		mm::PageTable _pt;		   // User lower half address page table
		mm::PageTable _kpt;		   
		TrapFrame	 *_trapframe;  // data page for uservec.S, use DMW address
		void		 *_context;	   // swtch() here to run process
		fs::file	 *_ofile[max_open_files]; // Open files
		// struct inode *cwd;           // Current directory
		char		  _name[256]; // Process name (debugging)

		int _slot;	   // Rest of time slot
		int _priority; // Process priority (0-20)=(highest-lowest)

		// about share memory
		ulong _shm;			   // The low-boundary address of share memory
		void *_shmva[SHM_NUM]; // The sharemem of process
		uint  _shmkeymask;	   // The mask of using shared physical memory page

		// about msg queue
		uint _mqmask;

		uint64 _start_tick;		// the tick value when the process starts running
		uint64 _user_ticks;		// the ticks of that the process has run in user mode
		uint64 _last_user_tick; // the tick value of that the process returns to user mode last time

		uint64 _heap_start = 0; // 堆起始地址，为程序数据段结束后向上对齐页地址
		uint64 _heap_ptr   = 0; // 堆结束指针

		// vm
		struct vma *vm[10]; // virtual memory area <<<<<<<<<<<<<<<<<< what??? Could ONE process has several vm space?


		// 线程/futex 相关

		int *_set_child_tid	  = nullptr;
		int *_clear_child_tid = nullptr;

		robust_list_head *_robust_list = nullptr;

		// for prlimit 进程资源相关
		rlimit64					_rlim_vec[ResourceLimitId::RLIM_NLIMITS];
		
		pm::ipc::signal::sigaction *_sigactions[SIGRTMAX];
		uint64 sigmask;

	public:

		Pcb();
		void init(const char *lock_name, uint gid);
		void map_kstack(mm::PageTable &pt);

		int get_priority();

	public:

		void *get_context() { return ( void * )_context; }
		// smp::Lock &get_lock() { return _lock; }		<<<<<<<<<<<<<<<<<<
		// 注意任何时候都不要尝试把任何的类的私有lock返回出去， lock不正当的使用会带来问题，
		// 外部需要申请这个类的资源时应当在类中实现一个返回资源的接口,
		// 而lock的使用应当在接口中由类内部来决定

	public:

		// fs::Dentry *get_cwd() { return _cwd; }
		fs::dentry	  *get_cwd() { return _cwd; }
		void		   kill() { _killed = 1; }
		Pcb			  *get_parent() { return parent; }
		void		   set_state(ProcState state) { _state = state; }
		void		   set_xstate(int xstate) { _xstate = xstate; }
		size_t		   get_sz() { return _sz; }
		// void set_chan(void *chan) { _chan = chan; }
		uint		   get_pid() { return _pid; }
		uint		   get_ppid() { return parent ? parent->_pid : 0; }
		TrapFrame	  *get_trapframe() { return _trapframe; }
		uint64		   get_kstack() { return _kstack; }
		mm::PageTable *get_pagetable() { return &_pt; }
		mm::PageTable *get_kpagetable() { return &_kpt; }
		ProcState	   get_state() { return _state; }
		char		  *get_name() { return _name; }
		uint64		   get_size() { return _sz; }
		uint64		   get_last_user_tick() { return _last_user_tick; }
		uint64		   get_user_ticks() { return _user_ticks; }
		fs::file	  *get_open_file(int fd)
		{
			if ( fd < 0 || fd >= ( int )max_open_files ) return nullptr;
			return _ofile[fd];
		}

		void set_trapframe(TrapFrame *tf) { _trapframe = tf; }

		void set_last_user_tick(uint64 tick) { _last_user_tick = tick; }
		void set_user_ticks(uint64 ticks) { _user_ticks = ticks; }

		bool is_killed() { return _killed != 0; }
		int get_max_rss()
		{
			return _rlim_vec[RLIMIT_RSS].rlim_cur;
		}
	};

	extern Pcb k_proc_pool[num_process];
} // namespace pm
