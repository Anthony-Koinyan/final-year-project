import { setTimeout, clearTimeout, setInterval, clearInterval } from "timers";

function testTimeout() {
  setTimeout(() => {
    console.log("--- Starting Timers Module Tests ---");
    console.log("Called after 1 second");
  }, 1000);

  const t = setTimeout(() => {
    console.log("Called after 2 second");
  }, 2000); // this log should never be printed

  clearTimeout(t);
}

function testInterval() {
  const interval = 1500;
  const stopInterval = 10000;

  const i = setInterval(() => {
    console.log("Called every", interval, "milli seconds");
  }, interval);

  setTimeout(() => {
    console.log("Clearing interval after", stopInterval, "milli seconds");
    clearInterval(i);
    console.log("--- Timers Module Tests Complete ---");
  }, stopInterval);
}

export function testTimers() {
  testTimeout();
  testInterval();
}
