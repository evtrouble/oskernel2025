#pragma once


#include <smp/spin_lock.hh>
#include <virtual_device.hh>

#include "ahci_port_driver_ls.hh"

namespace riscv
{
	namespace qemu2k100 
    {
        class DiskDriver : public hsai::VirtualDevice
		{
        private:

			hsai::SpinLock	  _lock;


		public:

			virtual hsai::DeviceType type() override { return hsai::DeviceType::dev_other; }

			virtual bool read_ready() override { return false; }
			virtual bool write_ready() override { return true; }
			virtual int	 handle_intr() override;

		public:

            DiskDriver() = default;
            DiskDriver( const char *lock_name, void *base_addr );
        // private:
        //     void disk_init(void);
        //     void disk_read(struct buf *b);
        //     void disk_write(struct buf *b);
        //     void disk_intr(void);
        };
	} // namespace qemu2k100
}


