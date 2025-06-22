#pragma once
#include <EASTL/list.h>
#include <EASTL/string.h>
#include <EASTL/tuple.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <smp/spin_lock.hh>

#include "fs/dentry.hh"
#include "types.hh"

using eastl::list;
using eastl::string;
using eastl::tuple;
using eastl::unique_ptr;
using eastl::vector;

namespace fs
{
	class dentry;
	namespace dentrycache
	{
		constexpr uint MAX_DENTRY_NUM =4096 ;
		/**
		 * @brief Dentry cache
		 * @test dentryCacheTest
		 */

		// struct dentryCacheElement
		// {
		// 	fs::dentry dentry;
		// 	bool	   pin_;
		// 	dentryCacheElement() { pin_ = false; };
		// 	dentryCacheElement( fs::dentry den, bool pin_ ) : dentry( den ), pin_( pin_ ) {}
		// };

		class dentryCache
		{
			hsai::SpinLock			   _lock;
			list<fs::dentry *> freeList_; // leaf list
			fs::dentry		   dentryCacheElementPool_[MAX_DENTRY_NUM];

		public:

			dentryCache()  = default;
			~dentryCache() = default;

			void	init();
			dentry *allocDentry();
			void    releaseDentryCache( dentry *de );
		};

		extern dentryCache k_dentryCache;
	} // namespace dentrycache
} // namespace fs
