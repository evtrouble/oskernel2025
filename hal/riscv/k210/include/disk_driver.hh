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
			void identify_device();
			
		private:
			enum
			{
				mbr_none   = 0,
				mbr_chk_ok = 1,
				mbr_gpt	   = -10,
				mbr_fat32  = 2,
				mbr_ext	   = 3
			};

			int	_check_mbr_partition( u8 *mbr );

		public:
            DiskDriver() = default;
            DiskDriver( const char *lock_name );
        };

	} // namespace k210
	
} // namespace riscv