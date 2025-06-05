#include "fs/dentrycache.hh"

#include <EASTL/iterator.h>
#include <EASTL/list.h>
#include <EASTL/string.h>

#include "klib/common.hh"

namespace fs
{

	namespace dentrycache
	{
		dentryCache k_dentryCache;

		void dentryCache::init()
		{
			_lock.init( "dentryCache" );

			uint _index = 0;
			for ( ; _index < MAX_DENTRY_NUM; ++_index )
			{
				dentryCacheElementPool_[_index].pin_ = false;
				freeList_.push_back( &dentryCacheElementPool_[_index] );
			}
		}

		dentry* dentryCache::allocDentry()
		{
			_lock.acquire();
			dentry* dentry = nullptr;
			for ( auto it = freeList_.begin(); it != freeList_.end(); ++it )
			{
				// 还未分配
				if ( !( *it )->pin_ )
				{
					dentry		  = &( ( *it )->dentry );
					( *it )->pin_ = true;
					break;
				}
			}
			_lock.release();
			if ( !dentry ) log_panic( "孩子，内存真不够用了，dentryCache::alloDentry()分配失败\n" );
			return dentry;
		}

		void dentryCache::releaseDentryCache( dentry* de )
		{

			_lock.acquire();
			for ( auto it = freeList_.end(); it != freeList_.begin(); --it )
			{
				if ( ( &( *it )->dentry ) == de && ( *it )->pin_ )
				{
					( *it )->pin_  = false;
					dentry* parent = de->getParent();
					if ( parent ) { parent->children.erase( de->rName() ); }
					break;
				}
			}
			_lock.release();
		}


	} //  namespace dentrycache

} // namespace fs
