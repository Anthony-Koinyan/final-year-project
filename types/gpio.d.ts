/**
 * @module gpio
 * @description A module for controlling General Purpose Input/Output (GPIO) pins.
 */

declare module "gpio" {
  /**
   * Represents the configuration options for a GPIO pin.
   */
  export type PinMode = "disable" | "input" | "output" | "input_output";

  /**
   * Represents the pull mode for a GPIO pin.
   */
  export type PullMode = "pullup" | "pulldown" | "both" | "floating";

  /**
   * Represents the interrupt trigger type for a GPIO pin.
   */
  export type InterruptType =
    | "rising"
    | "falling"
    | "both"
    | "low"
    | "high"
    | "none";

  /**
   * Configuration object for setting up a GPIO pin.
   */
  export interface PinConfig {
    /** The I/O mode for the pin. */
    mode: PinMode;
    /** The pull-up or pull-down configuration. */
    pullMode?: PullMode;
    /** The interrupt trigger type. */
    interrupt?: InterruptType;
    /**
     * The debounce timeout in milliseconds for this specific pin. If provided,
     * software debouncing is enabled. If omitted, it is disabled.
     * Recommended value is 50-100ms for bouncy switches.
     */
    debounce?: number;
  }

  /**
   * Represents a single GPIO pin that has been configured.
   * This object provides methods to interact with the pin.
   */
  export interface Pin {
    /** The GPIO pin number. */
    readonly pin: number;

    /**
     * Reads the current logic level of the pin.
     * @returns {boolean} The current level (false for low, true for high).
     */
    read(): boolean;

    /**
     * Writes a logic level to the pin.
     * @param {boolean} level The logic level to set (false for low, true for high).
     */
    write(level: boolean): void;

    /**
     * Attaches an interrupt handler to the pin.
     * The callback will be executed when the interrupt configured in `setup` occurs.
     * @param {() => void} callback The function to call when the interrupt is triggered.
     */
    attachISR(callback: () => void): void;

    /**
     * Detaches the interrupt handler from the pin.
     */
    detachISR(): void;

    /**
     * Resets the pin to its default state and releases all associated
     * resources, including any attached interrupt handlers.
     */
    close(): void;
  }

  /**
   * Configures one or more GPIO pins.
   *
   * @param {number | number[]} pin The GPIO pin number or an array of pin numbers to configure.
   * @param {PinConfig} config The configuration object for the pin(s).
   * @returns {Pin | Pin[]} A Pin object if a single pin was configured, or an array of Pin objects.
   */
  export function setup(pin: number, config: PinConfig): Pin;
  export function setup(pins: number[], config: PinConfig): Pin[];
}
