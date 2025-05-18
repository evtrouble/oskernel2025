//
// Created by Li Shuang ( pseudonym ) on 2024-05-23 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#include "fs/dev/acpi_controller.hh"
#include "klib/klib.hh"

namespace dev
{
	namespace acpi
	{
		AcpiController k_acpi_controller;

		void AcpiController::init( const char * lock_name, uint64 acpi_reg_base )
		{
			_lock.init( lock_name );
			_reg_base = ( void * ) acpi_reg_base;
		}

		void AcpiController::power_off()
		{
			assert( _reg_base != nullptr, "ACPI not init" );
			*(volatile uint8 *)(_reg_base) = 0x34;
		}

	} // namespace acpi

} // namespace fs
