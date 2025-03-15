#include "include/virtio.hh"

#include <device_manager.hh>
#include <hsai_global.hh>
#include <mbr.hh>
#include <mem/virtual_memory.hh>
#include <memory_interface.hh>

#include "include/qemu.hh"
#include "include/disk_driver.hh"
namespace riscv
{
	namespace qemu
	{
		DiskDriver::DiskDriver( const char *lock_name )
		{
			_lock.init( lock_name );

			new ( &disk_ )
					VirtioDriver( (void *)VIRTIO0_V, 0 );

			// 注册到 HSAI
			hsai::k_devm.register_device( this, "Disk driver" );
		}

		int DiskDriver::handle_intr()
		{
			disk_.handle_intr();
			return 0;
		}
	} // namespace qemu

} // namespace riscv