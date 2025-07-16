import { setup } from "gpio";
import { setTimeout } from "timers";
const PIR_PIN = 13;
const BUTTON_PIN = 5;
const LED_PIN = 2;
const BUZZER_PIN = 4;
let isArmed = false;

const pir = setup(PIR_PIN, { mode: "input", interrupt: "falling" });
const button = setup(BUTTON_PIN, {
  mode: "input",
  pullMode: "pullup",
  interrupt: "falling",
  debounce: 50,
});
const led = setup(LED_PIN, { mode: "output" });
const buzzer = setup(BUZZER_PIN, { mode: "output" });

function onIntruderDetected() {
  console.log("Intruder Detected!");
  buzzer.write(true);
  setTimeout(() => {
    buzzer.write(false);
  }, 2000);
}
function armSystem() {
  isArmed = !isArmed;
  if (isArmed) {
    pir.attachISR(onIntruderDetected);
    led.write(true);
    console.log("System Armed.");
  } else {
    pir.detachISR();
    led.write(false);
    console.log("System Disarmed.");
  }
}

console.log("Intruder Alert System Initialised.");
button.attachISR(armSystem);
