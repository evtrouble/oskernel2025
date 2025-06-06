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
				// dentryCacheElementPool_[_index].pin_ = false;
				freeList_.push_back( &dentryCacheElementPool_[_index] );
			}
		}

		dentry* dentryCache::allocDentry()
		{
			_lock.acquire();
			if( freeList_.empty() )
			{
				log_panic( "孩子，内存真不够用了，dentryCache::alloDentry()分配失败\n" );
			}
			dentry* de = freeList_.front();
			freeList_.pop_front();
			_lock.release();
			return de;
		}

		void dentryCache::releaseDentryCache( dentry* de )
		{
			if(de != nullptr) {
				if ( de->isRoot() || de->isMntPoint() ) return;
				dentry* parent = de->getParent();
				if ( parent ) { 
					parent->children.erase( de->rName() ); 
					if(parent->children.empty() && parent->refcnt <= 0)
						releaseDentryCache( parent );
				}
				_lock.acquire();
				freeList_.push_back( de );
				_lock.release();
			}
		}


	} //  namespace dentrycache

} // namespace fs
