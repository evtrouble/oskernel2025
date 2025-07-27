//
// Created by Li Shuang ( pseudonym ) on 2024-07-11 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#pragma once 
#include <EASTL/vector.h>

namespace hsai
{
	class VirtualInterruptManager __hsai_hal
	{
	public:
		virtual int handle_dev_intr() = 0;

		eastl::vector<int> _intr_nums;

		void increase_intr_count(int irq) { 
			if(irq >= (int)_intr_nums.size())
				_intr_nums.resize(irq + 1);
			_intr_nums[irq]++; 
		}

	public:
		static int register_interrupt_manager( VirtualInterruptManager * im );
	};

} // namespace hsai
