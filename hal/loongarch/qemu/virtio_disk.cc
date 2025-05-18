//
// driver for qemu's virtio disk device.
// uses qemu's mmio interface to virtio.
// qemu presents a "legacy" virtio interface.
//
// qemu ... -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
//


#include "include/virtio.hh"
#include <device_manager.hh>
#include <process_interface.hh>
#include <klib/klib.hh>
#include <mm/page_table.hh>
#include "la_cpu.hh"
#include "include/qemu.hh"
#include "disk_driver.hh"
#include "include/interrupt_manager.hh"

static void		_handle_intr() { loongarch::qemu::k_im.handle_dev_intr(); }

namespace loongarch
{
	namespace qemu
	{
    // PCI配置空间基地址 (LoongArch特定)
	  static constexpr uint64_t LA_PCI_CONFIG_BASE = 0x2000'0000UL | loongarch::win_1;

	  // PCI配置空间访问的底层实现
    static void pci_config_write(uint64 pdev, int reg, uint32_t value, int size) {
        uint64 cfg_addr = pdev + (reg & 0xFC);
        switch(size) {
            case 1:
                *(volatile u8*)cfg_addr = value;
                break;
            case 2:
                *(volatile uint16_t*)cfg_addr = value;
                break;
            case 4:
                *(volatile uint32_t*)cfg_addr = value;
                break;
        }
    }

    static uint32_t pci_config_read(uint64 pdev, int reg, int size) {
        uint64 cfg_addr = pdev + (reg & 0xFC);
        switch(size) {
            case 1:
                return *(volatile u8*)cfg_addr;
            case 2:
                return *(volatile uint16_t*)cfg_addr;
            case 4:
                return *(volatile uint32_t*)cfg_addr;
            default:
                return 0;
        }
    }

    // API函数实现
    void API_PciDevConfigWriteByte(uint64 pdev, int reg, u8 value) {
        pci_config_write(pdev, reg, value, 1);
    }

    void API_PciDevConfigWriteWord(uint64 pdev, int reg, uint16_t value) {
        pci_config_write(pdev, reg, value, 2);
    }

    void API_PciDevConfigWriteDword(uint64 pdev, int reg, uint32_t value) {
        pci_config_write(pdev, reg, value, 4);
    }

    uint8_t API_PciDevConfigReadByte(uint64 pdev, int reg) {
        return pci_config_read(pdev, reg, 1);
    }

    uint16_t API_PciDevConfigReadWord(uint64 pdev, int reg) {
        return pci_config_read(pdev, reg, 2);
    }

    uint32_t API_PciDevConfigReadDword(uint64 pdev, int reg) {
        return pci_config_read(pdev, reg, 4);
    }

    VirtioDriver::VirtioDriver(uint8_t bus, uint8_t device, uint8_t function, int port_id)
    {
      _pci_dev = LA_PCI_CONFIG_BASE + (( bus << 16 ) | ( device << 11 ) |
                        ( function << 8 ));

      uint32 status = 0;
      _port_id	  = port_id;

      disk.vdisk_lock.init( "virtio_disk" );

      // Check PCI vendor and device ID
      // 读取设备ID和厂商ID
      uint32_t id_reg = API_PciDevConfigReadDword( _pci_dev, PCI_VENDOR_ID );
      uint16_t vendor_id = id_reg & 0xFFFF;
      uint16_t device_id = ( id_reg >> 16 ) & 0xFFFF;
	  hsai_printf( "vendor_id:%d,device_id:%d", vendor_id, device_id );

	  if ( vendor_id != PCI_VENDOR_ID_REDHAT_QUMRANET || device_id < 0x1000 ||
        device_id > 0x103f )
      {
        hsai_panic( "could not find virtio disk" );
      }
      
      // Enable PCI bus mastering
      uint16_t cmd = API_PciDevConfigReadWord(_pci_dev, PCI_COMMAND);
      API_PciDevConfigWriteWord(_pci_dev, PCI_COMMAND, cmd | PCI_COMMAND_MASTER | PCI_COMMAND_IO | PCI_COMMAND_MEMORY);

      _reg = API_PciDevConfigReadDword(_pci_dev, PCI_BASE_ADDRESS_0);
      // 清除低4位（BAR标志位），获取实际基地址
      _reg &= ~0xF;
	    _reg |= loongarch::win_1;

	    // Set device status
      // Virtio设备初始化
      // 1 重置设备
      API_PciDevConfigWriteByte(_reg, VIRTIO_PCI_STATUS, status);
      // 2 设置状态：已识别设备
      status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
      API_PciDevConfigWriteByte(_reg, VIRTIO_PCI_STATUS, status);

      // 3 设置状态：驱动已加载
      status |= VIRTIO_CONFIG_S_DRIVER;
      API_PciDevConfigWriteByte( _reg, VIRTIO_PCI_STATUS, status );

      // 特性协商
      // Negotiate features
      uint32_t features_lo = API_PciDevConfigReadDword( _reg, VIRTIO_PCI_HOST_FEATURES );
      uint64	 features	 = features_lo;

      features &= ~( 1 << VIRTIO_BLK_F_RO );
      features &= ~( 1 << VIRTIO_BLK_F_SCSI );
      features &= ~( 1 << VIRTIO_BLK_F_CONFIG_WCE );
      features &= ~( 1 << VIRTIO_BLK_F_MQ ); // 禁用多队列
      features &= ~( 1 << VIRTIO_F_ANY_LAYOUT );
      features &= ~( 1 << VIRTIO_RING_F_EVENT_IDX );
      features &= ~( 1 << VIRTIO_RING_F_INDIRECT_DESC );

      API_PciDevConfigWriteDword( _reg, VIRTIO_PCI_GUEST_FEATURES, features & 0xffffffff );

      // Complete feature negotiation
      status |= VIRTIO_CONFIG_S_FEATURES_OK;
      API_PciDevConfigWriteByte( _reg, VIRTIO_PCI_STATUS, status );

      // Verify the FEATURES_OK bit was accepted
      uint8_t device_status = API_PciDevConfigReadByte( _reg, VIRTIO_PCI_STATUS );
      if ( !( device_status & VIRTIO_CONFIG_S_FEATURES_OK ) )
        hsai_panic( "Device did not accept features" );

      // Tell device we're ready
      status |= VIRTIO_CONFIG_S_DRIVER_OK;
      API_PciDevConfigWriteByte( _reg, VIRTIO_PCI_STATUS, status );

      // 读取中断引脚和中断线
	    // init_intr();

      // Initialize queue 0
      API_PciDevConfigWriteWord( _pci_dev, VIRTIO_PCI_QUEUE_SEL, 0 );
      uint16_t qsize = API_PciDevConfigReadWord( _pci_dev, VIRTIO_PCI_QUEUE_NUM_MAX );

      if ( qsize == 0 ) hsai_panic( "virtio disk has no queue 0" );
      if ( qsize < NUM ) hsai_panic( "virtio disk max queue too short" );

      memset( disk.pages, 0, sizeof( disk.pages ) );

      // Setup the queue
      uint32_t pfn = ( (uint64) disk.pages ) >> 12;
      API_PciDevConfigWriteDword( _reg, VIRTIO_PCI_QUEUE_PFN, pfn );

      // desc = pages -- num * VRingDesc
      // avail = pages + 0x40 -- 2 * uint16, then num * uint16
      // used = pages + 4096 -- 2 * uint16, then num * vRingUsedElem

      disk.desc  = (VRingDesc *) disk.pages;
      disk.avail = (uint16 *) ( ( (char *) disk.desc ) + NUM * sizeof( VRingDesc ) );
      disk.used  = (UsedArea *) ( disk.pages + PG_SIZE );

      for ( int i = 0; i < NUM; i++ ) disk.free[i] = 1;

      static char _default_dev_name[] = "hd?";
      for ( ulong i = 0; i < sizeof _default_dev_name; ++i ) _dev_name[i] = _default_dev_name[i];
      _dev_name[2] = 'a' + (char) _port_id;
      _dev_name[3] = '\0';
      for ( auto &pname : _partition_name )
      {
        for ( ulong i = 0; i < sizeof _dev_name; ++i ) pname[i] = _dev_name[i];

        pname[3] = 'p';
        pname[4] = '?';
        pname[5] = 0;
      }
      hsai::k_devm.register_block_device( this, _dev_name );
    }

    void VirtioDriver::init_intr()
    {
      uint8_t cap_ptr = API_PciDevConfigReadByte(_pci_dev, PCI_CAPABILITIES_POINTER);

      // 遍历 PCI 能力列表查找 MSI/MSI-X 能力
      while (cap_ptr != 0) {
          uint8_t cap_id = API_PciDevConfigReadByte(_pci_dev, cap_ptr);
          if(cap_id == PCI_CAP_ID_MSI) {
            deal_msi( cap_ptr );
            // 解析 MSI 配置...
            break;
          }
          else if(cap_id == PCI_CAP_ID_MSIX)
          {
            deal_msix( cap_ptr );
            break;
          }
          cap_ptr = API_PciDevConfigReadByte(_pci_dev, cap_ptr + 1); // 下一个能力指针
      }
      if(cap_ptr == 0)
        hsai_panic( "virtio-blk-pci don't support interruption!\n" );
    }

    void VirtioDriver::deal_msix(int msix_cap_ptr)
    {
      // 获取 MSI-X 表地址
      uint32_t msix_table_ptr = API_PciDevConfigReadDword(_pci_dev, msix_cap_ptr + MSIX_TABLE_PTR);
      uint8_t msix_table_bar = msix_table_ptr & 0x7;
      uint32_t msix_table_offset = (msix_table_ptr >> 3) << 3;
      uint32_t table_bar_base = API_PciDevConfigReadDword(_pci_dev, PCI_BASE_ADDRESS_0 + (msix_table_bar * 4));
      uint32_t msix_table_base = (table_bar_base & ~0xF) + msix_table_offset;
uint32_t entry_addr = msix_table_base;
	  uint32 tmp = API_PciDevConfigReadDword( _pci_dev, entry_addr + 8 );
	  hsai_printf( "tmp:%d\n", tmp );

	  // 分配单个中断向量
      uint32_t irq_vector = DISK_IRQ;

      // 配置队列0的MSI-X表项（假设队列0对应表项0）
      // uint32_t entry_addr = msix_table_base;
      uint64 handle_addr = (uint64) &_handle_intr;
      API_PciDevConfigWriteDword( _pci_dev, entry_addr, handle_addr & 0xffffffff ); // 低32位地址
      API_PciDevConfigWriteDword(_pci_dev, entry_addr + 4, handle_addr >> 32); // 高32位地址
      API_PciDevConfigWriteDword(_pci_dev, entry_addr + 8, irq_vector | 
                                  0x4000); // 数据（中断向量 + 标志）

      // 启用 MSI-X
      uint16_t msix_control = API_PciDevConfigReadWord(_pci_dev, msix_cap_ptr + MSIX_CONTROL);
      msix_control &= ~MSIX_CONTROL_MASKALL;
      msix_control |= MSIX_CONTROL_ENABLE;
      API_PciDevConfigWriteWord(_pci_dev, msix_cap_ptr + MSIX_CONTROL, msix_control);

      // 注册中断处理函数
      // register_interrupt_handler(irq_vector, virtio_disk_interrupt_handler, disk);
    }

    void VirtioDriver::deal_msi(int msi_cap_ptr)
    {
      hsai_panic( "msi have not achieved!\n" );
    }

    // find a free descriptor, mark it non-free, return its index.
    int VirtioDriver::alloc_desc()
    {
      for(int i = 0; i < NUM; i++){
        if(disk.free[i]){
          disk.free[i] = 0;
          return i;
        }
      }
      return -1;
    }

    // mark a descriptor as free.
    void VirtioDriver::free_desc(int i)
    {
      if(i >= NUM)
        hsai_panic("virtio_disk_intr 1");
      if(disk.free[i])
        hsai_panic("virtio_disk_intr 2");
      disk.desc[i].addr = 0;
      disk.free[i] = 1;
      hsai::wakeup_at(&disk.free[0]);
    }

    // free a chain of descriptors.
    void VirtioDriver::free_chain(int i)
    {
      while(1){
        free_desc(i);
        if(disk.desc[i].flags & VRING_DESC_F_NEXT)
          i = disk.desc[i].next;
        else
          break;
      }
    }

    int VirtioDriver::alloc3_desc(int *idx)
    {
      for(int i = 0; i < 3; i++){
        idx[i] = alloc_desc();
        if(idx[i] < 0){
          for(int j = 0; j < i; j++)
            free_desc(idx[j]);
          return -1;
        }
      }
      return 0;
    }

    void VirtioDriver::virtio_disk_rw( long start_block, long block_count,
      hsai::BufferDescriptor *buf_list, int buf_count, bool write )
    {
      // 读取 VirtIO 设备状态寄存器
    uint8_t status = API_PciDevConfigReadByte(_reg, VIRTIO_PCI_STATUS);
    hsai_printf("VIRTIO_PCI_STATUS = 0x%02X\n", status);
    
    // 验证驱动状态位
    if (status & VIRTIO_CONFIG_S_DRIVER_OK) {
        hsai_printf("VirtIO driver is OK\n");
    } else {
        hsai_printf("ERROR: VirtIO driver is not initialized properly!\n");
    }
    
    // 读取中断状态寄存器
    uint8_t isr = API_PciDevConfigReadByte(_reg, VIRTIO_PCI_ISR);
    hsai_printf("VIRTIO_PCI_ISR = 0x%02X\n", isr);
    
    // 验证队列是否就绪
    API_PciDevConfigWriteWord(_pci_dev, VIRTIO_PCI_QUEUE_SEL, 0);
    uint16_t qsize = API_PciDevConfigReadWord(_pci_dev, VIRTIO_PCI_QUEUE_NUM_MAX);
    hsai_printf("Queue 0 size = %d\n", qsize);
      if(buf_count > 1)
        hsai_panic( "buf_count > 1 not implement" );
      disk.vdisk_lock.acquire();

      // the spec says that legacy block operations use three
      // descriptors: one for type/reserved/sector, one for
      // the data, one for a 1-byte status result.

      int idx[3];
      while(1){
        if(alloc3_desc(idx) == 0) {
          break;
        }
        hsai::sleep_at(&disk.free[0], disk.vdisk_lock);
      }
      
      struct virtio_blk_outhdr {
        uint32 type;
        uint32 reserved;
        uint64 sector;
      } buf0;
      
      buf0.type = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
      buf0.reserved = 0;
      buf0.sector = start_block;
      
      // 设置请求头描述符
      disk.desc[idx[0]].addr  = (uint64) &buf0 | loongarch::win_1;
      disk.desc[idx[0]].len	  = sizeof( buf0 );
      disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
      disk.desc[idx[0]].next = idx[1];
      
      // 设置数据描述符
      disk.desc[idx[1]].addr = buf_list[0].buf_addr;
      disk.desc[idx[1]].len	 = _block_size * block_count;
      disk.desc[idx[1]].flags	 = write ? 0 : VRING_DESC_F_WRITE;
      disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
      disk.desc[idx[1]].next = idx[2];
      
      // 设置状态描述符
      disk.info[idx[0]].status = 0; // 设置为未完成状态
      disk.desc[idx[2]].addr = (uint64) &disk.info[idx[0]].status;
      disk.desc[idx[2]].len = 1;
      disk.desc[idx[2]].flags = VRING_DESC_F_WRITE;
      disk.desc[idx[2]].next = 0;
      
      // 记录请求信息
      disk.info[idx[0]].b = buf_list;
      disk.info[idx[0]].wait = true;

      // 通知设备
      disk.avail[2 + (disk.avail[1] % NUM)] = idx[0];
      __sync_synchronize();
      disk.avail[1]++;
      
      // PCI方式通知队列
      API_PciDevConfigWriteWord(_pci_dev, VIRTIO_PCI_QUEUE_NOTIFY, 0);

      // 等待操作完成
      while(disk.info[idx[0]].wait) {
        if(hsai::get_cur_proc())
          hsai::sleep_at(buf_list, disk.vdisk_lock);
        else {
          disk.vdisk_lock.release();
          asm("wait"); // 使用wait指令代替轮询
          disk.vdisk_lock.acquire();
        }
      }
      disk.info[idx[0]].b = 0;   // disk is done with buf

      free_chain(idx[0]);
      disk.vdisk_lock.release();
    }

    int	VirtioDriver::read_blocks_sync( long start_block, long block_count, hsai::BufferDescriptor *buf_list,
        int buf_count )
    {
      virtio_disk_rw(start_block, block_count, buf_list, buf_count, 0);
      return 0;
    }
    int	VirtioDriver::read_blocks( long start_block, long block_count,
      hsai::BufferDescriptor *buf_list, int buf_count )
    {
      hsai_panic( "not implement" );
      while ( 1 );
    }
    int	VirtioDriver::write_blocks_sync( long start_block, long block_count,
      hsai::BufferDescriptor *buf_list, int buf_count ) 
    {
      virtio_disk_rw(start_block, block_count, buf_list, buf_count, 1);
      return 0;
    }
    int	VirtioDriver::write_blocks( long start_block, long block_count,
      hsai::BufferDescriptor *buf_list, int buf_count )
    {
      hsai_panic( "not implement" );
      while ( 1 );
    }

    int VirtioDriver::handle_intr()
    {
      hsai_printf( "333\n" );
      disk.vdisk_lock.acquire();

      // Read ISR to acknowledge the interrupt
      uint8_t isr = API_PciDevConfigReadByte(_reg, VIRTIO_PCI_ISR);

      if(isr & VIRTIO_PCI_ISR_INTR) {
          while((disk.used_idx % NUM) != (disk.used->id % NUM)){
              int id = disk.used->elems[disk.used_idx].id;

              if(disk.info[id].status != 0)
                  hsai_panic("virtio_disk_intr status");

              disk.info[id].wait = false;   // disk is done with buf
              hsai::wakeup_at(disk.info[id].b);

              disk.used_idx = (disk.used_idx + 1) % NUM;
          }
          API_PciDevConfigWriteByte(_reg, VIRTIO_PCI_ISR, isr & ~(VIRTIO_PCI_ISR_INTR));
      }

      disk.vdisk_lock.release();
      return 0;
    }
	} // namespace qemu

} // namespace loongarch