#include <ata/ata.hh>
#include <device_manager.hh>
#include <hsai_global.hh>
#include <hsai_log.hh>
#include <mem/virtual_memory.hh>

#include "include/dmac.hh"
#include "include/spi.hh"
#include "include/sdcard.hh"

using namespace riscv::qemuk210;

void Sdcard::sd_send_cmd(uint8 cmd, uint32 arg, uint8 crc) {
	uint8 frame[6];
	frame[0] = (cmd | 0x40);
	frame[1] = (uint8)(arg >> 24);
	frame[2] = (uint8)(arg >> 16);
	frame[3] = (uint8)(arg >> 8);
	frame[4] = (uint8)(arg);
	frame[5] = (crc);
	SD_CS_LOW();
	sd_write_data(frame, 6);
}

void Sdcard::sd_end_cmd(void) {
	uint8 frame[1] = {0xFF};
	/*!< SD chip select high */
	SD_CS_HIGH();
	/*!< Send the Cmd bytes */
	sd_write_data(frame, 1);
}

uint8 Sdcard::sd_get_response_R1(void) {
	uint8 result;
	uint16 timeout = 0xff;

	while (timeout--) {
		sd_read_data(&result, 1);
		if (result != 0xff)
			return result;
	}

	// timeout!
	return 0xff;
}

int Sdcard::switch_to_SPI_mode(void) {
	int timeout = 0xff;

	while (--timeout) {
		sd_send_cmd(SD_CMD0, 0, 0x95);
		uint64 result = sd_get_response_R1();
		sd_end_cmd();

		if (0x01 == result) break;
	}
	if (0 == timeout) {
		printf("SD_CMD0 failed\n");
		return 0xff;
	}

	return 0;
}

int Sdcard::verify_operation_condition(void) {
	uint64 result;

	// Stores the response reversely. 
	// That means 
	// frame[2] - VCA 
	// frame[3] - Check Pattern 
	uint8 frame[4];

	sd_send_cmd(SD_CMD8, 0x01aa, 0x87);
	result = sd_get_response_R1();
	sd_get_response_R7_rest(frame);
	sd_end_cmd();

	if (0x09 == result) {
		printf("invalid CRC for CMD8\n");
		return 0xff;
	}
	else if (0x01 == result && 0x01 == (frame[2] & 0x0f) && 0xaa == frame[3]) {
		return 0x00;
	}

	printf("verify_operation_condition() fail!\n");
	return 0xff;
}

int Sdcard::read_OCR(void) {
	uint64 result;
	uint8 ocr[4];

	int timeout;

	timeout = 0xff;
	while (--timeout) {
		sd_send_cmd(SD_CMD58, 0, 0);
		result = sd_get_response_R1();
		sd_get_response_R3_rest(ocr);
		sd_end_cmd();

		if (
			0x01 == result && // R1 response in idle status 
			(ocr[1] & 0x1f) && (ocr[2] & 0x80) 	// voltage range valid 
		) {
			return 0;
		}
	}

	// timeout!
	printf("read_OCR() timeout!\n");
	printf("result = %d\n", result);
	return 0xff;
}

// send ACMD41 to tell sdcard to finish initializing 
int Sdcard::set_SDXC_capacity(void) {
	uint8 result = 0xff;

	int timeout = 0xfff;
	while (--timeout) {
		sd_send_cmd(SD_CMD55, 0, 0);
		result = sd_get_response_R1();
		sd_end_cmd();
		if (0x01 != result) {
			printf("SD_CMD55 fail! result = %d\n", result);
			return 0xff;
		}

		sd_send_cmd(SD_ACMD41, 0x40000000, 0);
		result = sd_get_response_R1();
		sd_end_cmd();
		if (0 == result) {
			return 0;
		}
	}

	// timeout! 
	printf("set_SDXC_capacity() timeout!\n");
	printf("result = %d\n", result);
	return 0xff;
}

// check OCR register to see the type of sdcard, 
// thus determine whether block size is suitable to buffer size
static int check_block_size(void) {
	uint8 result = 0xff;
	uint8 ocr[4];

	int timeout = 0xff;
	while (timeout --) {
		sd_send_cmd(SD_CMD58, 0, 0);
		result = sd_get_response_R1();
		sd_get_response_R3_rest(ocr);
		sd_end_cmd();

		if (0 == result) {
			if (ocr[0] & 0x40) {
				printf("SDHC/SDXC detected\n");
				if (512 != BSIZE) {
					printf("BSIZE != 512\n");
					return 0xff;
				}

				is_standard_sd = false;
			}
			else {
				printf("SDSC detected, setting block size\n");

				// setting SD card block size to BSIZE 
				int timeout = 0xff;
				int result = 0xff;
				while (--timeout) {
					sd_send_cmd(SD_CMD16, BSIZE, 0);
					result = sd_get_response_R1();
					sd_end_cmd();

					if (0 == result) break;
				}
				if (0 == timeout) {
					printf("check_OCR(): fail to set block size");
					return 0xff;
				}

				is_standard_sd = true;
			}

			return 0;
		}
	}

	// timeout! 
	printf("check_OCR() timeout!\n");
	printf("result = %d\n", result);
	return 0xff;
}

/*
 * @brief  Initializes the SD/SD communication.
 * @param  None
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
int Sdcard::read_blocks_sync( long start_block, long block_count,
										hsai::BufferDescriptor *buf_list, int buf_count )
{
	if ( buf_count <= 0 )
	{
		hsai_warn( "不合法的缓冲区数量(%d)", buf_count );
		return -1;
	}

	uint8 result;
	uint32 address;
	uint8 dummy_crc[2];

	if ( is_standard_sd ) { address = start_block << 9; }
	else { address = start_block; }

	// enter critical section!
	_lock.acquire();
	sd_send_cmd(SD_CMD17, address, 0);
	result = sd_get_response_R1();
	if (0 != result) {
		_lock.release();
		hsai_panic("sdcard: fail to read");
	}

	int timeout = 0xffffff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0xfe == result) break;
	}
	if (0 == timeout) {
		hsai_panic("sdcard: timeout waiting for reading");
	}
	sd_read_data_dma(buf_list->buf_addr, buf_list->buf_size);
	sd_read_data(dummy_crc, 2);

	sd_end_cmd();
	_lock.release();
	// leave critical section!

	return 0;
}

int Sdcard::read_blocks( long start_block, long block_count,
								   hsai::BufferDescriptor *buf_list, int buf_count )
{
	hsai_panic( "not implement" );
	while ( 1 );
}

int Sdcard::write_blocks_sync( long start_block, long block_count,
										 hsai::BufferDescriptor *buf_list, int buf_count )
{
	if ( buf_count <= 0 )
	{
		hsai_warn( "不合法的缓冲区数量(%d)", buf_count );
		return -1;
	}
	uint32 address;
	static uint8 const START_BLOCK_TOKEN = 0xfe;
	uint8 dummy_crc[2] = {0xff, 0xff};

	if (is_standard_sd) {
		address = start_block << 9;
	}
	else {
		address = start_block;
	}

	// enter critical section!
	_lock.acquire();

	sd_send_cmd(SD_CMD24, address, 0);
	if (0 != sd_get_response_R1()) {
		_lock.release();
		panic("sdcard: fail to write");
	}

	// sending data to be written 
	sd_write_data(&START_BLOCK_TOKEN, 1);
	sd_write_data_dma(buf_list->buf_addr, buf_list->buf_size);
	sd_write_data(dummy_crc, 2);

	// waiting for sdcard to finish programming 
	uint8 result;
	int timeout = 0xfff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0x05 == (result & 0x1f)) {
			break;
		}
	}
	if (0 == timeout) {
		_lock.release();
		hsai_panic("sdcard: invalid response token");
	}
	
	timeout = 0xffffff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0 != result) break;
	}
	if (0 == timeout) {
		_lock.release();
		hsai_panic("sdcard: timeout waiting for response");
	}
	sd_end_cmd();

	// send SD_CMD13 to check if writing is correctly done 
	uint8 error_code = 0xff;
	sd_send_cmd(SD_CMD13, 0, 0);
	result = sd_get_response_R1();
	sd_read_data(&error_code, 1);
	sd_end_cmd();
	if (0 != result || 0 != error_code) {
		_lock.release();
		printf("result: %x\n", result);
		printf("error_code: %x\n", error_code);
		hsai_panic("sdcard: an error occurs when writing");
	}

	_lock.release();
	// leave critical section!
	return 0;
}

int Sdcard::write_blocks( long start_block, long block_count,
									hsai::BufferDescriptor *buf_list, int buf_count )
{
	hsai_panic( "not implement" );
	while ( 1 );
}

Sdcard::Sdcard( int port_id)
{
	uint8 frame[10];
	_lock.init( "sdcard" );

	sd_lowlevel_init(0);
	//SD_CS_HIGH();
	SD_CS_LOW();

	// send dummy bytes for 80 clock cycles 
	for (int i = 0; i < 10; i ++) 
		frame[i] = 0xff;
	sd_write_data(frame, 10);

	if (0 != switch_to_SPI_mode()) 
		hsai_panic( "\e[5m" "switch_to_SPI_mode失败!" );
	if (0 != verify_operation_condition()) 
		hsai_panic( "\e[5m" "verify_operation_condition失败!" );
	if (0 != read_OCR()) 
		hsai_panic( "\e[5m" "read_OCR失败!" );
	if (0 != set_SDXC_capacity()) 
		hsai_panic( "\e[5m" "set_SDXC_capacity失败!" );
	if (0 != check_block_size()) 
		hsai_panic( "\e[5m" "check_block_size失败!" );

	_port_id		  = port_id;
	static char _default_dev_name[] = "hd?";
	for ( ulong i = 0; i < sizeof _default_dev_name; ++i ) _dev_name[i] = _default_dev_name[i];
	_dev_name[2] = 'a' + (char) _port_id;
	_dev_name[3] = '\0';
	hsai::k_devm.register_block_device( this, _dev_name );
}

void AhciPortDriverLs::isu_cmd_identify( void *buffer, uint len,
										 std::function<int( void )> callback_handler )
{
	if ( len < 512 )
	{
		hsai_warn( "buffer size is not enough.\n" "identify command would not be issue" );
		return;
	}

	uint slot = _default_cmd_slot;

	// 配置 command head

	hsai::AhciCmdHeader &head = _cmd_lst->headers[slot];
	_fill_command_head( head, 1 );

	// 配置 command FIS

	hsai::SataFisRegH2D *fis = (hsai::SataFisRegH2D *) _cmd_tbl->cmd_fis;
	_fill_command_fis( *fis, hsai::ata_cmd_identify_device, 0, 0, 0, 0 );

	// 配置 PRDT （数据区域表）

	hsai::AhciPrd &prd0 = _cmd_tbl->prdt[0];

	u64 pra		   = hsai::k_mem->to_dma( (ulong) buffer );
	prd0.dba	   = pra;
	prd0.interrupt = 0;
	prd0.dbc	   = len - 1;

	// 设置中断回调函数

	if ( callback_handler )
		_call_back = callback_handler;
	else
		_call_back = std::bind( &AhciPortDriverLs::ahci_port_default_call_back, this );

	// 发布命令

	while ( _cmd_slot_busy( slot ) || _task_busy() );
	_issue_command_slot( slot );
}

void AhciPortDriverLs::isu_cmd_read_dma(
	u64 lba, u16 blk_cnt, uint prd_cnt,
	std::function<void( uint prd_i, uint64 &pr_base, uint32 &pr_size )> set_prd_handler,
	std::function<int( void )>											callback_handler )
{
	uint slot = _default_cmd_slot;

	// 配置 command head

	hsai::AhciCmdHeader &head = _cmd_lst->headers[slot];
	_fill_command_head( head, prd_cnt );

	// 配置 command FIS

	hsai::SataFisRegH2D *fis = (hsai::SataFisRegH2D *) _cmd_tbl->cmd_fis;
	_fill_command_fis( *fis, hsai::ata_cmd_read_dma, lba, blk_cnt, 0, 1 << 6 );

	// 配置 PRDT （数据区域表）

	hsai::AhciPrd *prd;
	uint64		   dba;
	uint32		   dbc;
	for ( uint i = 0; i < prd_cnt; i++ )
	{
		prd = &_cmd_tbl->prdt[i];
		dba = dbc = 0;
		set_prd_handler( i, dba, dbc );
		if ( dba == 0 || dbc == 0 )
		{
			hsai_error( "AHCI : 无效的DBA或DBC" );
			return;
		}
		if ( dbc > _1M * 4 )
		{
			hsai_warn( "AHCI : PR长度超过4MiB, read DMA命令将不会被发送" );
			return;
		}
		prd->dba	   = hsai::k_mem->to_dma( dba );
		prd->dbc	   = dbc - 1;
		prd->interrupt = 0;
		prd->resv1	   = 0;
		prd->resv2	   = 0;
	}

	// 设置中断回调函数

	if ( callback_handler )
		_call_back = callback_handler;
	else
		_call_back = std::bind( &AhciPortDriverLs::ahci_port_default_call_back, this );

	// 发布命令

	while ( _cmd_slot_busy( slot ) || _task_busy() );
	_issue_command_slot( slot );
}

void AhciPortDriverLs::isu_cmd_write_dma(
	uint64 lba, uint64 blk_cnt, uint prd_cnt,
	std::function<void( uint prd_i, uint64 &pr_base, uint32 &pr_size )> set_prd_handler,
	std::function<int( void )>											callback_handler )
{
	uint slot = _default_cmd_slot;

	// 配置 command head

	hsai::AhciCmdHeader &head = _cmd_lst->headers[slot];
	_fill_command_head( head, prd_cnt );

	// 配置 command FIS

	hsai::SataFisRegH2D *fis = (hsai::SataFisRegH2D *) _cmd_tbl->cmd_fis;
	_fill_command_fis( *fis, hsai::ata_cmd_write_dma, lba, blk_cnt, 0, 1 << 6 );

	// 配置 PRDT （数据区域表）

	hsai::AhciPrd *prd;
	uint64		   dba;
	uint32		   dbc;
	for ( uint i = 0; i < prd_cnt; i++ )
	{
		prd = &_cmd_tbl->prdt[i];
		dba = dbc = 0;
		set_prd_handler( i, dba, dbc );
		if ( dba == 0 || dbc == 0 )
		{
			hsai_error( "AHCI : 无效的DBA或DBC" );
			return;
		}
		if ( dbc > _1M * 4 )
		{
			hsai_warn( "AHCI : PR长度超过4MiB, read DMA命令将不会被发送" );
			return;
		}
		prd->dba	   = hsai::k_mem->to_dma( dba );
		prd->dbc	   = dbc - 1;
		prd->interrupt = 0;
		prd->resv1	   = 0;
		prd->resv2	   = 0;
	}

	// 设置中断回调函数

	if ( callback_handler )
		_call_back = callback_handler;
	else
		_call_back = std::bind( &AhciPortDriverLs::ahci_port_default_call_back, this );

	// 发布命令

	while ( _cmd_slot_busy( slot ) || _task_busy() );
	_issue_command_slot( slot );
}


int Sdcard::handle_intr()
{
	dmac_intr(DMAC_CHANNEL0);
	return 0;
}
