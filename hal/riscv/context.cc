#include "context.hh"

#include <process_interface.hh>

namespace riscv
{
	Context k_proc_context_pool[ hsai::proc_pool_size ];

	Context * proc_context_pool() { return k_proc_context_pool; }
} // namespace riscv
