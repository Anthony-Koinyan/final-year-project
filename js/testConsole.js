import * as console from "console";

export function testConsole(logVal) {
  console.log("--- Starting Console Module Tests ---");

  console.warn("This is a warning from the simple script.");
  console.error("And this is an error!");
  console.log(logVal);

  console.log("--- Console Module Tests Complete ---");
}
