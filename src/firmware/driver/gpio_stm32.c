#include <driver/gpio_stm32.h>
#include <service/system.h>

static struct gpio_pin *exti_pins;

void gpio_toggle(struct gpio_pin *pin)
{
    HAL_GPIO_TogglePin(pin->port, pin->pin);
}

void gpio_exti_callback(struct gpio_pin *pin, exti_handler_t handler)
{
    system_critical_section_enter();

    if (pin->exti_handler != NULL) {
        return;
    }

    if (exti_pins != NULL) {
        pin->exti_next = exti_pins;
    }

    exti_pins = pin;
    pin->exti_handler = handler;

    system_critical_section_exit();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    struct gpio_pin *exti_pin = exti_pins;

    // find pin
    while ((exti_pin != NULL) && (exti_pin->pin != GPIO_Pin)) {
        exti_pin = exti_pin->exti_next;
    }

    // invoke handler
    if (exti_pin != NULL) {
        exti_pin->exti_handler(exti_pin);
    }
}
