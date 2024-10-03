#include <driver/gpio_sim.h>
#include <util/container_of.h>

static void adapter_handler(struct adapter *adapter, const struct adapter_message *arg);

void gpio_toggle(struct gpio_pin *pin)
{
    pin->state = !pin->state;
    adapter_send_bool(&pin->adapter, pin->state);
}

void gpio_exti_callback(struct gpio_pin *pin, exti_handler_t handler)
{
    pin->callback = handler;
}

void gpio_pin_init(struct gpio_pin *pin, const char *name, bool_t initial_state)
{
    pin->name = name;
    pin->state = initial_state;
    pin->callback = NULL;

    adapter_init(&pin->adapter, adapter_handler, "gpio/%s", name);
}

static void adapter_handler(struct adapter *adapter, const struct adapter_message *arg)
{
    struct gpio_pin *pin = CONTAINER_OF(adapter, struct gpio_pin, adapter);

    if (adapter_check_bool(arg, true)) {
        pin->state = true;
    }

    if (adapter_check_bool(arg, false)) {
        pin->state = false;
    }

    if (adapter_check_string(arg, "exti")) {
        if (pin->callback) {
            pin->callback(pin);
        }
    }
}
