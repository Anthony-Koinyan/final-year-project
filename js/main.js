import { sum } from "./testImport.js";
import { testConsole } from "./testConsole";

console.log("Hello from main.js script!");
console.log("The sum is:", sum);

testConsole("foo bar baz");
