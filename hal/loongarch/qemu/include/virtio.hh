#pragma once

#include <block_device.hh>
#include <disk_partition_device.hh>
#include <hsai_global.hh>
#include <hsai_log.hh>
#include <kernel/klib/function.hh>
#include <mem/virtual_memory.hh>
#include <smp/spin_lock.hh>

#include <mem/page.hh>
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
namespace loongarch
{
	namespace qemu
	{
    // // PCI设备结构体
    // struct pci_device {
    //     uint8_t bus;      // 总线号
    //     uint8_t device;   // 设备号
    //     uint8_t function; // 功能号
    //     uint16_t vendor_id;  // 厂商ID
    //     uint16_t device_id;  // 设备ID
    // };  // PCI设备句柄;

		class VirtioDriver : public hsai::BlockDevice
		{
			friend class DiskDriver;
// virtio PCI common configuration
#define VIRTIO_PCI_HOST_FEATURES        0  // host/device features
#define VIRTIO_PCI_GUEST_FEATURES       4  // guest features
#define VIRTIO_PCI_QUEUE_PFN            8  // physical page number for queue
#define VIRTIO_PCI_QUEUE_NUM           12  // number of ring entries
#define VIRTIO_PCI_QUEUE_SEL           14  // queue select
#define VIRTIO_PCI_QUEUE_NOTIFY        16  // queue notify
#define VIRTIO_PCI_STATUS              18  // device status register
#define VIRTIO_PCI_ISR                 19  // interrupt status register
#define VIRTIO_PCI_CONFIG              20  // configuration vector
#define VIRTIO_PCI_QUEUE_NUM_MAX       0x20

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE 1
#define VIRTIO_CONFIG_S_DRIVER		2
#define VIRTIO_CONFIG_S_DRIVER_OK	4
#define VIRTIO_CONFIG_S_FEATURES_OK 8

// device feature bits
#define VIRTIO_BLK_F_RO				5  /* Disk is read-only */
#define VIRTIO_BLK_F_SCSI			7  /* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE		11 /* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ				12 /* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT			27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX		29

// this many virtio descriptors.
// must be a power of two.
#define NUM 8

			struct VRingDesc
			{
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
      volatile uint64 _pci_dev;
      volatile uint64 _reg;
      static constexpr int _block_size = 512;

      private:
        char _dev_name[8];
        char _partition_name[4][8];
        hsai::DiskPartitionDevice _disk_partition[4]; // MBR 硬盘只支持最多4个分区

        int		   _port_id = 0;
        struct Disk {
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
            bool wait;
          } info[NUM];
             
          hsai::SpinLock vdisk_lock;
             
        } __attribute__ ((aligned (PG_SIZE))) disk;

        void virtio_disk_rw( long start_block, long block_count,
        hsai::BufferDescriptor *buf_list, int buf_count, bool write );
        void free_desc( int i );
        void free_chain( int i );
        int	alloc3_desc( int *idx );
        int	alloc_desc();
        void init_intr();
        void deal_msix(int msix_cap_ptr);
        void deal_msi(int msi_cap_ptr);

        // PCI中断相关标志
        static constexpr uint8_t VIRTIO_PCI_ISR_INTR = 0x1;    // 数据中断
        static constexpr uint8_t VIRTIO_PCI_ISR_CONFIG = 0x2;  // 配置改变中断

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

		virtual bool read_ready() override {
          // 检查设备状态
          // uint32 status = *R(VIRTIO_MMIO_STATUS);
          // if ((status & VIRTIO_CONFIG_S_DRIVER_OK) == 0) {
          //   return false;  // 设备未就绪
          // }
          return true;
        }
        virtual bool write_ready() override {
          // 检查设备状态
          // uint32 status = *R(VIRTIO_MMIO_STATUS);
          // if ((status & VIRTIO_CONFIG_S_DRIVER_OK) == 0) {
          //   return false;  // 设备未就绪
          // }

          // // 检查设备是否只读
          // uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
          // if (features & (1 << VIRTIO_BLK_F_RO)) {
          //   return false;  // 设备只读
          // }
          return true;
        }

	    public:
        VirtioDriver() = default;
		    VirtioDriver(uint8_t bus, uint8_t device, uint8_t function, int port_id);
		};

// PCI配置空间寄存器偏移
#define PCI_VENDOR_ID           0x00    // 厂商ID寄存器
#define PCI_DEVICE_ID          0x02    // 设备ID寄存器
#define PCI_COMMAND            0x04    // 命令寄存器
#define PCI_STATUS             0x06    // 状态寄存器
#define PCI_REVISION_ID        0x08    // 版本ID寄存器
#define PCI_CLASS_PROG         0x09    // 编程接口寄存器
#define PCI_CLASS_DEVICE       0x0a    // 设备类寄存器
#define PCI_HEADER_TYPE        0x0e    // 头类型寄存器
#define PCI_BASE_ADDRESS_0     0x10    // BAR0基地址寄存器
#define PCI_INTERRUPT_LINE     0x3c    // 中断线寄存器
#define PCI_INTERRUPT_PIN      0x3d    // 中断引脚寄存器
#define PCI_CAPABILITIES_POINTER 0x34   // 能力列表指针偏移

// PCI命令寄存器位定义
#define PCI_COMMAND_IO         0x1     // I/O空间访问使能
#define PCI_COMMAND_MEMORY    0x2     // 内存空间访问使能
#define PCI_COMMAND_MASTER    0x4     // 总线主控使能
#define PCI_COMMAND_SPECIAL   0x8     // 特殊周期使能
#define PCI_COMMAND_INVALIDATE 0x10    // 内存写与使能位
#define PCI_COMMAND_VGA_PALETTE 0x20   // VGA调色板窥探
#define PCI_COMMAND_PARITY    0x40    // 奇偶校验错误应答使能
#define PCI_COMMAND_WAIT      0x80    // SERR#使能
#define PCI_COMMAND_SERR      0x100   // 快速返回使能
#define PCI_CAP_ID_MSI            0x05  // MSI能力ID
#define PCI_CAP_ID_MSIX           0x11  // MSI-X能力ID
#define MSIX_CONTROL                  0x02
#define MSIX_TABLE_PTR                0x04
#define MSIX_PBA_PTR                  0x08
#define MSIX_CONTROL_ENABLE           0x8000
#define MSIX_CONTROL_MASKALL          0x4000

// PCI厂商ID
#define PCI_VENDOR_ID_REDHAT_QUMRANET  0x1AF4   // Red Hat的virtio设备厂商ID

// PCI设备类型
#define PCI_CLASS_STORAGE_EXPRESS   0x01    // 存储设备类
#define PCI_CLASS_NETWORK          0x02    // 网络设备类
#define PCI_CLASS_DISPLAY          0x03    // 显示设备类
#define PCI_CLASS_MULTIMEDIA       0x04    // 多媒体设备类
#define PCI_CLASS_MEMORY           0x05    // 内存控制器类
#define PCI_CLASS_BRIDGE           0x06    // 桥设备类
#define PCI_CLASS_COMMUNICATION    0x07    // 通信设备类
#define PCI_CLASS_SYSTEM           0x08    // 系统设备类
#define PCI_CLASS_INPUT            0x09    // 输入设备类
#define PCI_CLASS_DOCKING          0x0a    // 扩展坞类
#define PCI_CLASS_PROCESSOR        0x0b    // 处理器类
#define PCI_CLASS_SERIAL           0x0c    // 串行总线类
#define PCI_CLASS_WIRELESS         0x0d    // 无线控制器类
#define PCI_CLASS_SATELLITE        0x0f    // 卫星通信类
#define PCI_CLASS_OTHERS           0xff    // 其他设备类

	} // namespace qemu

} // namespace loongarch