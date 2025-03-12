#ifndef QEMU
#include "include/sdcard.hh"
#include "include/dmac.hh"
#else
#include "include/virtio.hh"
#endif 

#include <device_manager.hh>
#include <hsai_global.hh>
#include <mbr.hh>
#include <mem/virtual_memory.hh>
#include <memory_interface.hh>

#include "qemu_k210.hh"

using namespace riscv;
using namespace riscv::qemuk210;

DiskDriver::DiskDriver( const char *lock_name, void *base_addr )
{
	_lock.init( lock_name );

    #ifdef QEMU
    virtio_disk_init();
	#else 
	sdcard_init();
    #endif
	// 注册到 HSAI
	hsai::k_devm.register_device( this, "Disk driver" );
}

int DiskDriver::handle_intr()
{
	#ifdef QEMU
    virtio_disk_intr();
    #else 
    dmac_intr(DMAC_CHANNEL0);
    #endif
	return 0;
}

void disk_read(struct buf *b)
{
    #ifdef QEMU
	virtio_disk_rw(b, 0);
    #else 
	sdcard_read_sector(b->data, b->sectorno);
	#endif
}

void disk_write(struct buf *b)
{
    #ifdef QEMU
	virtio_disk_rw(b, 1);
    #else 
	sdcard_write_sector(b->data, b->sectorno);
	#endif
}
