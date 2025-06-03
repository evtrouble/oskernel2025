#include "fs/file/file.hh"
#include "pm/ipc/pipe.hh"

using namespace pm::ipc;

namespace fs
{
	class pipe_file : public file
	{
	private:
		uint64 _off = 0;
		Pipe *_pipe;
		bool is_write;
	public:
		pipe_file( FileAttrs attrs, Pipe *pipe_,bool is_write ) : file( attrs ), _pipe( pipe_ ),is_write(is_write) { new ( &_stat ) Kstat( _pipe ); dup(); }
		pipe_file( Pipe *pipe_,bool is_write ) : file( FileAttrs( FileTypes::FT_PIPE, 0777 ) ), _pipe( pipe_ ),is_write(is_write) { new ( &_stat ) Kstat( _pipe ); dup(); }
		~pipe_file(){
			_pipe->close(is_write);
		};

		/// @note pipe read 没有偏移的概念
		long read( uint64 buf, size_t len, long off, bool upgrade ) override {
			refcnt = 1;
			return _pipe->read( buf, len );
		};

		/// @note pipe write 没有偏移的概念
		long write( uint64 buf, size_t len, long off, bool upgrade ) override { 
			refcnt = 1;
			return _pipe->write_in_kernel( buf, len ); 
		};
		

		int write_in_kernel( uint64 buf, size_t len ) { 
			refcnt = 1;
			return _pipe->write_in_kernel( buf, len ); 
		}

		virtual bool read_ready() override { return _pipe->read_is_open(); }
		virtual bool write_ready() override { return _pipe->write_is_open(); }
		virtual off_t lseek( off_t offset, int whence ) override { return -ESPIPE; }
	};
}