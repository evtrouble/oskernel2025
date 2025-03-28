#pragma once
#include <kernel/types.hh>
#include "dmac.hh"
namespace riscv
{
	namespace k210
    {    
        /* clang-format off */
        struct _spi
        {
            /* SPI Control Register 0                                    (0x00)*/
            volatile uint32 ctrlr0;
            /* SPI Control Register 1                                    (0x04)*/
            volatile uint32 ctrlr1;
            /* SPI Enable Register                                       (0x08)*/
            volatile uint32 ssienr;
            /* SPI Microwire Control Register                            (0x0c)*/
            volatile uint32 mwcr;
            /* SPI Slave Enable Register                                 (0x10)*/
            volatile uint32 ser;
            /* SPI Baud Rate Select                                      (0x14)*/
            volatile uint32 baudr;
            /* SPI Transmit FIFO Threshold Level                         (0x18)*/
            volatile uint32 txftlr;
            /* SPI Receive FIFO Threshold Level                          (0x1c)*/
            volatile uint32 rxftlr;
            /* SPI Transmit FIFO Level Register                          (0x20)*/
            volatile uint32 txflr;
            /* SPI Receive FIFO Level Register                           (0x24)*/
            volatile uint32 rxflr;
            /* SPI Status Register                                       (0x28)*/
            volatile uint32 sr;
            /* SPI Interrupt Mask Register                               (0x2c)*/
            volatile uint32 imr;
            /* SPI Interrupt Status Register                             (0x30)*/
            volatile uint32 isr;
            /* SPI Raw Interrupt Status Register                         (0x34)*/
            volatile uint32 risr;
            /* SPI Transmit FIFO Overflow Interrupt Clear Register       (0x38)*/
            volatile uint32 txoicr;
            /* SPI Receive FIFO Overflow Interrupt Clear Register        (0x3c)*/
            volatile uint32 rxoicr;
            /* SPI Receive FIFO Underflow Interrupt Clear Register       (0x40)*/
            volatile uint32 rxuicr;
            /* SPI Multi-Master Interrupt Clear Register                 (0x44)*/
            volatile uint32 msticr;
            /* SPI Interrupt Clear Register                              (0x48)*/
            volatile uint32 icr;
            /* SPI DMA Control Register                                  (0x4c)*/
            volatile uint32 dmacr;
            /* SPI DMA Transmit Data Level                               (0x50)*/
            volatile uint32 dmatdlr;
            /* SPI DMA Receive Data Level                                (0x54)*/
            volatile uint32 dmardlr;
            /* SPI Identification Register                               (0x58)*/
            volatile uint32 idr;
            /* SPI DWC_ssi component version                             (0x5c)*/
            volatile uint32 ssic_version_id;
            /* SPI Data Register 0-36                                    (0x60 -- 0xec)*/
            volatile uint32 dr[36];
            /* SPI RX Sample Delay Register                              (0xf0)*/
            volatile uint32 rx_sample_delay;
            /* SPI SPI Control Register                                  (0xf4)*/
            volatile uint32 spi_ctrlr0;
            /* reserved                                                  (0xf8)*/
            volatile uint32 resv;
            /* SPI XIP Mode bits                                         (0xfc)*/
            volatile uint32 xip_mode_bits;
            /* SPI XIP INCR transfer opcode                              (0x100)*/
            volatile uint32 xip_incr_inst;
            /* SPI XIP WRAP transfer opcode                              (0x104)*/
            volatile uint32 xip_wrap_inst;
            /* SPI XIP Control Register                                  (0x108)*/
            volatile uint32 xip_ctrl;
            /* SPI XIP Slave Enable Register                             (0x10c)*/
            volatile uint32 xip_ser;
            /* SPI XIP Receive FIFO Overflow Interrupt Clear Register    (0x110)*/
            volatile uint32 xrxoicr;
            /* SPI XIP time out register for continuous transfers        (0x114)*/
            volatile uint32 xip_cnt_time_out;
            volatile uint32 endian;
        } __attribute__((packed, aligned(4)));
        using spi_t = _spi;
		/* clang-format on */

		enum _spi_device_num
        {
            SPI_DEVICE_0,
            SPI_DEVICE_1,
            SPI_DEVICE_2,
            SPI_DEVICE_3,
            SPI_DEVICE_MAX,
        };
		using spi_device_num_t = _spi_device_num;

		enum _spi_work_mode
        {
            SPI_WORK_MODE_0,
            SPI_WORK_MODE_1,
            SPI_WORK_MODE_2,
            SPI_WORK_MODE_3,
        };
		using spi_work_mode_t = _spi_work_mode;

		enum _spi_frame_format
        {
            SPI_FF_STANDARD,
            SPI_FF_DUAL,
            SPI_FF_QUAD,
            SPI_FF_OCTAL
        };
		using spi_frame_format_t = _spi_frame_format;

		enum _spi_instruction_address_trans_mode
        {
            SPI_AITM_STANDARD,
            SPI_AITM_ADDR_STANDARD,
            SPI_AITM_AS_FRAME_FORMAT
        };
		using spi_instruction_address_trans_mode_t = _spi_instruction_address_trans_mode;

		enum _spi_transfer_mode
        {
            SPI_TMOD_TRANS_RECV,
            SPI_TMOD_TRANS,
            SPI_TMOD_RECV,
            SPI_TMOD_EEROM
        };
		using spi_transfer_mode_t = _spi_transfer_mode;

		enum _spi_transfer_width
        {
            SPI_TRANS_CHAR = 0x1,
            SPI_TRANS_SHORT = 0x2,
            SPI_TRANS_INT = 0x4,
        };
		using spi_transfer_width_t = _spi_transfer_width;

		enum _spi_chip_select
        {
            SPI_CHIP_SELECT_0,
            SPI_CHIP_SELECT_1,
            SPI_CHIP_SELECT_2,
            SPI_CHIP_SELECT_3,
            SPI_CHIP_SELECT_MAX,
        };
		using spi_chip_select_t = _spi_chip_select;

		typedef enum
        {
            WRITE_CONFIG,
            READ_CONFIG,
            WRITE_DATA_BYTE,
            READ_DATA_BYTE,
            WRITE_DATA_BLOCK,
            READ_DATA_BLOCK,
        } spi_slave_command_e;

        typedef struct
        {
            uint8 cmd;
            uint8 err;
            uint32 addr;
            uint32 len;
        } spi_slave_command_t;

        typedef enum
        {
            IDLE,
            COMMAND,
            TRANSFER,
        } spi_slave_status_e;

        typedef int (*spi_slave_receive_callback_t)(void *ctx);

        // typedef struct _spi_slave_instance
        // {
        //     uint8 int_pin;
        //     uint8 ready_pin;
        //     dmac_channel_number_t dmac_channel;
        //     uint8 dfs;
        //     uint8 slv_oe;
        //     uint8 work_mode;
        //     uint64 data_bit_length;
        //     volatile spi_slave_status_e status;
        //     volatile spi_slave_command_t command;
        //     volatile uint8 *config_ptr;
        //     uint32 config_len;
        //     spi_slave_receive_callback_t callback;
        //     uint8 is_dual;
        //     uint8 mosi_pin;
        //     uint8 miso_pin;
        // } spi_slave_instance_t;

        // typedef struct _spi_data_t
        // {
        //     dmac_channel_number_t tx_channel;
        //     dmac_channel_number_t rx_channel;
        //     uint32 *tx_buf;
        //     uint64 tx_len;
        //     uint32 *rx_buf;
        //     uint64 rx_len;
        //     spi_transfer_mode_t transfer_mode;
        //     bool fill_mode;
        // } spi_data_t;

        extern volatile spi_t *const spi[4];

        /**
         * @brief       Set spi configuration
         *
         * @param[in]   spi_num             Spi bus number
         * @param[in]   mode                Spi mode
         * @param[in]   frame_format        Spi frame format
         * @param[in]   data_bit_length     Spi data bit length
         * @param[in]   endian              0:little-endian 1:big-endian
         *
         * @return      Void
         */
        void spi_init(spi_device_num_t spi_num, spi_work_mode_t work_mode, spi_frame_format_t frame_format,
                    uint64 data_bit_length, uint32 endian);

        /**
         * @brief       Set multiline configuration
         *
         * @param[in]   spi_num                                 Spi bus number
         * @param[in]   instruction_length                      Instruction length
         * @param[in]   address_length                          Address length
         * @param[in]   wait_cycles                             Wait cycles
         * @param[in]   instruction_address_trans_mode          Spi transfer mode
         *
         */
        void spi_init_non_standard(spi_device_num_t spi_num, uint32 instruction_length, uint32 address_length,
                                uint32 wait_cycles, spi_instruction_address_trans_mode_t instruction_address_trans_mode);

        /**
         * @brief       Spi send data
         *
         * @param[in]   spi_num         Spi bus number
         * @param[in]   chip_select     Spi chip select
         * @param[in]   cmd_buff        Spi command buffer point
         * @param[in]   cmd_len         Spi command length
         * @param[in]   tx_buff         Spi transmit buffer point
         * @param[in]   tx_len          Spi transmit buffer length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        void spi_send_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8 *cmd_buff,
                                    uint64 cmd_len, const uint8 *tx_buff, uint64 tx_len);

        /**
         * @brief       Spi receive data
         *
         * @param[in]   spi_num             Spi bus number
         * @param[in]   chip_select         Spi chip select
         * @param[in]   cmd_buff            Spi command buffer point
         * @param[in]   cmd_len             Spi command length
         * @param[in]   rx_buff             Spi receive buffer point
         * @param[in]   rx_len              Spi receive buffer length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        void spi_receive_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8 *cmd_buff,
                                    uint64 cmd_len, uint8 *rx_buff, uint64 rx_len);

        /**
         * @brief       Spi special receive data
         *
         * @param[in]   spi_num         Spi bus number
         * @param[in]   chip_select     Spi chip select
         * @param[in]   cmd_buff        Spi command buffer point
         * @param[in]   cmd_len         Spi command length
         * @param[in]   rx_buff         Spi receive buffer point
         * @param[in]   rx_len          Spi receive buffer length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        void spi_receive_data_multiple(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint32 *cmd_buff,
                                    uint64 cmd_len, uint8 *rx_buff, uint64 rx_len);

        /**
         * @brief       Spi special send data
         *
         * @param[in]   spi_num         Spi bus number
         * @param[in]   chip_select     Spi chip select
         * @param[in]   cmd_buff        Spi command buffer point
         * @param[in]   cmd_len         Spi command length
         * @param[in]   tx_buff         Spi transmit buffer point
         * @param[in]   tx_len          Spi transmit buffer length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        void spi_send_data_multiple(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint32 *cmd_buff,
                                    uint64 cmd_len, const uint8 *tx_buff, uint64 tx_len);

        /**
         * @brief       Spi send data by dma
         *
         * @param[in]   channel_num     Dmac channel number
         * @param[in]   spi_num         Spi bus number
         * @param[in]   chip_select     Spi chip select
         * @param[in]   cmd_buff        Spi command buffer point
         * @param[in]   cmd_len         Spi command length
         * @param[in]   tx_buff         Spi transmit buffer point
         * @param[in]   tx_len          Spi transmit buffer length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        void spi_send_data_standard_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num,
                                        spi_chip_select_t chip_select,
                                        const uint8 *cmd_buff, uint64 cmd_len, const uint8 *tx_buff, uint64 tx_len);

        /**
         * @brief       Spi receive data by dma
         *
         * @param[in]   w_channel_num       Dmac write channel number
         * @param[in]   r_channel_num       Dmac read channel number
         * @param[in]   spi_num             Spi bus number
         * @param[in]   chip_select         Spi chip select
         * @param[in]   cmd_buff            Spi command buffer point
         * @param[in]   cmd_len             Spi command length
         * @param[in]   rx_buff             Spi receive buffer point
         * @param[in]   rx_len              Spi receive buffer length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        void spi_receive_data_standard_dma(dmac_channel_number_t dma_send_channel_num,
                                        dmac_channel_number_t dma_receive_channel_num,
                                        spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8 *cmd_buff,
                                        uint64 cmd_len, uint8 *rx_buff, uint64 rx_len);

        /**
         * @brief       Spi special send data by dma
         *
         * @param[in]   channel_num     Dmac channel number
         * @param[in]   spi_num         Spi bus number
         * @param[in]   chip_select     Spi chip select
         * @param[in]   cmd_buff        Spi command buffer point
         * @param[in]   cmd_len         Spi command length
         * @param[in]   tx_buff         Spi transmit buffer point
         * @param[in]   tx_len          Spi transmit buffer length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        // void spi_send_data_multiple_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num,
        //                                 spi_chip_select_t chip_select,
        //                                 const uint32 *cmd_buff, uint64 cmd_len, const uint8 *tx_buff, uint64 tx_len);

        /**
         * @brief       Spi special receive data by dma
         *
         * @param[in]   dma_send_channel_num        Dmac write channel number
         * @param[in]   dma_receive_channel_num     Dmac read channel number
         * @param[in]   spi_num                     Spi bus number
         * @param[in]   chip_select                 Spi chip select
         * @param[in]   cmd_buff                    Spi command buffer point
         * @param[in]   cmd_len                     Spi command length
         * @param[in]   rx_buff                     Spi receive buffer point
         * @param[in]   rx_len                      Spi receive buffer length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        // void spi_receive_data_multiple_dma(dmac_channel_number_t dma_send_channel_num,
        //                                    dmac_channel_number_t dma_receive_channel_num,
        //                                    spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint32 *cmd_buff,
        //                                    uint64 cmd_len, uint8 *rx_buff, uint64 rx_len);

        /**
         * @brief       Spi fill dma
         *
         * @param[in]   channel_num     Dmac channel number
         * @param[in]   spi_num         Spi bus number
         * @param[in]   chip_select     Spi chip select
         * @param[in]   tx_buff        Spi command buffer point
         * @param[in]   tx_len         Spi command length
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        // void spi_fill_data_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num, spi_chip_select_t chip_select,
        //                        const uint32 *tx_buff, uint64 tx_len);

        /**
         * @brief       Spi normal send by dma
         *
         * @param[in]   channel_num     Dmac channel number
         * @param[in]   spi_num         Spi bus number
         * @param[in]   chip_select     Spi chip select
         * @param[in]   tx_buff         Spi transmit buffer point
         * @param[in]   tx_len          Spi transmit buffer length
         * @param[in]   stw             Spi transfer width
         *
         * @return      Result
         *     - 0      Success
         *     - Other  Fail
         */
        void spi_send_data_normal_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num,
                                    spi_chip_select_t chip_select,
                                    const void *tx_buff, uint64 tx_len, spi_transfer_width_t spi_transfer_width);

        /**
         * @brief       Spi normal send by dma
         *
         * @param[in]   spi_num         Spi bus number
         * @param[in]   spi_clk         Spi clock rate
         *
         * @return      The real spi clock rate
         */
        uint32 spi_set_clk_rate(spi_device_num_t spi_num, uint32 spi_clk);

        /**
         * @brief       Spi full duplex send receive data by dma
         *
         * @param[in]   dma_send_channel_num          Dmac write channel number
         * @param[in]   dma_receive_channel_num       Dmac read channel number
         * @param[in]   spi_num                       Spi bus number
         * @param[in]   chip_select                   Spi chip select
         * @param[in]   tx_buf                        Spi send buffer
         * @param[in]   tx_len                        Spi send buffer length
         * @param[in]   rx_buf                        Spi receive buffer
         * @param[in]   rx_len                        Spi receive buffer length
         *
         */
        // void spi_dup_send_receive_data_dma(dmac_channel_number_t dma_send_channel_num,
        //                                    dmac_channel_number_t dma_receive_channel_num,
        //                                    spi_device_num_t spi_num, spi_chip_select_t chip_select,
        //                                    const uint8 *tx_buf, uint64 tx_len, uint8 *rx_buf, uint64 rx_len);

        /**
         * @brief       Set spi slave configuration
         *
         * @param[in]   int_pin             SPI master starts sending data interrupt.
         * @param[in]   ready_pin           SPI slave ready.
         * @param[in]   dmac_channel        Dmac channel number for block.
         * @param[in]   data_bit_length     Spi data bit length
         * @param[in]   data                SPI slave device data buffer.
         * @param[in]   len                 The length of SPI slave device data buffer.
         * @param[in]   callback            Callback of spi slave.
         *
         * @return      Void
         */
        // void spi_slave_config(uint8 int_pin, uint8 ready_pin, dmac_channel_number_t dmac_channel, uint64 data_bit_length, uint8 *data, uint32 len, spi_slave_receive_callback_t callback);

        // void spi_slave_dual_config(uint8 int_pin,
        //                            uint8 ready_pin,
        //                            uint8 mosi_pin,
        //                            uint8 miso_pin,
        //                            dmac_channel_number_t dmac_channel,
        //                            uint64 data_bit_length,
        //                            uint8 *data,
        //                            uint32 len,
        //                            spi_slave_receive_callback_t callback);

        /**
         * @brief       Spi handle transfer data operations
         *
         * @param[in]   spi_num         Spi bus number
         * @param[in]   chip_select     Spi chip select
         * @param[in]   data            Spi transfer data information
         * @param[in]   cb              Spi DMA callback
         *
         */
        // void spi_handle_data_dma(spi_device_num_t spi_num, spi_chip_select_t chip_select, spi_data_t data, plic_interrupt_t *cb);
        
    } // namespace k210

} // namespace riscv