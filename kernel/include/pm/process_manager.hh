//
// Created by Li shuang ( pseudonym ) on 2024-03-28 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#pragma once 
#include "pm/process.hh"

#include <smp/spin_lock.hh>

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace fs
{
	struct Kstat;

	namespace fat
	{
		class Fat32Dentry;
	}
}

namespace tmm
{
	struct tms;
} // namespace tmm


namespace pm
{
	constexpr int default_proc_slot = 4;

	struct robust_list_head;
	struct rlimit64;

#define MAXARG 32
	class ProcessManager
	{
	private:
		hsai::SpinLock _pid_lock;
		hsai::SpinLock _wait_lock;
		int _cur_pid;
		Pcb * _init_proc;		// user init proc

		uint _last_alloc_proc_gid;

	public:
		ProcessManager() = default;
		void init( const char *pid_lock_name, const char *wait_lock_name );

		Pcb *get_cur_pcb();
		bool change_state( Pcb *p, ProcState state );
		void alloc_pid( Pcb *p );
		Pcb *alloc_proc();

		void set_slot( Pcb *p, int slot );
		void set_priority( Pcb *p, int priority );
		void set_shm( Pcb *p );
		// void set_vma( Pcb *p );
		int set_trapframe( Pcb *p );

		mm::PageTable proc_pagetable( Pcb *p );
		void proc_freepagetable( mm::PageTable pt, uint64 sz );
		void freeproc( Pcb *p );
		void sche_proc( Pcb *p );

		int execve( eastl::string path, eastl::vector<eastl::string> args, eastl::vector<eastl::string> envs );
		int wait( int child_pid, uint64 addr );
		int load_seg( mm::PageTable &pt, uint64 va, fs::dentry *de, uint offset, uint size );

		void sleep( void *chan, hsai::SpinLock *lock );
		void wakeup( void *chan );
		void exit_proc( Pcb * p, int state );
		void exit( int state );
		int fork( uint64 usp );
		int fork();
		void fork_ret();
		long brk( long n );
		int open( int dir_fd, eastl::string path, uint flags );
		int mkdir( int dir_fd, eastl::string path, uint flags );
		int close( int fd );
		int fstat( int fd, struct fs::Kstat *buf );
		int chdir( eastl::string &path );
		int getcwd( char * out_buf );
		int mmap( int fd, int map_size );
		int unlink( int fd, eastl::string path, int flags );
		int pipe( int *fd, int );
		int set_tid_address( int * tidptr );
		int set_robust_list( robust_list_head * head, size_t len );
		int prlimit64( int pid, int resource, rlimit64 * new_limit, rlimit64 * old_limit );
		void exit_group( int status );
		void reparent( Pcb *p );

		void user_init();

		int alloc_fd( Pcb * p, fs::file * f );
		int alloc_fd( Pcb * p, fs::file * f, int fd );

		void get_cur_proc_tms( tmm::tms * tsv );

	public:
		void kill_proc( Pcb * p ) { p->_killed = 1; }

	private:
		void _proc_create_vm( Pcb *p, mm::PageTable &pt );
		void _proc_create_vm( Pcb *p );

	public: // ================ test functions ================
		void vectortest();
		void stringtest();
		void maptest();
		void hashtest();
	};

	extern ProcessManager k_pm;
}