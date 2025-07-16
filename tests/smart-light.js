import { setup } from "gpio";
import { setTimeout, clearTimeout } from "timers";
const PIR_PIN = 13;
const LDR_PIN = 12;
const BUTTON_PIN = 5;
const RELAY_PIN = 4;
let lightOn = false;
let autoMode = true;
let autoOffTimer = null;

const pir = setup(PIR_PIN, { mode: "input" });
const ldr = setup(LDR_PIN, { mode: "input" });
const button = setup(BUTTON_PIN, {
  mode: "input",
  pullMode: "pullup",
  interrupt: "falling",
  debounce: 200,
});
const relay = setup(RELAY_PIN, { mode: "output" });

function setLight(state) {
  lightOn = state;
  relay.write(state);
  console.log(state ? "Light ON" : "Light OFF");
}
function handleMotion() {
  if (autoOffTimer) {
    clearTimeout(autoOffTimer);
  }
  if (!lightOn) {
    const isDark = !ldr.read();
    if (isDark) {
      console.log("Motion detected in the dark, turning light on.");
      setLight(true);
    } else {
      console.log("Motion detected, but it is bright enough.");
    }
  }
  autoOffTimer = setTimeout(() => {
    console.log("Inactivity timeout, turning light off.");
    setLight(false);
  }, 10000);
}
function toggleMode() {
  autoMode = !autoMode;
  console.log(`Mode switched to: ${autoMode ? "AUTO" : "MANUAL"}`);
  if (autoMode) {
    pir.attachISR(handleMotion);
  } else {
    pir.detachISR();
    setLight(!lightOn);
  }
}

console.log("Smart Light Controller Initialised. Mode: AUTO");
setLight(false);
pir.attachISR(handleMotion);
button.attachISR(toggleMode);
