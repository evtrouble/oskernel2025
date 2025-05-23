//
// Created by Li shuang ( pseudonym ) on 2024-03-29
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use
// | Contact Author: lishuang.mk@whu.edu.cn
// --------------------------------------------------------------
//

#include "pm/process_manager.hh"

#include "fs/elf.hh"
#include "mm/memlayout.hh"
#include "mm/physical_memory_manager.hh"
#include "mm/userstack_stream.hh"
#include "mm/virtual_memory_manager.hh"
#include "pm/futex.hh"
#include "pm/ipc/pipe.hh"
#include "pm/prlimit.hh"
#include "pm/process.hh"
#include "pm/scheduler.hh"
#include "tm/timer_manager.hh"
#include "klib/klib.hh"
// #include "fs/fat/fat32_dir_entry.hh"
// #include "fs/fat/fat32Dentry.hh"
// #include "fs/fat/fat32_file_system.hh"
#include "fs/fat/fat32fs.hh"
#include "fs/file/file.hh"
// #include "fs/dev/console.hh"
#include <EASTL/hash_map.h>
#include <EASTL/map.h>
#include <EASTL/stack.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <device_manager.hh>
#include <hsai_global.hh>
#include <mem/virtual_memory.hh>
#include <process_interface.hh>
#include <virtual_cpu.hh>

#include "fs/file/device.hh"
#include "fs/file/directory.hh"
#include "fs/file/file.hh"
#include "fs/file/normal.hh"
#include "fs/file/pipe.hh"
#include "fs/fs_defs.hh"
#include "fs/kstat.hh"
#include "fs/path.hh"
#include "fs/ramfs/ramfs.hh"
#include "klib/common.hh"

extern "C" {
extern uint64 _start_u_init;
extern uint64 _end_u_init;
extern uint64 _u_init_stks;
extern uint64 _u_init_stke;
extern uint64 _u_init_txts;
extern uint64 _u_init_txte;
extern uint64 _u_init_dats;
extern uint64 _u_init_date;
extern int	  init_main( void );

void _wrp_fork_ret( void ) { pm::k_pm.fork_ret(); }
}


namespace pm
{
	ProcessManager k_pm;

	void ProcessManager::init( const char *pid_lock_name, const char *wait_lock_name )
	{
		_pid_lock.init( pid_lock_name );
		_wait_lock.init( wait_lock_name );
		for ( uint i = 0; i < num_process; ++i )
		{
			Pcb &p = k_proc_pool[i];
			new ( &p ) Pcb();
			p.init( "pcb", i );
		}
		_cur_pid			 = 1;
		_last_alloc_proc_gid = num_process - 1;
	}

	Pcb *ProcessManager::get_cur_pcb()
	{
		hsai::VirtualCpu *cpu = hsai::get_cpu();
		pm::Pcb			 *pcb = cpu->get_cur_proc();
		return pcb;
	}

	void ProcessManager::alloc_pid( Pcb *p )
	{
		_pid_lock.acquire();
		p->_pid = _cur_pid;
		_cur_pid++;
		_pid_lock.release();
	}

	Pcb *ProcessManager::alloc_proc()
	{
		Pcb *p;
		for ( uint i = 0; i < num_process; i++ )
		{
			p = &k_proc_pool[( _last_alloc_proc_gid + i ) % num_process];
			p->_lock.acquire();
			if ( p->_state == ProcState::unused )
			{
				pm::k_pm.alloc_pid( p );
				p->_state	 = ProcState::used;
				p->_slot	 = default_proc_slot;
				p->_priority = default_proc_prio;

				// p->_shm = mm::vml::vm_trap_frame - 64 * 2 * hsai::page_size;
				// p->_shmkeymask = 0;
				// pm::k_pm.set_vma( p );

				if ( ( p->_trapframe = (TrapFrame *) mm::k_pmm.alloc_page() ) == nullptr )
				{
					freeproc( p );
					p->_lock.release();
					return nullptr;
				}

				_proc_create_vm( p );
				if ( p->_pt.get_base() == 0 )
				{
					freeproc( p );
					p->_lock.release();
					return nullptr;
				}

				p->_mqmask = 0;

				memset( p->_context, 0, hsai::context_size );

				hsai::set_context_entry( p->_context, (void *) _wrp_fork_ret );

				hsai::set_context_sp( p->_context,
									  p->_kstack + hsai::page_size * default_proc_kstack_pages );

				p->_lock.release();

				_last_alloc_proc_gid = p->_gid;

				return p;
			}
			else { p->_lock.release(); }
		}
		return nullptr;
	}


	void ProcessManager::set_shm( Pcb *p )
	{
		k_pm._pid_lock.acquire();
		p->_lock.acquire();
		p->_shm		   = mm::vml::vm_trap_frame - 64 * 2 * hsai::page_size;
		p->_shmkeymask = 0;
		p->_lock.release();
		k_pm._pid_lock.release();
	}

	// void ProcessManager::set_vma( Pcb * p )
	// {
	// 	for ( int i = 1; i < 10; i++ )
	// 	{
	// 		p->vm[ i ]->next = -1;
	// 		p->vm[ i ]->length = 0;
	// 	}
	// 	p->vm[ 0 ]->next = 1;
	// }

	void ProcessManager::freeproc( Pcb *p )
	{
		mm::k_vmm.vm_unmap( p->_pt, mm::vml::vm_trap_frame, 1, 1 );
		p->_trapframe = 0;
		if ( !p->_pt.is_null() )
		{
			ulong sec_start, sec_size;
			for ( int i = 0; i < p->_prog_section_cnt; ++i )
			{
				auto &sec = p->_prog_sections[i];
				sec_start = hsai::page_round_down( (ulong) sec._sec_start );
				sec_size =
					hsai::page_round_up( (ulong) sec._sec_start + sec._sec_size - sec_start );
				mm::k_vmm.vm_unmap( p->_pt, sec_start, sec_size / hsai::page_size, 1 );
			}

			ulong stack_page_cnt = default_proc_ustack_pages;
			ulong stackbase		 = mm::vm_ustack_end - stack_page_cnt * hsai::page_size;
			mm::k_vmm.vm_unmap( p->_pt, stackbase - hsai::page_size, stack_page_cnt + 1, 1 );

			ulong heapbase = p->_heap_start;
			ulong heapsize = hsai::page_round_up( p->_heap_ptr - p->_heap_start );
			mm::k_vmm.vm_unmap( p->_pt, heapbase, heapsize / hsai::page_size, 1 );
			hsai::proc_free( p );
			p->_pt.freewalk();
		}

		if ( !p->_kpt.is_null() )
		{
			ulong stack_page_cnt = default_proc_ustack_pages;
			ulong stackbase		 = mm::vm_ustack_end - stack_page_cnt * hsai::page_size;
			mm::k_vmm.vm_unmap( p->_kpt, stackbase - hsai::page_size, stack_page_cnt + 1, 0 );
			p->_kpt.kfreewalk( stackbase - hsai::page_size );
		}

		p->_pt.set_base( 0 );
		p->_kpt.set_base( 0 );
		p->_prog_section_cnt = 0;
		p->_sz				 = 0;
		p->_heap_ptr		 = 0;
		p->_pid				 = 0;
		p->parent			 = 0;
		p->_chan			 = 0;
		p->_name[0]			 = 0;
		p->_killed			 = 0;
		p->_xstate			 = 0;
		p->_state			 = ProcState::unused;
		if ( p->_ofile[1]->refcnt > 1 ) p->_ofile[1]->refcnt--;
		for ( int i = 3; i < (int) max_open_files; ++i )
		{
			if ( p->_ofile[i] != nullptr && p->_ofile[i]->refcnt > 0 )
			{
				p->_ofile[i]->free_file();
				p->_ofile[i] = nullptr;
			}
		}
	}


	void ProcessManager::user_init()
	{
		static int	inited				  = 0;
		static char user_init_proc_name[] = "user init";
		if ( inited != 0 )
		{
			log_warn( "re-init user." );
			return;
		}

		Pcb *p = alloc_proc();
		assert( p != nullptr, "pm: alloc proc fail while user init." );

		_init_proc = p;
		p->_lock.acquire();

		for ( uint i = 0; i < sizeof( user_init_proc_name ); ++i )
		{
			p->_name[i] = user_init_proc_name[i];
		}

		// p->_priority = 19;

		p->_sz = (uint64) &_end_u_init - (uint64) &_start_u_init;
		// p->_hp = p->_sz;
		// p->_sz = 0;

		// map user init stack

		int	  stack_page_cnt = default_proc_ustack_pages;
		ulong stackbase		 = mm::vml::vm_ustack_end - stack_page_cnt * hsai::page_size;
		ulong sp			 = mm::vml::vm_ustack_end;

		if ( mm::k_vmm.vm_alloc( p->_pt, stackbase - hsai::page_size, sp ) == 0 )
		{
			log_panic( "user-init: vmalloc when allocating stack" );
			return;
		}

		ulong phy_stackbase = p->_pt.walk_addr( stackbase );
		log_trace( "user-init set stack-base = %p", phy_stackbase);
		log_trace( "user-init set page containing sp is %p",
				   p->_pt.walk_addr( sp - hsai::page_size ) );

		if ( mm::k_vmm.vm_set_super( p->_pt, stackbase - hsai::page_size, 1 ) < 0 )
			log_panic( "user-init: set stack protector fail" );

		mm::k_vmm.vm_set( p->_pt, (void *) stackbase, 0, stack_page_cnt );

		hsai::set_trap_frame_user_sp( p->_trapframe, sp );

		// mm::k_vmm.map_data_pages(
		// 	p->_pt,
		// 	0,
		// 	( uint64 ) &_u_init_stke - ( uint64 ) &_u_init_stks,
		// 	( uint64 ) &_u_init_stks,
		// 	true
		// );

		{
			int ps_cnt = p->_prog_section_cnt;

			// map user init code
			mm::k_vmm.map_code_pages( p->_pt, (uint64) &_u_init_txts - (uint64) &_start_u_init,
									  (uint64) &_u_init_txte - (uint64) &_u_init_txts,
									  (uint64) &_u_init_txts, true );
			p->_prog_sections[ps_cnt]._sec_start = (void *) hsai::page_round_down(
				( (uint64) &_u_init_txts - (uint64) &_start_u_init ) );
			p->_prog_sections[ps_cnt]._sec_size =
				hsai::page_round_up( (uint64) &_u_init_txte - (uint64) &_u_init_txts );
			p->_prog_sections[ps_cnt]._debug_name = "user-init code";
			ps_cnt++;

			// map user init data
			mm::k_vmm.map_data_pages( p->_pt, (uint64) &_u_init_dats - (uint64) &_start_u_init,
									  (uint64) &_u_init_date - (uint64) &_u_init_dats,
									  (uint64) &_u_init_dats, true );
			p->_prog_sections[ps_cnt]._sec_start = (void *) hsai::page_round_down(
				( (uint64) &_u_init_dats - (uint64) &_start_u_init ) );
			p->_prog_sections[ps_cnt]._sec_size =
				hsai::page_round_up( (uint64) &_u_init_date - (uint64) &_u_init_dats );
			p->_prog_sections[ps_cnt]._debug_name = "user-init data";
			ps_cnt++;

			p->_prog_section_cnt = ps_cnt;

			// set heap ptr
			p->_heap_ptr = p->_heap_start =
				hsai::page_round_up( (ulong) p->_prog_sections[ps_cnt - 1]._sec_start +
									 p->_prog_sections[ps_cnt - 1]._sec_size );
		}

#ifndef LOONGARCH
		mm::k_vmm.map_data_pages( p->_kpt, stackbase, sp - stackbase, 
									  phy_stackbase, false );
#endif

		// p->_trapframe->era = ( uint64 ) &init_main - ( uint64 )
		// &_start_u_init; log_info( "user init: era = %p", p->_trapframe->era
		// ); p->_trapframe->sp = ( uint64 ) &_u_init_stke - ( uint64 )
		// &_start_u_init; log_info( "user init: sp  = %p", p->_trapframe->sp );
		hsai::user_proc_init( (void *) p );

		// fs::File *f = fs::k_file_table.alloc_file();
		fs::Path		 path( "/dev/stdin" );
		fs::FileAttrs	 fAttrsin = fs::FileAttrs( fs::FileTypes::FT_DEVICE, 0444 ); // only read
		fs::device_file *f_in = new fs::device_file( fAttrsin, DEV_STDIN_NUM, path.pathSearch() );
		assert( f_in != nullptr, "pm: alloc stdin file fail while user init." );

		fs::Path		 pathout( "/dev/stdout" );
		fs::FileAttrs	 fAttrsout = fs::FileAttrs( fs::FileTypes::FT_DEVICE, 0222 ); // only write
		fs::device_file *f_out =
			new fs::device_file( fAttrsout, DEV_STDOUT_NUM, pathout.pathSearch() );
		assert( f_out != nullptr, "pm: alloc stdout file fail while user init." );

		fs::Path		 patherr( "/dev/stderr" );
		fs::FileAttrs	 fAttrserr = fs::FileAttrs( fs::FileTypes::FT_DEVICE, 0222 ); // only write
		fs::device_file *f_err =
			new fs::device_file( fAttrserr, DEV_STDERR_NUM, patherr.pathSearch() );
		assert( f_err != nullptr, "pm: alloc stderr file fail while user init." );

		// new ( &f->ops ) fs::FileOps( 3 );
		// f->major = DEV_STDOUT_NUM;

		// f->type = fs::FileTypes::FT_DEVICE;
		p->_ofile[0] = f_in;
		p->_ofile[1] = f_out;
		p->_ofile[2] = f_err;
		// p->_cwd = fs::fat::k_fatfs.get_root();
		/// @todo 这里暂时修改进程的工作目录为fat的挂载点
		p->_cwd		 = fs::ramfs::k_ramfs.getRoot()->EntrySearch( "mnt" );
		p->_cwd_name = "/mnt/";


		/// TODO:
		/// set p->cwd = "/"

		p->_state = ProcState::runnable;

		p->_start_tick = tmm::k_tm.get_ticks();
		p->_user_ticks = 0;

		p->_lock.release();

		inited = 1;

		hsai::VirtualCpu *cpu = hsai::get_cpu();
		cpu->set_cur_proc( p );
	}

	void ProcessManager::sche_proc( Pcb *p )
	{
		p->_slot--;
		if ( p->_slot == 0 )
		{
			p->_slot = default_proc_slot;
			k_scheduler.yield();
		}
	}

	int ProcessManager::fork() { return fork( 0 ); }

	int ProcessManager::fork( uint64 usp )
	{
		int	 i, pid;
		Pcb *np;				// new proc
		Pcb *p = get_cur_pcb(); // current proc

		// Allocate process.
		if ( ( np = alloc_proc() ) == nullptr ) { return -1; }

		np->_lock.acquire();

		// Copy user memory from parent to child.

		mm::PageTable *curpt, *newpt;
		curpt = p->get_pagetable();
		newpt = np->get_pagetable();

		// vm copy : 1. 拷贝LOAD程序段

		{
			ulong sec_start;
			ulong sec_size;
			for ( int j = 0; j < p->_prog_section_cnt; j++ )
			{
				auto &pd  = p->_prog_sections[j];
				sec_start = hsai::page_round_down( (ulong) pd._sec_start );
				sec_size  = hsai::page_round_up( (ulong) pd._sec_start + pd._sec_size ) - sec_start;
				if ( mm::k_vmm.vm_copy( *curpt, *newpt, sec_start, sec_size ) < 0 )
				{
					freeproc( np );
					np->_lock.release();
					return -1;
				}
				np->_prog_sections[j] = pd;
				np->_prog_section_cnt++;
			}
		}

		// vm copy : 2. 拷贝堆内存

		{
			ulong heap_start = p->_heap_start;
			ulong heap_size	 = hsai::page_round_up( p->_heap_ptr - heap_start );
			if ( mm::k_vmm.vm_copy( *curpt, *newpt, heap_start, heap_size ) < 0 )
			{
				freeproc( np );
				np->_lock.release();
				return -2;
			}
		}

		// vm copy : 3. 拷贝用户栈

		{
			// 多出的一页是保护页面，防止栈溢出
			ulong stack_start =
				mm::vml::vm_ustack_end - ( 1 + default_proc_ustack_pages ) * hsai::page_size;
#ifdef LOONGARCH
			if ( mm::k_vmm.vm_copy( *curpt, *newpt, stack_start,
									( 1 + default_proc_ustack_pages ) * hsai::page_size ) < 0 )
#else 
			if ( mm::k_vmm.vm_copy( *curpt, *newpt, *np->get_kpagetable(), stack_start, 
									( 1 + default_proc_ustack_pages ) * hsai::page_size ) < 0 )	
#endif
			{
				freeproc( np );
				np->_lock.release();
				return -3;
			}
		}

		hsai::proc_init( (void *) np );

		np->_sz			= p->_sz;
		np->_heap_start = p->_heap_start;
		np->_heap_ptr	= p->_heap_ptr;

		/// TODO: >> Share Memory Copy
		// shmaddcount( p->shmkeymask );
		// np->shm = p->shm;
		// np->shmkeymask = p->shmkeymask;
		// for ( i = 0; i < MAX_SHM_NUM; ++i )
		// {
		// 	if ( shmkeyused( i, np->shmkeymask ) )
		// 	{
		// 		np->shmva[ i ] = p->shmva[ i ];
		// 	}
		// }

		// copy saved user registers.
		hsai::copy_trap_frame( p->get_trapframe(), np->get_trapframe() );

		// Cause fork to return 0 in the child.
		hsai::set_trap_frame_return_value( np->get_trapframe(), 0 );

		if ( usp != 0 ) hsai::set_trap_frame_user_sp( np->get_trapframe(), usp );

		/// TODO: >> Message Queue Copy
		// addmqcount( p->mqmask );
		// np->mqmask = p->mqmask;

		// increment reference counts on open file descriptors.
		for ( i = 0; i < (int) max_open_files; i++ )
			if ( p->_ofile[i] )
			{
				// fs::k_file_table.dup( p->_ofile[ i ] );
				p->_ofile[i]->dup();
				np->_ofile[i] = p->_ofile[i];
			}
		np->_cwd	  = p->_cwd;
		np->_cwd_name = p->_cwd_name;

		/// TODO: >> cwd inode ref-up
		// np->cwd = idup( p->cwd );

		strncpy( np->_name, p->_name, sizeof( p->_name ) );

		pid = np->_pid;

		np->_lock.release();

		_wait_lock.acquire();
		np->parent = p;
		_wait_lock.release();

		np->_lock.acquire();
		np->_state		= ProcState::runnable;
		np->_start_tick = tmm::k_tm.get_ticks();
		np->_user_ticks = 0;
		np->_lock.release();

		return pid;
	}

	void ProcessManager::fork_ret()
	{
		// Still holding p->lock from scheduler.
		pm::Pcb *proc = get_cur_pcb();
		proc->_lock.release();

		hsai::user_trap_return();
	}


	mm::PageTable ProcessManager::proc_pagetable( Pcb *p )
	{
		mm::PageTable pt;

		uint64 pa = (uint64) mm::k_pmm.alloc_page();
		if ( pa == 0 ) return pt;
		pt.set_base( pa );

		// if(!mm::k_vmm.map_pages(pt, mm::vm_trap_frame, hsai::page_size,
		// (uint64)p->_trapframe, 	( loongarch::PteEnum::presence_m ) | 	(
		// loongarch::PteEnum::writable_m ) | 	( loongarch::PteEnum::plv_m ) |
		// ( loongarch::PteEnum::mat_m ) | 	( loongarch::PteEnum::dirty_m )))
		// {
		// 	mm::k_vmm.vmfree(pt, hsai::page_size);
		// 	return pt;
		// }
		return pt;
	}

	void ProcessManager::proc_freepagetable( mm::PageTable pt, uint64 sz )
	{
		// mm::k_vmm.vmunmap( pt, hsai::page_size, 1, 0 );
		// mm::k_vmm.vmfree( pt, sz );
		pt.freewalk_mapped();
	}

	static int _free_pt_with_sec( mm::PageTable &pt, program_section_desc *dsc_v, int dsc_c )
	{
		using psd = program_section_desc;
		if ( dsc_c > 0 )
		{
			for ( int i = 0; i < dsc_c; ++i )
			{
				psd	 &dsc = dsc_v[i];
				ulong va  = hsai::page_round_down( (ulong) dsc._sec_start );
				ulong npg = hsai::page_round_up( (ulong) dsc._sec_start + dsc._sec_size );
				npg		  = ( npg - va ) / hsai::page_size;
				mm::k_vmm.vm_unmap( pt, va, npg, 1 );
			}
		}
		pt.freewalk();
		return 0;
	}

	int ProcessManager::execve( eastl::string path, eastl::vector<eastl::string> argv,
								eastl::vector<eastl::string> envs )
	{
		Pcb			 *proc = get_cur_pcb();
		uint64		  sp;
		uint64		  stackbase;
		mm::PageTable pt;
		elf::elfhdr	  elf;
		elf::proghdr  ph = {};
		// fs::fat::Fat32DirInfo dir_;
		fs::dentry	 *de;
		int			  i, off;

		// proc->_pt.freewalk();
		// mm::k_vmm.vmfree( proc->_pt, proc->_sz );

		// if ( ( de = fs::fat::k_fatfs.get_root_dir()->EntrySearch( path ) )
		// 							== nullptr )
		eastl::string ab_path;
		if ( path[0] == '/' )
			ab_path = path;
		else
			ab_path = proc->_cwd_name + path;

		log_trace( "execve file : %s", ab_path.c_str() );

		fs::Path path_resolver( ab_path );
		if ( ( de = path_resolver.pathSearch() ) == nullptr )
		// if ( ( de = fs::ramfs::k_ramfs.getRoot()->EntrySearch( "mnt"
		// )->EntrySearch( path ) ) == nullptr )
		{
			log_error( "execve: cannot find file" );
			return -1; // 拿到文件夹信息
		}

		/// @todo check ELF header
		de->getNode()->nodeRead( reinterpret_cast<uint64>( &elf ), 0, sizeof( elf ) );

		if ( elf.magic != elf::elfEnum::ELF_MAGIC ) // check magicnum
		{
			log_error( "execve: not a valid ELF file" );
			return -1;
		}

		/// @todo 这里有bug，如果后面的代码失败，
		///       那么，原来的进程映像就全被释放掉了
		// proc->_pt.freewalk_mapped();
		// _proc_create_vm( proc );

		/// @todo 应当先创建一个新的
		mm::PageTable new_pt = mm::k_vmm.vm_create();
		u64			  new_sz = 0;


		// create user pagetable for given process
		// if((pt = proc_pagetable(proc)).is_null()){
		// 	log_error("execve: cannot create pagetable");
		// 	return -1;
		// }
		using psd_t		  = program_section_desc;
		int	  new_sec_cnt = 0;
		psd_t new_sec_desc[max_program_section_num];
		{
			bool load_bad = false;

			for ( i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof( ph ) )
			{
				de->getNode()->nodeRead( reinterpret_cast<uint64>( &ph ), off, sizeof( ph ) );

				if ( ph.type != elf::elfEnum::ELF_PROG_LOAD ) continue;
				if ( ph.memsz < ph.filesz )
				{
					log_error( "execve: memsz < filesz" );
					load_bad = true;
					break;
				}
				if ( ph.vaddr + ph.memsz < ph.vaddr )
				{
					log_error( "execve: vaddr + memsz < vaddr" );
					load_bad = true;
					break;
				}
				uint64 sz1;
				bool   executable = ( ph.flags & 0x1 ); // 段是否可执行？
				ulong  pva		  = hsai::page_round_down( ph.vaddr );
				if ( ( sz1 = mm::k_vmm.vm_alloc( new_pt, pva, ph.vaddr + ph.memsz, executable ) ) ==
					 0 )
				{
					log_error( "execve: uvmalloc" );
					load_bad = true;
					break;
				}
				new_sz += hsai::page_round_up( ph.vaddr + ph.memsz ) - pva;
				// if ( ( ph.vaddr % hsai::page_size ) != 0 )
				// {
				// 	log_error( "execve: vaddr not aligned" );
				// 	proc_freepagetable( proc->_pt, sz );
				// 	return -1;
				// }

				if ( load_seg( new_pt, ph.vaddr, de, ph.off, ph.filesz ) < 0 )
				{
					log_error( "execve: load_icode" );
					load_bad = true;
					break;
				}

				// 记录程序段

				new_sec_desc[new_sec_cnt]._sec_start  = (void *) ph.vaddr;
				new_sec_desc[new_sec_cnt]._sec_size	  = ph.memsz;
				new_sec_desc[new_sec_cnt]._debug_name = "LOAD";
				new_sec_cnt++;
			}

			if ( load_bad ) // load 阶段出错，释放页表
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				return -1;
			}
		}

		// 为程序映像转储 elf 程序头

		// sz		 = hsai::page_round_up(sz);
		u64 phdr = 0; // for AT_PHDR
		{
			ulong phsz = elf.phentsize * elf.phnum;
			u64	  sz1;
			u64	  load_end = 0;

			for ( auto &sec : new_sec_desc ) // 搜索load段末尾地址
			{
				u64 end = (ulong) sec._sec_start + sec._sec_size;
				if ( end > load_end ) load_end = end;
			}

			load_end = hsai::page_round_up( load_end );
			if ( ( sz1 = mm::k_vmm.vm_alloc( new_pt, load_end, load_end + phsz ) ) == 0 )
			{
				log_error( "execve: vaalloc" );
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				return -1;
			}

			u8 *tmp = new u8[phsz + 8];
			if ( tmp == nullptr )
			{
				log_error( "execve: no mem" );
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				return -1;
			}

			if ( de->getNode()->nodeRead( (ulong) tmp, elf.phoff, phsz ) != phsz )
			{
				log_error( "execve: node read" );
				delete[] tmp;
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				return -1;
			}

			if ( mm::k_vmm.copyout( new_pt, load_end, (void *) tmp, phsz ) < 0 )
			{
				log_error( "execve: copy out" );
				delete[] tmp;
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				return -1;
			}

			delete[] tmp;

			phdr = load_end;

			// 将该段作为程序段记录下来
			psd_t &ph_psd	   = new_sec_desc[new_sec_cnt];
			ph_psd._sec_start  = (void *) phdr;
			ph_psd._sec_size   = phsz;
			ph_psd._debug_name = "program headers";
			new_sec_cnt++;

			new_sz += hsai::page_round_up( phsz );
		}

		// allocate two pages , the second is used for the user stack

		// 此处分配栈空间遵循 memlayout
		// 进程的用户虚拟空间占用地址低 64GiB，内核虚拟空间从 0xFFF0_0000_0000
		// 开始 分配栈空间大小为 32 个页面，开头的 1 个页面用作保护页面

		int stack_page_cnt = default_proc_ustack_pages;
		stackbase		   = mm::vml::vm_ustack_end - stack_page_cnt * hsai::page_size;
		sp				   = mm::vml::vm_ustack_end;

		if ( mm::k_vmm.vm_alloc( new_pt, stackbase - hsai::page_size, sp ) == 0 )
		{
			log_error( "execve: vmalloc when allocating stack" );
			_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
			return -1;
		}

		ulong phy_stackbase = new_pt.walk_addr( stackbase );
		log_trace( "execve set stack-base = %p", phy_stackbase );
		log_trace( "execve set page containing sp is %p",
				   new_pt.walk_addr( sp - hsai::page_size ) );

		if ( mm::k_vmm.vm_set_super( new_pt, stackbase - hsai::page_size, 1 ) < 0 )
		{
			log_error( "execve: set stack protector fail" );
			_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
			return -1;
		}

		mm::k_vmm.vm_set( new_pt, (void *) stackbase, 0, stack_page_cnt );

		new_sz += ( stack_page_cnt + 1 ) * hsai::page_size;

		// >>>> 此后的代码用于支持 glibc，包括将 auxv, envp, argv, argc
		// 压到用户栈中由glibc解析

		mm::UserstackStream ustack( (void *) stackbase, stack_page_cnt * hsai::page_size, &new_pt );
		ustack.open();

		// 1. 使用0标记栈底，压入一个用于glibc的伪随机数，并以16字节对齐

		u64 rd_pos = 0;
		{
			ulong data;
			data = 0;
			ustack << data;
			data = -0x11'4514'FF11'4514UL;
			ustack << data;
			// data = 0x0050'4D4F'4353'4F43UL;
			data = 0x2UL << 60;
			ustack << data;
			// data = 0x4249'4C47'4B43'5546UL; // "FUCKGLIBCOSCOMP\0"
			data = 0x3UL << 60;
			ustack << data;

			rd_pos = ustack.sp(); // 伪随机数的位置
		}

		// 2. 压入 env string

		ulong uenvp[MAXARG];
		ulong envc;
		for ( envc = 0; envc < envs.size(); envc++ )
		{
			if ( envc >= MAXARG )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: too many arguments" );
				return -1;
			}

			sp		= ustack.sp();
			ustack -= ( sp - envs[envc].length() - 1 ) % 16;

			if ( sp < stackbase )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: sp < stackbase" );
				return -1;
			}
			ustack << envs[envc].c_str();
			if ( ustack.errno() != ustack.rc_ok )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: push into stack" );
				return -1;
			}

			uenvp[envc] = ustack.sp();
		}
		uenvp[envc] = 0; // envp[end] = nullptr

		// 3. 压入 arg string

		uint64 uargv[MAXARG];
		ulong  argc = 0;
		for ( ; argc < argv.size(); argc++ )
		{
			if ( argc >= MAXARG )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: too many arguments" );
				return -1;
			}

			sp		= ustack.sp();
			ustack -= ( sp - argv[argc].length() - 1 ) % 16;

			if ( sp < stackbase )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: sp < stackbase" );
				return -1;
			}

			ustack << argv[argc].c_str();
			if ( ustack.errno() != ustack.rc_ok )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: push into stack" );
				return -1;
			}

			uargv[argc] = ustack.sp();
		}
		uargv[argc] = 0; // argv[end] = nullptr

		sp		= ustack.sp();
		ustack -= sp % 16;

		// 4. 压入 auxv
		{
			elf::Elf64_auxv_t aux;
			// auxv[end] = AT_NULL
			aux.a_type	   = elf::AT_NULL;
			aux.a_un.a_val = 0;
			ustack << aux;
			if ( ustack.errno() != ustack.rc_ok )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: push into stack" );
				return -1;
			}

			if ( phdr != 0 )
			{
				// auxy[4] = AT_PAGESZ
				aux.a_type	   = elf::AT_PAGESZ;
				aux.a_un.a_val = hsai::page_size;
				ustack << aux;
				if( ustack.errno() != ustack.rc_ok)
				{
					_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
					log_error( "execve: push into stack" );
					return -1;
				}
				// auxv[3] = AT_PHNUM
				aux.a_type	   = elf::AT_PHNUM;
				aux.a_un.a_val = elf.phnum;
				ustack << aux;
				if ( ustack.errno() != ustack.rc_ok )
				{
					_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
					log_error( "execve: push into stack" );
					return -1;
				}
				// auxv[2] = AT_PHENT
				aux.a_type	   = elf::AT_PHENT;
				aux.a_un.a_val = elf.phentsize;
				ustack << aux;
				if ( ustack.errno() != ustack.rc_ok )
				{
					_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
					log_error( "execve: push into stack" );
					return -1;
				}
				// auxv[1] = AT_PHDR
				aux.a_type	   = elf::AT_PHDR;
				aux.a_un.a_val = phdr;
				ustack << aux;
				if ( ustack.errno() != ustack.rc_ok )
				{
					_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
					log_error( "execve: push into stack" );
					return -1;
				}
			}
			// auxv[0] = AT_RANDOM
			aux.a_type	   = elf::AT_RANDOM;
			aux.a_un.a_val = rd_pos;
			ustack << aux;
			if ( ustack.errno() != ustack.rc_ok )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: push into stack" );
				return -1;
			}
		}

		// 5. 压入 envp

		for ( long i = envc; i >= 0; --i )
		{
			ustack << uenvp[i];
			if ( ustack.errno() != ustack.rc_ok )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: push into stack" );
				return -1;
			}
		}

		// 6. 压入 argv

		for ( long i = argc; i >= 0; --i )
		{
			ustack << uargv[i];
			if ( ustack.errno() != ustack.rc_ok )
			{
				_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
				log_error( "execve: push into stack" );
				return -1;
			}
		}

		// 7. 压入 argc

		ustack << argc;
		if ( ustack.errno() != ustack.rc_ok )
		{
			_free_pt_with_sec( new_pt, new_sec_desc, new_sec_cnt );
			log_error( "execve: push into stack" );
			return -1;
		}

		// arguments to user main(argc, argv)
		// argc is returned via the system call return
		// value, which is in a0.
		// hsai::set_trap_frame_arg( proc->_trapframe, 1, sp );

		// 配置资源限制

		proc->_rlim_vec[ResourceLimitId::RLIMIT_STACK].rlim_cur =
			proc->_rlim_vec[ResourceLimitId::RLIMIT_STACK].rlim_max = ustack.sp() - stackbase;

		// 处理 F_DUPFD_CLOEXEC 标志位
		for ( auto &f : proc->_ofile )
		{
			if ( f != nullptr && f->_fl_cloexec )
			{
				f->free_file();
				f = nullptr;
			}
		}

		// save program name for debugging.
		for ( uint i = 0; i < sizeof proc->_name; i++ )
		{
			if ( i < path.size() )
				proc->_name[i] = path[i];
			else
				proc->_name[i] = 0;
		}

		// commit to the user image.
		proc->exe = ab_path;
		proc->_sz = new_sz;
		// unmap program sections
		for ( int i = 0; i < proc->_prog_section_cnt; i++ )
		{
			auto &osc = proc->_prog_sections[i];
			ulong sec_start, sec_end;
			sec_start = hsai::page_round_down( (ulong) osc._sec_start );
			sec_end	  = hsai::page_round_up( (ulong) osc._sec_start + osc._sec_size );
			mm::k_vmm.vm_unmap( proc->_pt, sec_start, ( sec_end - sec_start ) / hsai::page_size,
								1 );
		}
		{ // unmap heap
			ulong start = hsai::page_round_down( proc->_heap_start );
			ulong end	= hsai::page_round_up( proc->_heap_ptr );
			mm::k_vmm.vm_unmap( proc->_pt, start, ( end - start ) / hsai::page_size, 1 );
		}
		{ // unmap stack
			ulong pgs	= default_proc_ustack_pages + 1;
			ulong start = mm::vm_ustack_end - pgs * hsai::page_size;
			mm::k_vmm.vm_unmap( proc->_pt, start, pgs, 1 );
		}
		proc->_heap_start = 0;
		for ( int i = 0; i < new_sec_cnt; ++i )
		{
			auto &sec				= new_sec_desc[i];
			proc->_prog_sections[i] = sec;
			ulong sec_end			= hsai::page_round_up( (ulong) sec._sec_start + sec._sec_size );
			if ( sec_end > proc->_heap_start ) proc->_heap_start = sec_end;
		}
		proc->_prog_section_cnt = new_sec_cnt;
		mm::k_vmm.vm_unmap( proc->_pt, mm::vml::vm_trap_frame, 1, 0 );
		hsai::proc_free( proc );
		proc->_pt.freewalk();
		_proc_create_vm( proc, new_pt );
		hsai::proc_init( proc );

		if(!proc->_kpt.is_null())
		{
			mm::PageTable kpt;
			kpt = mm::k_vmm.vm_create();
			memcpy( (void *) kpt.get_base(), (void *) mm::k_pagetable.get_base(),
					hsai::page_size  );

			ulong pgs	= default_proc_ustack_pages + 1;
			ulong start = mm::vm_ustack_end - pgs * hsai::page_size;
			mm::k_vmm.map_data_pages( kpt, start, pgs * hsai::page_size, 
				phy_stackbase, false );

			hsai::VirtualCpu * cpu = hsai::get_cpu();
			cpu->set_mmu( kpt );

			mm::k_vmm.vm_unmap( proc->_kpt, start, pgs, 0 );
			proc->_kpt.kfreewalk( stackbase - hsai::page_size );

			proc->_kpt = kpt;
		}

		proc->_heap_ptr = proc->_heap_start;

		hsai::set_trap_frame_entry( proc->_trapframe, (void *) elf.entry );
		hsai::set_trap_frame_user_sp( proc->_trapframe, ustack.sp() );
		hsai::set_trap_frame_arg( proc->_trapframe, 1, ustack.sp() );
		proc->_state = ProcState::runnable;

		/// @note 此处是为了兼容glibc的需要，详见 how_to_adapt_glibc.md
		return 0x0; // rtld_fini
	}

	int ProcessManager::load_seg( mm::PageTable &pt, uint64 va, fs::dentry *de, uint offset,
								  uint size )
	{ // 好像没有机会返回 -1, pa失败的话会panic，de的read也没有返回值
		uint   i, n;
		uint64 pa;

		i = 0;
		if ( !hsai::is_page_align( va ) ) // 如果va不是页对齐的，先读出开头不对齐的部分
		{
			pa = pt.walk_addr( va );
			pa = hsai::k_mem->to_vir( pa );
			n  = hsai::page_round_up( va ) - va;
			de->getNode()->nodeRead( pa, offset + i, n );
			i += n;
		}

		for ( ; i < size; i += hsai::page_size ) // 此时 va + i 地址是页对齐的
		{
			pa = (uint64) pt.walk( va + i, 0 ).to_pa(); // pte.to_pa() 得到的地址是页对齐的
			if ( pa == 0 ) log_panic( "load_seg: walk" );
			if ( size - i < hsai::page_size ) // 如果是最后一页中的数据
				n = size - i;
			else
				n = hsai::page_size;
			de->getNode()->nodeRead( hsai::k_mem->to_vir( pa ), offset + i, n );
		}
		return 0;
	}


	int ProcessManager::wait( int child_pid, uint64 addr )
	{
		Pcb *p = k_pm.get_cur_pcb();
		int	 havekids, pid;
		Pcb *np = nullptr;
		if ( child_pid > 0 )
		{
			bool has_child = false;
			for ( auto &tmp : k_proc_pool )
			{
				if ( tmp._pid == child_pid && tmp.parent == p )
				{
					has_child = true;
					break;
				}
			}
			if ( !has_child ) return -1;
		}

		_wait_lock.acquire();
		for ( ;; )
		{
			havekids = 0;
			for ( np = k_proc_pool; np < &k_proc_pool[num_process]; np++ )
			{
				if ( child_pid > 0 && np->_pid != child_pid ) continue;

				if ( np->parent == p )
				{
					np->_lock.acquire();
					havekids = 1;

					if ( np->get_state() == ProcState::zombie )
					{
						pid = np->_pid;
						if ( addr != 0 &&
							 mm::k_vmm.copyout( p->_pt, addr, (const char *) &np->_xstate,
												sizeof( np->_xstate ) ) < 0 )
						{
							np->_lock.release();
							_wait_lock.release();
							return -1;
						}
						/// @todo release shm

						k_pm.freeproc( np );
						np->_lock.release();
						_wait_lock.release();
						return pid;
					}
					np->_lock.release();
				}
			}

			if ( !havekids || p->_killed )
			{
				_wait_lock.release();
				return -1;
			}

			// wait children to exit
			sleep( p, &_wait_lock );
		}
	}

	void ProcessManager::exit_proc( Pcb *p, int state )
	{
		if(p == _init_proc)
			log_panic("init exiting");
		// log_info( "exit proc %d", p->_pid );

		_wait_lock.acquire();
		reparent( p );

		if ( p->parent ) wakeup( p->parent );

		p->_lock.acquire();
		p->_xstate = state << 8;
		p->_state  = ProcState::zombie;

		_wait_lock.release();

		k_scheduler.call_sched(); // jump to schedular, never return
		log_panic( "zombie exit" );
	}

	// Pass p's abandoned children to init.
	void ProcessManager::reparent(Pcb *p)
	{
		Pcb *pp;
		for ( uint i = 0; i < num_process; i++ )
		{
			pp = &k_proc_pool[( _last_alloc_proc_gid + i ) % num_process];
			if(pp->parent == p){
				pp->_lock.acquire();
				pp->parent = _init_proc;
				pp->_lock.release();
			}
		}
	}

	void ProcessManager::exit( int state )
	{
		Pcb *p = get_cur_pcb();

		exit_proc( p, state );
	}

	void ProcessManager::exit_group( int status )
	{
		void *stk[num_process + 10]; // 这里不使用stl库是因为 exit
									 // 后不会返回，无法调用析构函数，可能造成内存泄露
		int	  stk_ptr = 0;

		u8 visit[num_process];
		memset( (void *) visit, 0, sizeof visit );
		pm::Pcb *cp = get_cur_pcb();

		_wait_lock.acquire();

		for ( uint i = 0; i < num_process; i++ )
		{
			if ( k_proc_pool[i]._state == ProcState::unused ) continue;
			if ( visit[i] != 0 ) continue;

			pm::Pcb *p	 = &k_proc_pool[i];
			visit[i]	 = 1;
			stk[stk_ptr] = (void *) p;
			stk_ptr++;
			bool need_chp = false;

			while ( p->parent != nullptr )
			{
				if ( p->parent == cp )
				{
					need_chp = true;
					break;
				}
				p			   = p->parent;
				visit[p->_gid] = 1;
				stk[stk_ptr]   = (void *) p;
				stk_ptr++;
			}

			while ( stk_ptr > 0 )
			{
				pm::Pcb *tp = (pm::Pcb *) stk[stk_ptr - 1];
				stk_ptr--;
				if ( need_chp ) { freeproc( tp ); }
			}
		}

		_wait_lock.release();

		exit_proc( cp, status );
	}

	void ProcessManager::wakeup( void *chan )
	{
		Pcb *p;
		for ( p = k_proc_pool; p < &k_proc_pool[num_process]; p++ )
		{
			if ( p != get_cur_pcb() )
			{
				p->_lock.acquire();
				if ( p->_state == ProcState::sleeping && p->_chan == chan )
				{
					p->_state = ProcState::runnable;
				}
				p->_lock.release();
			}
		}
	}

	void ProcessManager::sleep( void *chan, hsai::SpinLock *lock )
	{
		Pcb *proc = k_pm.get_cur_pcb();

		// get the lock to release and change it's state to scheduler
		proc->_lock.acquire();
		lock->release();

		proc->_chan	 = chan;
		proc->_state = ProcState::sleeping;

		k_scheduler.call_sched();
		proc->_chan = 0;

		proc->_lock.release();
		lock->acquire();
	}

	long ProcessManager::brk( long n )
	{
		Pcb *p = get_cur_pcb(); // 输入参数	：期望的堆大小

		if ( n <= 0 ) // get current heap size
			return p->_heap_ptr;

		// uint64 sz = p->_sz;		// 输出  	：实际的堆大小
		uint64		   oldhp  = p->_heap_ptr;
		uint64		   newhp  = n;
		mm::PageTable &pt	  = p->_pt;
		long		   differ = (long) newhp - (long) oldhp;


		if ( differ < 0 ) // shrink
		{
			if ( mm::k_vmm.vm_dealloc( pt, oldhp, newhp ) < 0 ) { return -1; }
		}
		else if ( differ > 0 )
		{
			if ( mm::k_vmm.vm_alloc( pt, oldhp, newhp ) == 0 ) return -1;
		}

		// log_info( "brk: newsize%d, oldsize%d", newhp, oldhp );
		p->_heap_ptr = newhp;
		return newhp; // 返回堆的大小
	}

	int ProcessManager::mkdir( int dir_fd, eastl::string path, uint flags )
	{
		Pcb *p = get_cur_pcb();
		fs::file   *file = nullptr;
		fs::dentry *dentry;

		if ( dir_fd != AT_FDCWD ) { file = p->get_open_file( dir_fd ); }

		fs::Path path_( path, file );
		dentry = path_.pathSearch();

		if ( dentry == nullptr) 
		{
			fs::dentry *par_ = path_.pathSearch( true );
			if( par_ == nullptr )
			    return -1;
			fs::FileAttrs attrs;
			attrs.filetype = fs::FileTypes::FT_DIRECT;
			attrs._value = 0777;
			if( ( dentry = par_->EntryCreate( path_.rFileName(), attrs ) ) == nullptr )
			{
				printf("Error creating new dentry %s failed\n", path_.rFileName() );
				return -1;
			}
		}
		if ( dentry == nullptr ) return -1;
		return 0;
	}

	int ProcessManager::open( int dir_fd, eastl::string path, uint flags )
	{
		// enum OpenFlags : uint
		// {
		// 	O_RDONLY	= 0x000U,
		// 	O_WRONLY	= 0x001U,
		// 	O_RDWR		= 0x002U,
		// 	O_CREATE	= 0x040U,
		// 	O_DIRECTORY = 0x020'0000U
		// };

		Pcb *p = get_cur_pcb();
		fs::file   *file = nullptr;
		fs::dentry *dentry;

		if ( dir_fd != AT_FDCWD ) { file = p->get_open_file( dir_fd ); }

		fs::Path path_( path, file );
		dentry = path_.pathSearch();

		if ( dentry == nullptr && flags & O_CREAT ) 
		{
			// @todo: create file
			fs::dentry *par_ = path_.pathSearch( true );
			if( par_ == nullptr )
			    return -1;
			fs::FileAttrs attrs;
			if ((flags & __S_IFMT) == S_IFDIR)
				attrs.filetype = fs::FileTypes::FT_DIRECT;
			else	
				attrs.filetype = fs::FileTypes::FT_NORMAL;
			attrs._value = 0777;
			if( ( dentry = par_->EntryCreate( path_.rFileName(), attrs ) ) == nullptr )
			{
				printf("Error creating new dentry %s failed\n", path_.rFileName() );
				return -1;
			}

		}
		if ( dentry == nullptr ) return -1; // file is not found

		int			  dev	= dentry->getNode()->rDev();
		fs::FileAttrs attrs = dentry->getNode()->rMode();

		if ( dev >= 0 ) // dentry is a device
		{
			fs::device_file *f = new fs::device_file( attrs, dev, dentry );
			return alloc_fd( p, f );
		} // else if( attrs.filetype == fs::FileTypes::FT_DIRECT)
		// 	fs::directory *f = new fs::directory( attrs, dentry );
		else // normal file
		{
			fs::normal_file *f = new fs::normal_file( attrs, dentry );
			// log_info( "test normal file read" );
			// {
			// 	fs::file *ff = ( fs::file * ) f;
			// 	char buf[ 8 ];
			// 	ff->read( ( ulong ) buf, 8 );
			// 	buf[ 8 ] = 0;
			// 	printf( "%s\n", buf );
			// }
			if( flags & O_APPEND )
				f->setAppend();
			return alloc_fd( p, f );
		} // because of open.c's fileattr defination is not clearly, so here we
		  // set flags = 7, which means O_RDWR | O_WRONLY | O_RDONLY

		// return alloc_fd( p, f );
	}

	int ProcessManager::close( int fd )
	{
		if ( fd < 0 || fd >= (int) max_open_files ) return -1;
		Pcb *p = get_cur_pcb();
		if ( p->_ofile[fd] == nullptr ) return 0;
		// fs::k_file_table.free_file( p->_ofile[ fd ] );
		p->_ofile[fd]->free_file();
		p->_ofile[fd] = nullptr;
		return 0;
	}

	int ProcessManager::fstat( int fd, fs::Kstat *st )
	{
		if ( fd < 0 || fd >= (int) max_open_files ) return -1;

		Pcb *p = get_cur_pcb();
		if ( p->_ofile[fd] == nullptr ) return -1;
		fs::file *f = p->_ofile[fd];
		*st			= f->_stat;

		return 0;
	}

	int ProcessManager::chdir( eastl::string &path )
	{
		Pcb *p = get_cur_pcb();

		fs::dentry *dentry;

		fs::Path pt( path );
		dentry = pt.pathSearch();
		// dentry = p->_cwd->EntrySearch( path );
		if ( dentry == nullptr ) return -1;
		p->_cwd		 = dentry;
		p->_cwd_name = path;
		return 0;
	}

	int ProcessManager::getcwd( char *out_buf )
	{
		Pcb *p = get_cur_pcb();

		eastl::string cwd;
		cwd	   = p->_cwd_name;
		uint i = 0;
		for ( ; i < cwd.size(); ++i ) out_buf[i] = cwd[i];
		out_buf[i] = '\0';
		return i + 1;
	}

	int ProcessManager::mmap( int fd, int map_size )
	{
		/// TODO: actually, it shall map buffer and pin buffer at memory

		Pcb *p = get_cur_pcb();

		if ( fd <= 2 || fd >= (int) max_open_files || map_size < 0 ) return -1;

		fs::file *f = p->_ofile[fd];
		if ( f->_attrs.filetype != fs::FileTypes::FT_NORMAL ) return -1;

		fs::normal_file *normal_f = static_cast<fs::normal_file *>( f );
		fs::dentry		*dent	  = normal_f->getDentry();
		if ( dent == nullptr ) return -1;

		uint64 fsz = (uint64) map_size;
		uint64 fst = p->_sz;

		uint64 newsz = mm::k_vmm.vm_alloc( p->_pt, fst, fst + fsz );
		if ( newsz == 0 ) return -1;

		p->_sz = newsz;

		char *buf = new char[fsz + 1];
		dent->getNode()->nodeRead( (uint64) buf, 0, fsz );

		if ( mm::k_vmm.copyout( p->_pt, fst, (const void *) buf, fsz ) < 0 )
		{
			delete[] buf;
			return -1;
		}

		delete[] buf;
		return fst;
	}

	int ProcessManager::unlink( int fd, eastl::string path, int flags )
	{

		if ( fd == -100 )
		{					  // atcwd
			if ( path == "" ) // empty path
				return -1;

			if ( path[0] == '.' && path[1] == '/' ) path = path.substr( 2 );

			return fs::k_file_table.unlink( path );
		}
		else
		{
			return -1; // current not support other dir, only for cwd
		}
	}

	int ProcessManager::pipe( int *fd, int flags )
	{
		fs::pipe_file *rf, *wf;
		rf = nullptr;
		wf = nullptr;

		int	 fd0, fd1;
		Pcb *p = get_cur_pcb();

		ipc::Pipe *pipe_ = new ipc::Pipe();
		if ( pipe_->alloc( rf, wf ) < 0 ) return -1;
		fd0 = -1;
		if ( ( ( fd0 = alloc_fd( p, rf ) ) < 0 ) || ( fd1 = alloc_fd( p, wf ) ) < 0 )
		{
			if ( fd0 >= 0 ) p->_ofile[fd0] = 0;
			// fs::k_file_table.free_file( rf );
			// fs::k_file_table.free_file( wf );
			rf->free_file();
			wf->free_file();
			return -1;
		}
		p->_ofile[fd0] = rf;
		p->_ofile[fd1] = wf;
		fd[0]		   = fd0;
		fd[1]		   = fd1;
		return 0;
	}

	int ProcessManager::set_tid_address( int *tidptr )
	{
		Pcb *p				= get_cur_pcb();
		p->_clear_child_tid = tidptr;
		return p->_pid;
	}

	int ProcessManager::set_robust_list( robust_list_head *head, size_t len )
	{
		if ( len != sizeof( *head ) ) return -22;

		Pcb *p			= get_cur_pcb();
		p->_robust_list = head;

		return 0;
	}

	int ProcessManager::prlimit64( int pid, int resource, rlimit64 *new_limit, rlimit64 *old_limit )
	{
		Pcb *proc = nullptr;
		if ( pid == 0 )
			proc = get_cur_pcb();
		else
			for ( Pcb &p : k_proc_pool )
			{
				if ( p._pid == pid )
				{
					proc = &p;
					break;
				}
			}
		if ( proc == nullptr ) return -10;

		ResourceLimitId rsid = (ResourceLimitId) resource;
		if ( rsid >= ResourceLimitId::RLIM_NLIMITS ) return -11;

		if ( old_limit != nullptr ) *old_limit = proc->_rlim_vec[rsid];
		if ( new_limit != nullptr ) proc->_rlim_vec[rsid] = *new_limit;

		return 0;
	}

	int ProcessManager::alloc_fd( Pcb *p, fs::file *f )
	{
		int fd;

		for ( fd = 3; fd < (int) max_open_files; fd++ )
		{
			if ( p->_ofile[fd] == nullptr )
			{
				p->_ofile[fd] = f;
				return fd;
			}
		}
		return -1;
	}

	int ProcessManager::alloc_fd( Pcb *p, fs::file *f, int fd )
	{
		// if ( fd <= 2 || fd >= ( int ) max_open_files )
		// 	return -1;
		// if ( p->_ofile[ fd ] != nullptr )
		// 	return -1;
		p->_ofile[fd] = f;
		return fd;
	}

	void ProcessManager::get_cur_proc_tms( tmm::tms *tsv )
	{
		Pcb	  *p		= get_cur_pcb();
		uint64 cur_tick = tmm::k_tm.get_ticks();

		tsv->tms_utime	= p->_user_ticks;
		tsv->tms_stime	= cur_tick - p->_start_tick - p->_user_ticks;
		tsv->tms_cstime = tsv->tms_cutime = 0;

		for ( auto &pp : k_proc_pool )
		{
			if ( pp._state == ProcState::unused || pp.parent != p ) continue;
			tsv->tms_cutime += pp._user_ticks;
			tsv->tms_cstime += cur_tick - pp._start_tick - pp._user_ticks;
		}
	}

	// ---------------- private helper functions ----------------

	void ProcessManager::_proc_create_vm( Pcb *p, mm::PageTable &pt )
	{
		if ( pt.get_base() == 0 ) return;

		if ( !mm::k_vmm.map_data_pages( pt, mm::vml::vm_trap_frame, hsai::page_size,
										(uint64) ( p->_trapframe ), false ) )
		{
			mm::k_vmm.vm_unmap( pt, mm::vm_trap_frame, 1, 0 );
			log_panic( "proc create vm but no mem." );
			return;
		}

		p->_pt = pt;
	}

	void ProcessManager::_proc_create_vm( Pcb *p )
	{
		mm::PageTable pt;

		pt = mm::k_vmm.vm_create();

		if ( pt.get_base() == 0 ) { return; }

		_proc_create_vm( p, pt );
#ifndef LOONGARCH
		mm::PageTable kpt;
		kpt = mm::k_vmm.vm_create();
		memcpy( (void *) kpt.get_base(), (void *) mm::k_pagetable.get_base(),
				hsai::page_size  );

		p->_kpt = kpt;
#endif
	}

	// ---------------- test function ----------------

	void ProcessManager::vectortest()
	{
		eastl::vector<int> v;

		// 测试 push_back
		v.push_back( 1 );
		v.push_back( 2 );
		v.push_back( 3 );
		v.push_back( 4 );

		// 测试 size
		log_trace( "vector size: %d\n", v.size() );

		// 测试 capacity
		log_trace( "vector capacity: %d\n", v.capacity() );

		// 测试 empty
		log_trace( "vector is empty: %d\n", v.empty() );

		// 测试 at
		log_trace( "vector at 2: %d\n", v.at( 2 ) );

		// 测试 front
		log_trace( "vector front: %d\n", v.front() );

		// 测试 back
		log_trace( "vector back: %d\n", v.back() );

		// 测试 insert
		v.insert( v.begin() + 2, 5 );
		log_trace( "vector after insert: " );
		for ( auto i = v.begin(); i != v.end(); i++ ) { log_trace( "%d ", *i ); }
		log_trace( "\n" );

		// 测试 erase
		v.erase( v.begin() + 2 );
		log_trace( "vector after erase: " );
		for ( auto i = v.begin(); i != v.end(); i++ ) { log_trace( "%d ", *i ); }
		log_trace( "\n" );

		// 测试 swap
		eastl::vector<int> v2;
		v2.push_back( 6 );
		v2.push_back( 7 );
		v.swap( v2 );
		log_trace( "vector after swap: " );
		for ( auto i = v.begin(); i != v.end(); i++ ) { log_trace( "%d ", *i ); }
		log_trace( "\n" );

		// 测试 resize
		v.resize( 5, 8 );
		log_trace( "vector after resize: " );
		for ( auto i = v.begin(); i != v.end(); i++ ) { log_trace( "%d ", *i ); }
		log_trace( "\n" );

		// 测试 reserve
		v.reserve( 10 );
		log_trace( "vector capacity after reserve: %d\n", v.capacity() );

		// 测试 clear
		v.clear();
		log_trace( "vector size after clear: %d\n", v.size() );
	}

	void ProcessManager::stringtest()
	{
		eastl::string s;

		// 测试赋值
		s = "hello world";
		log_trace( "string: %s\n", s.c_str() );

		// 测试 size 和 length
		log_trace( "string size: %d\n", s.size() );
		log_trace( "string length: %d\n", s.length() );

		// 测试 empty
		log_trace( "string is empty: %d\n", s.empty() );

		// 测试 append
		s.append( " EASTL" );
		log_trace( "string after append: %s\n", s.c_str() );

		// 测试 insert
		s.insert( 5, ", dear" );
		log_trace( "string after insert: %s\n", s.c_str() );

		// 测试 erase
		s.erase( 5, 6 );
		log_trace( "string after erase: %s\n", s.c_str() );

		// 测试 replace
		s.replace( 6, 5, "EASTL" );
		log_trace( "string after replace: %s\n", s.c_str() );

		// 测试 substr
		eastl::string sub = s.substr( 6, 5 );
		log_trace( "substring: %s\n", sub.c_str() );

		// 测试 find
		[[maybe_unused]] size_t pos = s.find( "EASTL" );
		log_trace( "find EASTL at: %d\n", pos );

		// 测试 rfind
		pos = s.rfind( 'l' );
		log_trace( "rfind 'l' at: %d\n", pos );

		// 测试 compare
		[[maybe_unused]] int cmp = s.compare( "hello EASTL" );
		log_trace( "compare with 'hello EASTL': %d\n", cmp );
	}

	void ProcessManager::maptest()
	{
		eastl::map<int, int> m;

		// 测试 insert
		m.insert( eastl::make_pair( 1, 2 ) );
		m.insert( eastl::make_pair( 3, 4 ) );
		m.insert( eastl::make_pair( 5, 6 ) );

		// 测试 size
		log_trace( "map size: %d\n", m.size() );

		// 测试 empty
		log_trace( "map is empty: %d\n", m.empty() );

		// 测试 at
		log_trace( "map at 3: %d\n", m.at( 3 ) );

		// 测试 operator[]
		log_trace( "map[5]: %d\n", m[5] );

		// 测试 find
		[[maybe_unused]] auto it = m.find( 3 );
		log_trace( "find 3: %d\n", it->second );

		// 测试 erase
		m.erase( 3 );
		log_trace( "map size after erase: %d\n", m.size() );

		// 测试 clear
		m.clear();
		log_trace( "map size after clear: %d\n", m.size() );
	}

	void ProcessManager::hashtest()
	{
		eastl::hash_map<int, int> m;

		// 测试 insert
		m.insert( eastl::make_pair( 1, 2 ) );
		m.insert( eastl::make_pair( 3, 4 ) );
		m.insert( eastl::make_pair( 5, 6 ) );

		// 测试 size
		log_trace( "hash_map size: %d\n", m.size() );

		// 测试 empty
		log_trace( "hash_map is empty: %d\n", m.empty() );

		// 测试 at
		log_trace( "hash_map at 3: %d\n", m.at( 3 ) );

		// 测试 operator[]
		log_trace( "hash_map[5]: %d\n", m[5] );

		// 测试 find
		[[maybe_unused]] auto it = m.find( 3 );
		log_trace( "find 3: %d\n", it->second );

		// 测试 erase
		m.erase( 3 );
		log_trace( "hash_map size after erase: %d\n", m.size() );

		// 测试 clear
		m.clear();
		log_trace( "hash_map size after clear: %d\n", m.size() );
	}
} // namespace pm
