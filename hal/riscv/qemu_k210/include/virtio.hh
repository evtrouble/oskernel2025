#pragma once

#include <block_device.hh>
#include <disk_partition_device.hh>
#include <hsai_global.hh>
#include <hsai_log.hh>
#include <kernel/klib/function.hh>
#include <mem/virtual_memory.hh>
#include <smp/spin_lock.hh>

#include "include/qemu_k210.hh"
#include "include/gpiohs.hh"
#include "include/spi/spi.hh"
#include "hsai/include/mem/page.hh"
#include <kernel/types.hh>

//
// virtio device definitions.
// for both the mmio interface, and virtio descriptors.
// only tested with qemu.
// this is the "legacy" virtio interface.
//
// the virtio spec:
// https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf
//


namespace riscv
{
	namespace qemuk210
	{
    class VirtioDriver : public hsai::BlockDevice
    {            
      // virtio mmio control registers, mapped starting at 0x10001000.
      // from qemu virtio_mmio.h
      #define VIRTIO_MMIO_MAGIC_VALUE		0x000 // 0x74726976
      #define VIRTIO_MMIO_VERSION		0x004 // version; 1 is legacy
      #define VIRTIO_MMIO_DEVICE_ID		0x008 // device type; 1 is net, 2 is disk
      #define VIRTIO_MMIO_VENDOR_ID		0x00c // 0x554d4551
      #define VIRTIO_MMIO_DEVICE_FEATURES	0x010
      #define VIRTIO_MMIO_DRIVER_FEATURES	0x020
      #define VIRTIO_MMIO_GUEST_PAGE_SIZE	0x028 // page size for PFN, write-only
      #define VIRTIO_MMIO_QUEUE_SEL		0x030 // select queue, write-only
      #define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034 // max size of current queue, read-only
      #define VIRTIO_MMIO_QUEUE_NUM		0x038 // size of current queue, write-only
      #define VIRTIO_MMIO_QUEUE_ALIGN		0x03c // used ring alignment, write-only
      #define VIRTIO_MMIO_QUEUE_PFN		0x040 // physical page number for queue, read/write
      #define VIRTIO_MMIO_QUEUE_READY		0x044 // ready bit
      #define VIRTIO_MMIO_QUEUE_NOTIFY	0x050 // write-only
      #define VIRTIO_MMIO_INTERRUPT_STATUS	0x060 // read-only
      #define VIRTIO_MMIO_INTERRUPT_ACK	0x064 // write-only
      #define VIRTIO_MMIO_STATUS		0x070 // read/write

      // status register bits, from qemu virtio_config.h
      #define VIRTIO_CONFIG_S_ACKNOWLEDGE	1
      #define VIRTIO_CONFIG_S_DRIVER		2
      #define VIRTIO_CONFIG_S_DRIVER_OK	4
      #define VIRTIO_CONFIG_S_FEATURES_OK	8

      // device feature bits
      #define VIRTIO_BLK_F_RO              5	/* Disk is read-only */
      #define VIRTIO_BLK_F_SCSI            7	/* Supports scsi command passthru */
      #define VIRTIO_BLK_F_CONFIG_WCE     11	/* Writeback mode available in config */
      #define VIRTIO_BLK_F_MQ             12	/* support more than one vq */
      #define VIRTIO_F_ANY_LAYOUT         27
      #define VIRTIO_RING_F_INDIRECT_DESC 28
      #define VIRTIO_RING_F_EVENT_IDX     29

      // this many virtio descriptors.
      // must be a power of two.
      #define NUM 8

      struct VRingDesc {
        uint64 addr;
        uint32 len;
        uint16 flags;
        uint16 next;
      };
      #define VRING_DESC_F_NEXT  1 // chained with another descriptor
      #define VRING_DESC_F_WRITE 2 // device writes (vs read)

      struct VRingUsedElem {
        uint32 id;   // index of start of completed descriptor chain
        uint32 len;
      };

      // for disk ops
      #define VIRTIO_BLK_T_IN  0 // read the disk
      #define VIRTIO_BLK_T_OUT 1 // write the disk

      struct UsedArea {
        uint16 flags;
        uint16 id;
        struct VRingUsedElem elems[NUM];
      };

      // the address of virtio mmio register r.
      #define R(r) ((volatile uint32 *)(VIRTIO0_V + (r)))
      static constexpr int _block_size = 512;

      private:
        char _dev_name[8];

        int _port_id = 0;
        static struct disk {
          // memory for virtio descriptors &c for queue 0.
          // this is a global instead of allocated because it must
          // be multiple contiguous pages, which kalloc()
          // doesn't support, and page aligned.
          char pages[2*PG_SIZE];
          struct VRingDesc *desc;
          uint16 *avail;
          struct UsedArea *used;
           
          // our own book-keeping.
          char free[NUM];  // is a descriptor free?
          uint16 used_idx; // we've looked this far in used[2..NUM].
         
          // track info about in-flight operations,
          // for use when completion interrupt arrives.
          // indexed by first descriptor index of chain.
          struct {
            hsai::BufferDescriptor *b;
            char status;
          } info[NUM];
             
          hsai::SpinLock vdisk_lock;
             
        } __attribute__ ((aligned (PG_SIZE))) disk;

		  void virtio_disk_rw( long start_block, long block_count,
      hsai::BufferDescriptor *buf_list, int buf_count, bool write );
      void free_desc( int i );
      void free_chain( int i );
      int	alloc3_desc( int *idx );
      int	alloc_desc();

    public:
      virtual long get_block_size() override { return (long) _block_size; }
      virtual int  read_blocks_sync( long start_block, long block_count,
                    hsai::BufferDescriptor *buf_list, int buf_count ) override;
      virtual int read_blocks( long start_block, long block_count, hsai::BufferDescriptor *buf_list,
                  int buf_count ) override;
      virtual int write_blocks_sync( long start_block, long block_count,
                    hsai::BufferDescriptor *buf_list, int buf_count ) override;
      virtual int write_blocks( long start_block, long block_count,
                  hsai::BufferDescriptor *buf_list, int buf_count ) override;
      virtual int handle_intr() override;

      virtual bool read_ready() override {}
      virtual bool write_ready() override {}

    public:

      VirtioDriver() = default;
      VirtioDriver( int port_id );
		};
	} // namespace qemuk210
} // namespace riscv
