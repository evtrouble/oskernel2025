#include "include/sdcard_driver.hh"
#include "include/dmac.hh"

#include <device_manager.hh>
#include <hsai_global.hh>
#include <mbr.hh>
#include <mem/virtual_memory.hh>
#include <memory_interface.hh>

#include "include/k210.hh"
#include "include/disk_driver.hh"
namespace riscv
{
	namespace k210
	{
		DiskDriver::DiskDriver( const char *lock_name )
		{
			_lock.init( lock_name );

			new ( &disk_ )
					SdcardDriver( 0 );
			// 注册到 HSAI
			hsai::k_devm.register_device( this, "Disk driver" );
		}

		int DiskDriver::handle_intr()
		{
			dmac_intr(DMAC_CHANNEL0);
			return 0;
		}
		
	} // namespace k210

} // namespace riscv