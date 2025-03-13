#ifndef QEMU
#include "include/sdcard_driver.hh"
#include "include/dmac.hh"
#else
#include "include/virtio.hh"
#endif 

#include <device_manager.hh>
#include <hsai_global.hh>
#include <mbr.hh>
#include <mem/virtual_memory.hh>
#include <memory_interface.hh>

#include "include/qemu_k210.hh"
#include "include/disk_driver.hh"
using namespace riscv::qemuk210;

DiskDriver::DiskDriver( const char *lock_name )
{
	_lock.init( lock_name );

    #ifdef QEMU
    new ( &disk_ )
			VirtioDriver( 0 );
    virtio_disk_init();
	#else 
    new ( &disk_ )
			SdcardDriver( 0 );
    #endif
	// 注册到 HSAI
	hsai::k_devm.register_device( this, "Disk driver" );
}

int DiskDriver::handle_intr()
{
	#ifdef QEMU
    disk_->handle_intr();
    #else 
    dmac_intr(DMAC_CHANNEL0);
    #endif

	return 0;
}

// void disk_read(struct buf *b)
// {
//     #ifdef QEMU
// 	virtio_disk_rw(b, 0);
//     #else 
// 	sdcard_read_sector(b->data, b->sectorno);
// 	#endif
// }

// void disk_write(struct buf *b)
// {
//     #ifdef QEMU
// 	virtio_disk_rw(b, 1);
//     #else 
// 	sdcard_write_sector(b->data, b->sectorno);
// 	#endif
// }
