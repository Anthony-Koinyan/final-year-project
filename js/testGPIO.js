import { config, set_level, get_level, reset_pin } from "gpio";

export function testGPIO(BLINK_GPIO) {
  console.log("--- Starting GPIO Tests ---");

  setTimeout(() => {
    try {
      console.log(`Resetting pin ${BLINK_GPIO}`);
      reset_pin(BLINK_GPIO);

      console.log(`Configuring pin ${BLINK_GPIO} as output`);

      // Corresponds to gpio_config_t
      const gpioConfig = {
        pin_bit_mask: 1 << BLINK_GPIO,
        mode: 3,
        pull_up_en: 0,
        pull_down_en: 0,
        intr_type: 0,
      };

      config(gpioConfig);

      console.log("Previous level for pin", BLINK_GPIO, get_level(BLINK_GPIO));
      console.log(`Toggling pin ${BLINK_GPIO} every 1 second for 20 seconds`);

      const i = setInterval(() => {
        if (get_level(BLINK_GPIO)) {
          set_level(BLINK_GPIO, 0);
        } else {
          set_level(BLINK_GPIO, 1);
        }

        console.log("Current level for pin", BLINK_GPIO, get_level(BLINK_GPIO));
      }, 3000);

      setTimeout(() => {
        clearInterval(i);
        console.log("--- GPIO Tests Complete ---");
      }, 25000);
    } catch (e) {
      console.error("An error occurred during GPIO setup:", e);
    }
  }, 10000);
}
