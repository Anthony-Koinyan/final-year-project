import { setup } from "gpio";
import { setInterval, setTimeout, clearInterval } from "timers";

export function testGPIO() {
  console.log("--- Starting GPIO API Test Script ---");

  // --- Configuration ---
  // Most ESP32 dev boards have a built-in LED on GPIO 2
  const LED_PIN = 2;
  // Most ESP32 dev boards have a BOOT button on GPIO 0
  const BUTTON_PIN = 5;

  try {
    // --- 1. Test LED Output ---
    console.log(`Setting up LED on GPIO ${LED_PIN}...`);
    const led = setup(LED_PIN, { mode: "output" });
    console.log(`LED Pin object created for pin: ${led.pin}`);

    let ledState = false;
    const blinker = setInterval(() => {
      ledState = !ledState;
      led.write(ledState);
      console.log(`LED state written. Reading back: ${led.read()}`);
    }, 1000);

    console.log("LED should now be blinking every second.");

    // --- 2. Test Button Input & Interrupt ---
    console.log(`Setting up Button on GPIO ${BUTTON_PIN} with interrupt...`);
    const button = setup(BUTTON_PIN, {
      mode: "input",
      pullMode: "pullup", // Use internal pull-up resistor
      interrupt: "falling", // Trigger on button press (high to low)
      debounce: 100,
    });

    let pressCount = 0;
    button.attachISR(() => {
      pressCount++;
      console.log(`!!! Button Pressed! (Count: ${pressCount})`);
    });
    console.log("Interrupt attached. Press the BOOT button to test.");

    // --- 3. Test Automatic Cleanup ---
    const TEST_DURATION_MS = 30000;
    console.log(
      `--- Test will run for ${
        TEST_DURATION_MS / 1000
      } seconds before cleanup ---`
    );

    setTimeout(() => {
      console.log("--- Cleaning up all resources... ---");

      // Stop the blinking
      clearInterval(blinker);
      console.log("Blinker interval cleared.");

      // Detach the interrupt handler
      button.detachISR();
      console.log("Button ISR detached.");

      // Close the pins, which also resets them
      led.close();
      console.log("LED pin closed.");
      button.close();
      console.log("Button pin closed.");

      console.log("--- Test Finished ---");
      console.log(
        "The LED should be off and the button should no longer log messages."
      );
    }, TEST_DURATION_MS);
  } catch (e) {
    console.error("An error occurred during GPIO setup or execution:", e);
  }
}
