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
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_busybox_path[]	 = "busybox_testcode.sh";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_lua_path[]	 = "lua_testcode.sh";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_libctest_path[]	 = "libctest_testcode.sh";
__attribute__( ( section( ".user.init.data" ) ) ) const char echo[]	 = "echo";
__attribute__( ( section( ".user.init.data" ) ) ) const char libctest_start[]	 = "#### OS COMP TEST GROUP START libctest-musl ####\n";
__attribute__( ( section( ".user.init.data" ) ) ) const char libctest_end[]	 = "#### OS COMP TEST GROUP END libctest-musl ####\n";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_libctest_static_path[]	 = "run-static.sh";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_libctest_dynamic_path[]	 = "run-dynamic.sh";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_lmbench_path[]	 = "libcbench_testcode.sh";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_static[]	 = "run-static.sh";
__attribute__(( section( ".user.init.data" ) )) const char	 start_test_glibc_basic[] =
	"#### OS COMP TEST GROUP START basic-glibc ####\n";
__attribute__(( section( ".user.init.data" ) )) const char	 end_test_glibc_basic[] =
	"#### OS COMP TEST GROUP END basic-glibc ####\n";
__attribute__(( section( ".user.init.data" ) )) const char	 start_test_musl_basic[] =
	"#### OS COMP TEST GROUP START basic-musl ####\n";
__attribute__(( section( ".user.init.data" ) )) const char	 end_test_musl_basic[] =
	"#### OS COMP TEST GROUP END basic-musl ####\n";
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

__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_echo[]	 = "write";
__attribute__( ( section( ".user.init.data" ) ) ) const char exec_test_echo2[]	 = "/mnt/musl/basic/write";
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

// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_busybox_unstrp[] =
// 	"busybox_unstrp";
// __attribute__( ( section( ".user.init.data" ) ) ) const char busybox_name[]	 = "busybox";
// __attribute__( ( section( ".user.init.data" ) ) ) const char busybox_arg_c[] = "-c";
// __attribute__( ( section( ".user.init.data" ) ) ) const char busybox_arg_cmd[] =
// 	"'echo hello,busybox!'";
__attribute__( ( section( ".user.init.data" ) ) ) const char sh_name[]	 = "sh";
__attribute__( ( section( ".user.init.data" ) ) ) const char back_path[]	 = "..";
__attribute__( ( section( ".user.init.data" ) ) ) const char libctest_parm0[]	 = "-w";
__attribute__( ( section( ".user.init.data" ) ) ) const char libctest_parm1_static[]	 = "entry-static.exe";
__attribute__( ( section( ".user.init.data" ) ) ) const char libctest_parm1_dynamic[]	 = "entry-dynamic.exe";
__attribute__( ( section( ".user.init.data" ) ) ) const char libctest_parm2[]	 = "argv";
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
// __attribute__( ( section( ".user.init.data" ) ) ) const char test_glibc_path[] = "/mnt/glibc";
__attribute__(( section( ".user.init.data.p" ) )) const char *bb_sh[8]		 = { 0 };
__attribute__(( section( ".user.init.data" ) )) const char	  busybox_path[] = "busybox";
__attribute__(( section( ".user.init.data" ) )) const char	  runtest_path[] = "./runtest.exe";
__attribute__(( section( ".user.init.data" ) )) const char	  ld_path[] = "/mnt/musl/lib/libc.so";
// __attribute__(( section( ".user.init.data.p" ) )) const char	busybox_path[]		 = "busybox";

// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_libcbench[] = "libc-bench";

// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench[]		 = "lmbench_all";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench_arg1[] = "lat_syscall";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench_arg2[] = "-P";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench_arg3[] = "1";
// __attribute__( ( section( ".user.init.data" ) ) ) const char exec_lmbench_arg4[] = "read";

// __attribute__(( section( ".user.init.data" ) )) const char rootpath[] = "basic/";

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

#define RUN_TESTS(test_list, param) do { \
    pid = fork(); \
    if(pid < 0) { \
      write(1, errstr, sizeof(errstr)); \
    } else if(pid == 0) { \
      if(execve(test_list, param, 0) < 0) { \
        write(1, exec_fail_str, sizeof(exec_fail_str)); \
      } \
      exit(0); \
    } else { \
      int child_exit_state = -100; \
      if(wait(-1, &child_exit_state) < 0) \
        write(1, wait_fail, sizeof(wait_fail)); \
    } \
} while(0)

int			basic_test( void ) __attribute__( ( section( ".user.init" ) ) );
int			test_busybox( void ) __attribute__( ( section( ".user.init" ) ) );
int			test_lua( void ) __attribute__( ( section( ".user.init" ) ) );
int			test_libctest( void ) __attribute__( ( section( ".user.init" ) ) );
int			test_lmbench( void ) __attribute__( ( section( ".user.init" ) ) );
int			test_local( void ) __attribute__( ( section( ".user.init" ) ) );

int			test_busybox( void ) 
{
	__attribute__(( __unused__ )) int pid;
	bb_sh[0] = sh_name;
	bb_sh[1] = exec_test_busybox_path;
	bb_sh[2] = 0;
	RUN_TESTS( busybox_path, bb_sh );
}

int           test_lua(void)
{	
	__attribute__(( __unused__ )) int pid;
	bb_sh[0] = sh_name;
	bb_sh[1] = exec_test_lua_path;
	bb_sh[2] = 0;
	RUN_TESTS( busybox_path, bb_sh );

}

int           test_lmbench(void)
{	
	__attribute__(( __unused__ )) int pid;
	bb_sh[0] = sh_name;
	bb_sh[1] = exec_test_lmbench_path;
	bb_sh[2] = 0;
	RUN_TESTS( busybox_path, bb_sh );

}

int			basic_test( void ) 
{
	__attribute__(( __unused__ )) int pid;
	RUN_TESTS( exec_test_echo, 0 );
	RUN_TESTS( exec_test_fork, 0 );
	RUN_TESTS( exec_test_exit, 0 );
	RUN_TESTS( exec_test_wait, 0 );
	RUN_TESTS( exec_test_getpid, 0 );
	RUN_TESTS( exec_test_getppid, 0 );
	RUN_TESTS( exec_test_dup, 0 );
	RUN_TESTS( exec_test_dup2, 0 );
	RUN_TESTS( exec_test_execve, 0 );
	RUN_TESTS( exec_test_getcwd, 0 );
	RUN_TESTS( exec_test_gettimeofday, 0 );
	RUN_TESTS( exec_test_yield, 0 );
	RUN_TESTS( exec_test_sleep, 0 );
	RUN_TESTS( exec_test_times, 0 );
	RUN_TESTS( exec_test_clone, 0 );
	RUN_TESTS( exec_test_brk, 0 );
	RUN_TESTS( exec_test_waitpid, 0 );
	RUN_TESTS( exec_test_mmap, 0 );
	RUN_TESTS( exec_test_fstat, 0 );
	RUN_TESTS( exec_test_uname, 0 );
	RUN_TESTS( exec_test_openat, 0 );
	RUN_TESTS( exec_test_open, 0 );
	RUN_TESTS( exec_test_close, 0 );
	RUN_TESTS( exec_test_read, 0 );
	RUN_TESTS( exec_test_getdents, 0 );
	RUN_TESTS( exec_test_mkdir, 0 );
	RUN_TESTS( exec_test_chdir, 0 );
	RUN_TESTS( exec_test_mount, 0 );
	RUN_TESTS( exec_test_umount, 0 );
	RUN_TESTS( exec_test_munmap, 0 );
	RUN_TESTS( exec_test_unlinkat, 0 );
	RUN_TESTS( exec_test_pipe, 0 );
}

// libctest 测试程序名称数组
__attribute__((section(".user.init.data"))) const char libctest_static_programs[][32] = {
    "argv",
    "basename",
    "clocale_mbfuncs",
    "clock_gettime",
    "dirname",
    "env",
    "fdopen",
    "fnmatch",
    "fscanf",
    "fwscanf",
    "iconv_open",
    "inet_pton",
    "mbc",
    "memstream",
    // "pthread_cancel_points",
    // "pthread_cancel",
    // "pthread_cond",
    // "pthread_tsd",
    "qsort",
    "random",
    "search_hsearch",
    "search_insque",
    "search_lsearch",
    "search_tsearch",
    "setjmp",
    "snprintf",
    "socket",
    "sscanf",
    "sscanf_long",
    "stat",
    "strftime",
    "string",
    "string_memcpy",
    "string_memmem",
    "string_memset",
    "string_strchr",
    "string_strcspn",
    "string_strstr",
    "strptime",
    "strtod",
    "strtod_simple",
    "strtof",
    "strtol",
    "strtold",
    "swprintf",
    "tgmath",
    "time",
    "tls_align",
    "udiv",
    "ungetc",
    "utime",
    "wcsstr",
    "wcstol",
    "daemon_failure",
    "dn_expand_empty",
    "dn_expand_ptr_0",
    "fflush_exit",
    "fgets_eof",
    "fgetwc_buffering",
    "fpclassify_invalid_ld80",
    "ftello_unflushed_append",
    "getpwnam_r_crash",
    "getpwnam_r_errno",
    "iconv_roundtrips",
    "inet_ntop_v4mapped",
    "inet_pton_empty_last_field",
    "iswspace_null",
    "lrand48_signextend",
    "lseek_large",
    "malloc_0",
    "mbsrtowcs_overflow",
    "memmem_oob_read",
    "memmem_oob",
    "mkdtemp_failure",
    "mkstemp_failure",
    "printf_1e9_oob",
    "printf_fmt_g_round",
    "printf_fmt_g_zeros",
    "printf_fmt_n",
    // "pthread_robust_detach",
    // "pthread_cancel_sem_wait",
    // "pthread_cond_smasher",
    // "pthread_condattr_setclock",
    // "pthread_exit_cancel",
    // "pthread_once_deadlock",
    // "pthread_rwlock_ebusy",
    "putenv_doublefree",
    "regex_backref_0",
    "regex_bracket_icase",
    "regex_ere_backref",
    "regex_escaped_high_byte",
    "regex_negated_range",
    "regexec_nosub",
    "rewind_clear_error",
    "rlimit_open_files",
    "scanf_bytes_consumed",
    "scanf_match_literal_eof",
    "scanf_nullbyte_char",
    "setvbuf_unget",
    "sigprocmask_internal",
    "sscanf_eof",
    "statvfs",
    "strverscmp",
    "syscall_sign_extend",
    "uselocale_0",
    "wcsncpy_read_overflow",
    "wcsstr_false_negative"
};
__attribute__((section(".user.init.data"))) const char libctest_dynamic_programs[][32] = {
    "argv",
    "basename",
    "clocale_mbfuncs",
    "clock_gettime",
    "dirname",
    "env",
    "fdopen",
    "fnmatch",
    "fscanf",
    "fwscanf",
    "iconv_open",
    "inet_pton",
    "mbc",
    "memstream",
    // "pthread_cancel_points",
    // "pthread_cancel",
    // "pthread_cond",
    // "pthread_tsd",
    "qsort",
    "random",
    "search_hsearch",
    "search_insque",
    "search_lsearch",
    "search_tsearch",
    "setjmp",
    "snprintf",
    "socket",
    "sscanf",
    "sscanf_long",
    "stat",
    "strftime",
    "string",
    "string_memcpy",
    "string_memmem",
    "string_memset",
    "string_strchr",
    "string_strcspn",
    "string_strstr",
    "strptime",
    "strtod",
    "strtod_simple",
    "strtof",
    "strtol",
    "strtold",
    "swprintf",
    "tgmath",
    "time",
    "tls_align",
    "udiv",
    "ungetc",
    "utime",
    "wcsstr",
    "wcstol",
    // "daemon_failure",
    "dn_expand_empty",
    "dn_expand_ptr_0",
    // "fflush_exit",
    "fgets_eof",
    "fgetwc_buffering",
    "fpclassify_invalid_ld80",
    "ftello_unflushed_append",
    "getpwnam_r_crash",
    "getpwnam_r_errno",
    "iconv_roundtrips",
    "inet_ntop_v4mapped",
    "inet_pton_empty_last_field",
    "iswspace_null",
    "lrand48_signextend",
    "lseek_large",
    "malloc_0",
    "mbsrtowcs_overflow",
    "memmem_oob_read",
    "memmem_oob",
    "mkdtemp_failure",
    "mkstemp_failure",
    "printf_1e9_oob",
    "printf_fmt_g_round",
    "printf_fmt_g_zeros",
    "printf_fmt_n",
    // "pthread_robust_detach",
    // "pthread_cancel_sem_wait",
    // "pthread_cond_smasher",
    // "pthread_condattr_setclock",
    // "pthread_exit_cancel",
    // "pthread_once_deadlock",
    // "pthread_rwlock_ebusy",
    "putenv_doublefree",
    "regex_backref_0",
    "regex_bracket_icase",
    "regex_ere_backref",
    "regex_escaped_high_byte",
    "regex_negated_range",
    "regexec_nosub",
    "rewind_clear_error",
    "rlimit_open_files",
    "scanf_bytes_consumed",
    "scanf_match_literal_eof",
    "scanf_nullbyte_char",
    "setvbuf_unget",
    "sigprocmask_internal",
    "sscanf_eof",
    "statvfs",
    "strverscmp",
    "syscall_sign_extend",
    "uselocale_0",
    "wcsncpy_read_overflow",
    "wcsstr_false_negative"
};
int           test_libctest(void)
{	
	__attribute__(( __unused__ )) int pid;
  write( 1, libctest_start, sizeof( libctest_start ) );
	//static部分
	int array_size = sizeof(libctest_static_programs) / sizeof(libctest_static_programs[0]);
	bb_sh[0] = runtest_path;
	bb_sh[1] = libctest_parm0;
	bb_sh[2] = libctest_parm1_static;
	bb_sh[3] = libctest_parm2;
	bb_sh[4] = 0;
	for(int i = 0; i < array_size; i++) {
		bb_sh[3]= libctest_static_programs[i];
        RUN_TESTS(runtest_path, bb_sh);
  }
	//动态部分
  array_size = sizeof(libctest_dynamic_programs) / sizeof(libctest_dynamic_programs[0]);
  bb_sh[2] = libctest_parm1_dynamic;
  for(int i = 0; i < array_size; i++) {
		bb_sh[3]= libctest_dynamic_programs[i];
        RUN_TESTS(runtest_path, bb_sh);
  }
  write( 1, libctest_end, sizeof( libctest_end ) );
}

int           test_local(void)
{	
	__attribute__(( __unused__ )) int pid;
  chdir(back_path);
	bb_sh[0] = runtest_path;
	bb_sh[1] = libctest_parm0;
	bb_sh[2] = libctest_parm1_static;
	bb_sh[3] = libctest_parm2;
  bb_sh[4] = 0;
  RUN_TESTS(runtest_path, bb_sh);
    // bb_sh[0] = exec_test_echo;
    // bb_sh[1] = 0;
  	// RUN_TESTS( exec_test_echo, bb_sh );


}

int init_main( void )
{


    chdir( test_musl_basic_path );
    //basic测试
    write( 1, start_test_musl_basic, sizeof( start_test_musl_basic ) );
    basic_test();
    write( 1, end_test_musl_basic, sizeof( end_test_musl_basic ) );
    // 回到musl目录
    chdir( back_path );
    //lua测试
    test_lua();
    //busybox测试
    test_busybox();
    // libctest测试
    test_libctest();
  #ifdef __riscv
    //lmbench
    test_lmbench();
  #endif


    chdir( test_glibc_basic_path );
    //basic测试
    write( 1, start_test_glibc_basic, sizeof( start_test_glibc_basic ) );
    basic_test();
    write( 1, end_test_glibc_basic, sizeof( end_test_glibc_basic ) );
    //回到 glibc目录
    chdir( back_path );
    //lua测试
    test_lua();
    // busybox测试
    test_busybox();
  #ifdef __riscv
    //lmbench
    test_lmbench();
  #endif
    poweroff();



	while ( 1 );
}
