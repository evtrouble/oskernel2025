#pragma once
#include <smp/spin_lock.hh>
#include "sdcard_driver.hh"
#include <virtual_device.hh>

namespace riscv
{
	namespace k210 
    {
        class DiskDriver : public hsai::VirtualDevice
		{
        private:

			hsai::SpinLock	  _lock;
			SdcardDriver disk_;

		public:

			virtual hsai::DeviceType type() override { return hsai::DeviceType::dev_other; }

			virtual bool read_ready() override { return false; }
			virtual bool write_ready() override { return true; }
			virtual int	 handle_intr() override;

		public:
            DiskDriver() = default;
            DiskDriver( const char *lock_name );
        };

	} // namespace k210
	
} // namespace riscv