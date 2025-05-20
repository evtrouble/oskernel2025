//
// Created by Li Shuang ( pseudonym ) on 2024-05-18
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use
// | Contact Author: lishuang.mk@whu.edu.cn
// --------------------------------------------------------------
//

#include "usyscall.h"
typedef unsigned int size_t;

char u_init_stack[4096] __attribute__( ( section( ".user.init.stack" ) ) );

int			init_main( void ) __attribute__( ( section( ".user.init" ) ) );
// static void printint( int xx, int base, int sign ) __attribute__( ( section( ".user.init" ) ) );
// static size_t strlen( const char* s ) __attribute__( ( section( ".user.init" ) ) );

// __attribute__( ( section( ".user.init.data" ) ) ) const char nextline[]	  = "\n";
__attribute__( ( section( ".user.init.data" ) ) ) const char str[]	  = "\nHello World\n\n\n";
__attribute__( ( section( ".user.init.data" ) ) ) const char errstr[] = "fork fail\n";
// __attribute__( ( section( ".user.init.data" ) ) ) const char parent_str[] =
	// "parent write. fork pid is ";
// __attribute__( ( section( ".user.init.data" ) ) ) const char child_str[]	   = "child write\n";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_fail_str[]   = "execve fail\n";
// __attribute__( ( section( ".user.init.data" ) ) ) const char wait_success[]	   = "  wait success\n";
__attribute__( ( section( ".user.init.data" ) ) ) const char wait_fail[]	   = "wait fail\n";
// __attribute__( ( section( ".user.init.data" ) ) ) const char sleep_success[]   = "sleep_success\n";
// __attribute__( ( section( ".user.init.data" ) ) ) const char print_int_error[] = "printint error\n";
// __attribute__( ( section( ".user.init.data" ) ) ) const char to_open_file[]	   = "text.txt";
// __attribute__( ( section( ".user.init.data" ) ) ) const char read_fail[]	   = "read fail\n";


__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_basic[]	 = "run-all.sh";
__attribute__(( section( ".user.init.data" ) )) const char	 start_test_basic[] =
	"#### OS COMP TEST GROUP START basic-musl-glibc ####\n";
__attribute__(( section( ".user.init.data" ) )) const char	 end_test_basic[] =
	"#### OS COMP TEST GROUP END basic-musl-glibc ####\n";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_lua[]	 =
// "lua_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_libctest[]	 =
// "libctest_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_iozone[]	 =
// "iozone_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_unixbench[] =
// "unixbench_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_iperf[]	 =
// "iperf_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_libcbench[]	 =
// "libcbench_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_lmbench[]	 =
// "lmbench_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_netperf[]	 =
// "netperf_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_cyclictest[]	  =
// "cyclictest_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_ltp[]	  =
// "ltp_testcode.sh";

// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_busybox[] = "busybox";

__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_echo[]	 = "basic/write";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_fork[]	 = "fork";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_exit[]	 = "exit";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_wait[]	 = "wait";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_getpid[]	 = "getpid";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_getppid[] = "getppid";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_dup[]	 = "dup";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_dup2[]	 = "dup2";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_execve[]	 = "execve";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_getcwd[]	 = "getcwd";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_gettimeofday[] =
	"gettimeofday";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_yield[]	  = "yield";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_sleep[]	  = "sleep";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_times[]	  = "times";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_clone[]	  = "clone";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_brk[]	  = "brk";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_uname[]	  = "uname";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_waitpid[]  = "waitpid";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_open[]	  = "open";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_fstat[]	  = "fstat";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_openat[]	  = "openat";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_close[]	  = "close";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_read[]	  = "read";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_getdents[] = "getdents";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_mkdir[]	  = "mkdir_";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_chdir[]	  = "chdir";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_mount[]	  = "mount";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_umount[]	  = "umount";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_mmap[]	  = "mmap";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_munmap[]	  = "munmap";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_unlinkat[] = "unlink";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_pipe[]	  = "pipe";
// __attribute__( ( section( ".user.init.data.p" ) ) ) const char *exec_test_basic[] = {
//   exec_test_echo, exec_test_fork, exec_test_exit, 
//   exec_test_wait, exec_test_getpid, exec_test_getppid, 
//   exec_test_dup, exec_test_dup2, exec_test_execve, 
//   exec_test_getcwd, exec_test_gettimeofday, exec_test_yield, 
//   exec_test_sleep, exec_test_times, exec_test_clone, 
//   exec_test_brk, exec_test_uname, exec_test_waitpid, 
//   exec_test_open, exec_test_fstat, exec_test_openat, 
//   exec_test_close, exec_test_read, exec_test_getdents, 
//   exec_test_mkdir, exec_test_chdir, exec_test_mount, 
//   exec_test_umount, exec_test_mmap, exec_test_munmap, 
//   exec_test_unlinkat, exec_test_pipe
// };
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_busybox_unstrp[] =
// 	"busybox_unstrp";
// __attribute__( ( section( ".user.init.data" ) ) ) const char busybox_name[]	 = "busybox";
// __attribute__( ( section( ".user.init.data" ) ) ) const char busybox_arg_c[] = "-c";
// __attribute__( ( section( ".user.init.data" ) ) ) const char busybox_arg_cmd[] =
// 	"'echo hello,busybox!'";
__attribute__( ( section( ".user.init.data" ) ) ) const char sh_name[]	 = "sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char echo_name[] = "echo";
// __attribute__( ( section( ".user.init.data" ) ) ) const char cat_name[]	 = "cat";
// __attribute__( ( section( ".user.init.data" ) ) ) const char hello_busybox_str[] =
// 	"hello, busybox!\n";
// __attribute__( ( section( ".user.init.data" ) ) ) const char busybox_testcode_str[] =
// 	"/mnt/sdcard/busybox_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char test_sh_str[] = "/mnt/sdcard/test.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char lua_test_sh[] =
// 	"/mnt/sdcard/lua/test.sh";
__attribute__( ( section( ".user.init.data" ) ) ) const char test_musl_basic_path[] = "/mnt/musl/basic/";
__attribute__( ( section( ".user.init.data" ) ) ) const char test_glibc_basic_path[] = "/mnt/glibc/basic/";
__attribute__(( section( ".user.init.data.p" ) )) const char *bb_sh[8]		 = { 0 };
__attribute__(( section( ".user.init.data" ) )) const char	  busybox_path[] = "../busybox";
// __attribute__(( section( ".user.init.data.p" ) )) const char	busybox_path[]		 = "busybox";

// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_libcbench[] = "libc-bench";

// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench[]		 = "lmbench_all";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench_arg1[] = "lat_syscall";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench_arg2[] = "-P";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench_arg3[] = "1";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench_arg4[] = "read";

__attribute__(( section( ".user.init.data" ) )) const char rootpath[] = "basic/";

__attribute__( ( section( ".user.init.data" ) ) ) const char digits[] = "0123456789abcdef";

// __attribute__( ( __unused__ ) ) static void printint( int xx, int base, int sign )
// {
// 	char		 buf[16 + 1];
// 	int			 i;
// 	unsigned int x;

// 	if ( sign && ( sign = xx < 0 ) )
// 		x = -xx;
// 	else
// 		x = xx;

// 	buf[16] = 0;
// 	i		= 15;
// 	do {
// 		buf[i--] = digits[x % base];
// 	}
// 	while ( ( x /= base ) != 0 );

// 	if ( sign ) buf[i--] = '-';
// 	i++;
// 	if ( i < 0 ) write( 1, print_int_error, sizeof( print_int_error ) );
// 	write( 1, buf + i, 16 - i );
// }

// size_t strlen( const char *s )
// {
// 	size_t len = 0;
// 	while ( *s )s++, len++;
// 	return len;
// }

// typedef unsigned long long uint64;
// struct linux_dirent64
// {
// 	uint64		   d_ino;	 // 索引结点号
// 	uint64		   d_off;	 // 到下一个dirent的偏移
// 	unsigned short d_reclen; // 当前dirent的长度
// 	unsigned char  d_type;	 // 文件类型
// 	char		   d_name[]; // 文件名
// };

// static int execv( const char *path, const char *argv[] ) { return execve( path, argv, 0 ); }

#define RUN_TESTS(test_list) do { \
    pid = fork(); \
    if(pid < 0) { \
      write(1, errstr, sizeof(errstr)); \
    } else if(pid == 0) { \
      if(execve(test_list, 0, 0) < 0) { \
        write(1, exec_fail_str, sizeof(exec_fail_str)); \
      } \
      exit(0); \
    } else { \
      int child_exit_state = -100; \
      if(wait(-1, &child_exit_state) < 0) \
        write(1, wait_fail, sizeof(wait_fail)); \
    } \
} while(0)

int init_main( void )
{
	__attribute__(( __unused__ )) int pid;
	// write( 1, errstr, sizeof( errstr ) );
	// write( 1, exec_test_basic[0], sizeof( exec_test_basic[0] ) );

	// __attribute__( ( __unused__ ) ) const char *bb_sh[8] = { 0 };

	// pid = fork();
	// if ( pid < 0 ) { write( 1, errstr, sizeof( errstr ) ); }
	// else if ( pid == 0 )
	// {
	// 	chdir( busybox_path );
	// 	bb_sh[0] = sh_name;
	// 	bb_sh[1] = test_sh_str;
	// 	bb_sh[2] = 0;
	// 	bb_sh[3] = 0;
	// 	if ( execve( exec_time_test, 0, 0 ) < 0 )
	// 	{
	// 		write( 1, exec_fail_str, sizeof( exec_fail_str ) );
	// 	}
	// 	exit( 0 );
	// }
	// else
	// {
	// 	int child_exit_state = -100;
	// 	if ( wait( -1, &child_exit_state ) < 0 ) write( 1, wait_fail, sizeof( wait_fail ) );
	// 	// write( 1, exec_fail_str, 21 );
	// }
	// char		path[]	= "/mnt/glibc/busybox";
	// const char *bb_sh[] = { "/mnt/glibc/busybox", "basic_testcode.sh", 0 };
	chdir( test_glibc_basic_path );
	write( 1, start_test_basic, sizeof( start_test_basic ) );
	// RUN_TESTS( exec_test_echo );
	// char *bb_sh[] = { "sh", "basic_testcode.sh", 0 };
	bb_sh[0] = sh_name;
	bb_sh[1] = exec_test_basic;
	bb_sh[2] = 0;

	pid = fork(); 
    if(pid < 0) {
      write(1, errstr, sizeof(errstr)); 
    } else if(pid == 0) { 
      if(execve(busybox_path, bb_sh, 0) < 0) { 
        write(1, exec_fail_str, sizeof(exec_fail_str)); 
      } 
      exit(0); 
    } else { 
      int child_exit_state = -100; 
      if(wait(-1, &child_exit_state) < 0) 
        write(1, wait_fail, sizeof(wait_fail)); 
    } 
	write( 1, end_test_basic, sizeof( end_test_basic ) );
	
	// RUN_TESTS( exec_test_echo );
	// RUN_TESTS( exec_test_fork );
	// RUN_TESTS( exec_test_exit );
	// RUN_TESTS( exec_test_wait );
	// RUN_TESTS( exec_test_getpid );
	// RUN_TESTS( exec_test_getppid );
	// RUN_TESTS( exec_test_dup2 );
	// RUN_TESTS( exec_test_execve );
	// RUN_TESTS( exec_test_getcwd );
	// RUN_TESTS( exec_test_gettimeofday );
	// RUN_TESTS( exec_test_yield );
	// RUN_TESTS( exec_test_sleep );
	// RUN_TESTS( exec_test_times );
	// RUN_TESTS( exec_test_clone );
	// RUN_TESTS( exec_test_brk );
	// RUN_TESTS( exec_test_waitpid );
	// RUN_TESTS( exec_test_fstat );
	// RUN_TESTS( exec_test_openat );
	// RUN_TESTS( exec_test_close );
	// RUN_TESTS( exec_test_read );
	// RUN_TESTS( exec_test_getdents );
	// RUN_TESTS( exec_test_mkdir );
	// RUN_TESTS( exec_test_chdir );
	// RUN_TESTS( exec_test_mount );
	// RUN_TESTS( exec_test_umount );
	// RUN_TESTS( exec_test_munmap );
	// RUN_TESTS( exec_test_unlinkat );
	// RUN_TESTS( exec_test_pipe );
	//test_clone
	//test_waitpid
	//test_close
	//test_mount

	// pid = fork();
	// if ( pid < 0 ) { write( 1, errstr, sizeof( errstr ) ); }
	// else if ( pid == 0 )
	// {
	// 	// chdir( busybox_path );
	// 	// bb_sh[0] = sh_name;
	// 	// bb_sh[1] = 0;
	// 	// bb_sh[2] = 0;
	// 	// bb_sh[3] = 0;
	// 	// bb_sh[4] = 0;
	// 	if ( execve( rootpath, 0, 0 ) < 0 )
	// 	{
	// 		write( 1, exec_fail_str, sizeof( exec_fail_str ) );
	// 	}
	// 	exit( 0 );
	// }
	// else
	// {
	// 	int child_exit_state = -100;
	// 	if ( wait( -1, &child_exit_state ) < 0 ) write( 1, wait_fail, sizeof( wait_fail ) );
	// 	// write( 1, exec_fail_str, 21 );
	// }

	// char dents[512];

	// int fd = openat( -1, rootpath, 02, 0 );
	// int					   r;
	// struct linux_dirent64 *de;
	// // 读取目录项
	// while((r = getdents(fd, dents, 512)) != 0) {
	// 	if ( r < 0 ) 
	// 		break;
	// 	for(int i = 0; i < r; i += ((struct linux_dirent64 *)&dents[i])->d_reclen) {
    //         de = (struct linux_dirent64 *)&dents[i];
	// 		write( 1, de->d_name, strlen( de->d_name ) );
	// 		write( 1, nextline, sizeof( nextline ) - 1 );
	// 	}
	// }

	poweroff();


	while ( 1 );
}
