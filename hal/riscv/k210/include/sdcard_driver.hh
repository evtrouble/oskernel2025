#pragma once

#include <block_device.hh>
#include <disk_partition_device.hh>
#include <hsai_global.hh>
#include <hsai_log.hh>
#include <kernel/klib/function.hh>
#include <mem/virtual_memory.hh>
#include <smp/spin_lock.hh>

#include "include/k210.hh"
#include "include/sdcard_driver.hh"
#include "include/gpiohs.hh"
#include "include/spi.hh"

namespace riscv
{
	namespace k210
	{
        class SdcardDriver : public hsai::BlockDevice
		{
            
			/*
			 * Be noticed: all commands & responses below
			 * 		are in SPI mode format. May differ from
			 * 		what they are in SD mode.
			 */
                            
             #define SD_CMD0 	0 
             #define SD_CMD8 	8
             #define SD_CMD58 	58 		// READ_OCR
             #define SD_CMD55 	55 		// APP_CMD
             #define SD_ACMD41 	41 		// SD_SEND_OP_COND
             #define SD_CMD16 	16 		// SET_BLOCK_SIZE 
             #define SD_CMD17 	17 		// READ_SINGLE_BLOCK
             #define SD_CMD24 	24 		// WRITE_SINGLE_BLOCK 
             #define SD_CMD13 	13 		// SEND_STATUS
        private:
			hsai::SpinLock _lock;
            char _dev_name[8];

			int _port_id = 0;
            // Used to differ whether sdcard is SDSC type.
			bool is_standard_sd = false;
            static constexpr int _block_size = 512;

		public:

			virtual long get_block_size() override { return (long) _block_size; }
			virtual int	 read_blocks_sync( long start_block, long block_count,
										   hsai::BufferDescriptor *buf_list,
										   int					   buf_count ) override;
			virtual int	 read_blocks( long start_block, long block_count,
									  hsai::BufferDescriptor *buf_list, int buf_count ) override;
			virtual int	 write_blocks_sync( long start_block, long block_count,
											hsai::BufferDescriptor *buf_list,
											int						buf_count ) override;
			virtual int	 write_blocks( long start_block, long block_count,
									   hsai::BufferDescriptor *buf_list, int buf_count ) override;
			virtual int	 handle_intr() override;

			virtual bool read_ready() override
			{
				uint8 result;
                uint8 status;
                
                sd_send_cmd(SD_CMD13, 0, 0);
                result = sd_get_response_R1();
                sd_read_data(&status, 1);
                sd_end_cmd();
                
                // 检查状态位
                // bit 0: 在空闲状态
                // bit 8: 准备好接受数据
                if (result != 0 || (status & 0x01)) {
                    return 0;  // 未准备好
                }
                return 1;  // 准备好
			}
			virtual bool write_ready() override
			{
                uint8 result;
                uint8 status;
                
                sd_send_cmd(SD_CMD13, 0, 0);
                result = sd_get_response_R1();
                sd_read_data(&status, 1);
                sd_end_cmd();
                
                // 检查状态位
                // bit 0: 在空闲状态
                // bit 8: 准备好接受数据
                // bit 4: 写保护状态
                if (result != 0 || (status & 0x01) || (status & 0x10)) {
                    return 0;  // 未准备好
                }
                return 1;  // 准备好
			}

		public:

            SdcardDriver() = default;
            SdcardDriver( int port_id);

        private:

			int check_block_size( void );

			void SD_CS_HIGH(void) {
                gpiohs_set_pin(7, GPIO_PV_HIGH);
            }
                            
            void SD_CS_LOW(void) {
                gpiohs_set_pin(7, GPIO_PV_LOW);
            }
                            
            void SD_HIGH_SPEED_ENABLE(void) {
                // spi_set_clk_rate(SPI_DEVICE_0, 10000000);
            }
                            
            void sd_lowlevel_init(uint8 spi_index) {
                gpiohs_set_drive_mode(7, GPIO_DM_OUTPUT);
                // spi_set_clk_rate(SPI_DEVICE_0, 200000);     /*set clk rate*/
            }
                            
            void sd_write_data(uint8 const *data_buff, uint32 length) {
                spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
                spi_send_data_standard(SPI_DEVICE_0, SPI_CHIP_SELECT_3, NULL, 0, data_buff, length);
            }
                            
            void sd_read_data(uint8 *data_buff, uint32 length) {
                spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
                spi_receive_data_standard(SPI_DEVICE_0, SPI_CHIP_SELECT_3, NULL, 0, data_buff, length);
            }
                            
            void sd_write_data_dma(uint8 const *data_buff, uint32 length) {
                spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
                spi_send_data_standard_dma(DMAC_CHANNEL0, SPI_DEVICE_0, SPI_CHIP_SELECT_3, NULL, 0, data_buff, length);
            }
                            
            void sd_read_data_dma(uint8 *data_buff, uint32 length) {
                spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
                spi_receive_data_standard_dma(-1, DMAC_CHANNEL0, SPI_DEVICE_0, SPI_CHIP_SELECT_3, NULL, 0, data_buff, length);
            }
                            
            /*
             * @brief  Send 5 bytes command to the SD card.
             * @param  Cmd: The user expected command to send to SD card.
             * @param  Arg: The command argument.
             * @param  Crc: The CRC.
             * @retval None
             */
			void sd_send_cmd( uint8 cmd, uint32 arg, uint8 crc );
                            
            /*
             * Read sdcard response in R1 type. 
             */
			uint8 sd_get_response_R1( void );

			/* 
             * Read the rest of R3 response 
             * Be noticed: frame should be at least 4-byte long 
             */
            void sd_get_response_R3_rest(uint8 *frame) {
                sd_read_data(frame, 4);
            }
                            
            /* 
             * Read the rest of R7 response 
             * Be noticed: frame should be at least 4-byte long 
             */
            void sd_get_response_R7_rest(uint8 *frame) {
                sd_read_data(frame, 4);
            }

			int switch_to_SPI_mode( void );

			// verify supply voltage range
			int verify_operation_condition( void );

			// read OCR register to check if the voltage range is valid 
            // this step is not mandotary, but I advise to use it
			int read_OCR( void );

			// send ACMD41 to tell sdcard to finish initializing
			int set_SDXC_capacity( void );

			void sd_end_cmd( void );
		};

	} // namespace k210
    
} // namespace riscv