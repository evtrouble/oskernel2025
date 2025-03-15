// GPIOHS Protocol Implementation
#include "include/gpiohs.hh"
#include "include/fpioa.hh"
#include "include/utils.hh"
#include "include/k210.hh"
namespace riscv
{
	namespace k210
    {
        #define GPIOHS_MAX_PINNO 32

        volatile gpiohs_t *const gpiohs = (volatile gpiohs_t *)GPIOHS_V;

        // typedef struct _gpiohs_pin_instance
        // {
        //     uint64 pin;
        //     gpio_pin_edge_t edge;
        //     void (*callback)();
        //     plic_irq_callback_t gpiohs_callback;
        //     void *context;
        // } gpiohs_pin_instance_t;

        // static gpiohs_pin_instance_t pin_instance[32];

        void gpiohs_set_drive_mode(uint8 pin, gpio_drive_mode_t mode)
        {
            // configASSERT(pin < GPIOHS_MAX_PINNO);
            int io_number = fpioa_get_io_by_function((fpioa_function_t)(FUNC_GPIOHS0 + pin));
            // configASSERT(io_number >= 0);

            fpioa_pull_t pull = FPIOA_PULL_NONE;
            uint32 dir = 0;

            switch(mode)
            {
                case GPIO_DM_INPUT:
                    pull = FPIOA_PULL_NONE;
                    dir = 0;
                    break;
                case GPIO_DM_INPUT_PULL_DOWN:
                    pull = FPIOA_PULL_DOWN;
                    dir = 0;
                    break;
                case GPIO_DM_INPUT_PULL_UP:
                    pull = FPIOA_PULL_UP;
                    dir = 0;
                    break;
                case GPIO_DM_OUTPUT:
                    pull = FPIOA_PULL_DOWN;
                    dir = 1;
                    break;
                default:
                    // configASSERT(!"GPIO drive mode is not supported.") 
                    break;
            }

            fpioa_set_io_pull(io_number, pull);
            volatile uint32 *reg = dir ? gpiohs->output_en.u32 : gpiohs->input_en.u32;
            volatile uint32 *reg_d = !dir ? gpiohs->output_en.u32 : gpiohs->input_en.u32;
            set_gpio_bit(reg_d, pin, 0);
            set_gpio_bit(reg, pin, 1);
        }

        // gpio_pin_value_t gpiohs_get_pin(uint8 pin)
        // {
        //     // configASSERT(pin < GPIOHS_MAX_PINNO);
        //     return get_gpio_bit(gpiohs->input_val.u32, pin);
        // }

        void gpiohs_set_pin(uint8 pin, gpio_pin_value_t value)
        {
            // configASSERT(pin < GPIOHS_MAX_PINNO);
            set_gpio_bit(gpiohs->output_val.u32, pin, value);
        }

    } // namespace k210
    
} // namespace riscv
