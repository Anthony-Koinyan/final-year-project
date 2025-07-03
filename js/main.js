// Import from our new native modules
import * as console from "console";
import { config, set_level, get_level, reset_pin } from "gpio";

// Import from a standard JS file on the filesystem
import { sum } from "./testImport.js";
import { testConsole } from "./testConsole.js";

console.log("--- Starting JavaScript Execution ---");

console.log("The sum from testImport.js is:", sum);
testConsole(sum);

const BLINK_GPIO = 2; // Using GPIO2 for the built-in LED on many boards

try {
  console.log(`Resetting pin ${BLINK_GPIO}`);
  reset_pin(BLINK_GPIO);

  console.log(`Configuring pin ${BLINK_GPIO} as output`);
  // Corresponds to gpio_config_t
  const gpioConfig = {
    pin_bit_mask: 1 << BLINK_GPIO,
    mode: 2, // GPIO_MODE_OUTPUT = 2
    pull_up_en: 0,
    pull_down_en: 0,
    intr_type: 0,
  };
  config(gpioConfig);

  console.log(`Previous level for pin ${BLINK_GPIO}`, get_level(BLINK_GPIO));
  console.log(`Setting pin ${BLINK_GPIO} level to 1 (ON)`);
  set_level(BLINK_GPIO, 1);
} catch (e) {
  console.error("An error occurred during GPIO setup:", e);
}

console.log("--- JavaScript Execution Finished ---");
