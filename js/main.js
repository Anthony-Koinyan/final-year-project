import { sum } from "./testImport.js";
import { testConsole } from "./testConsole.js";
import { testGPIO } from "./testGPIO.js";
import { testTimers } from "./testTimers.js";

console.log("--- Starting JavaScript Execution ---");

console.log("The sum from testImport.js is:", sum);
testConsole(sum);
testTimers();

const BLINK_GPIO = 2; // Using GPIO2 for the built-in LED on many boards
testGPIO(BLINK_GPIO);

console.log("--- JavaScript Execution Finished ---");
