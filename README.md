# Arduino Smart Security Alarm System 🚨

A robust, Finite State Machine (FSM) based security alarm system built with Arduino. This project features hardware interrupts for immediate motion detection, a 30-second entry delay countdown, and dynamic password management via a 4x4 Matrix Keypad and an I2C LCD.

## ✨ Features

* **Finite State Machine (FSM) Logic:** Clean and modular transitions between states (`DISARMED`, `MENU`, `EXIT_DELAY`, `ARMED`, `ENTRY_DELAY`, `ALARM`).
* **Hardware Interrupts:** * Uses `INT0` for the PIR sensor to ensure instantaneous motion detection without blocking the main loop.
  * Uses `Timer1` interrupts to generate an oscillating siren effect and toggle the alarm LED concurrently.
* **Entry & Exit Delays:** 10-second delay when leaving the house, and a 30-second countdown to input the PIN when entering before the alarm triggers.
* **Dynamic Password Management:** Change the default PIN directly from the keypad menu.
* **Interactive UI:** Real-time feedback via a 16x2 I2C LCD.

## 🛠️ Hardware Requirements

* 1x Arduino (Uno / Mega / Nano)
* 1x PIR Motion Sensor
* 1x 4x4 Matrix Keypad
* 1x 16x2 LCD Display with I2C Module
* 1x Active/Passive Buzzer
* 1x LED (and 220Ω resistor)
* Jumper Wires & Breadboard

## 🔌 Pin Configuration

| Component | Arduino Pin | Notes |
| :--- | :--- | :--- |
| **PIR Sensor** | `Pin 2` | External Interrupt 0 (`INT0`) |
| **Buzzer** | `Pin 12` | PWM compatible |
| **LED** | `Pin 13` | Built-in or external |
| **Keypad Rows** | `11, 10, 9, 8` | Top to bottom |
| **Keypad Cols** | `7, 6, 5, 4` | Left to right |
| **LCD (I2C)** | `SDA, SCL` | Typically `A4`, `A5` on Arduino Uno |

## 📚 Libraries Used

Make sure to install the following libraries via the Arduino Library Manager:
* [Wire.h](https://www.arduino.cc/en/reference/wire) (Built-in)
* [LiquidCrystal_I2C.h](https://github.com/johnrickman/LiquidCrystal_I2C)
* [Keypad.h](https://playground.arduino.cc/Code/Keypad/)
* [TimerOne.h](https://playground.arduino.cc/Code/Timer1/)

## 🚀 How to Use

1. **Wiring:** Connect the components according to the Pin Configuration table.
2. **Upload:** Flash the code to your Arduino.
3. **Default PIN:** The system defaults to **`1234`**.
4. **Operation:**
   * Press `*` to open the Menu.
   * Press `A` to Activate the alarm (Starts a 10s exit countdown).
   * Press `B` to Change the Password.
   * During the `ENTRY_DELAY` (when motion is detected), enter the PIN and press `#` to disarm.
   * Press `D` to backspace/delete a typed character.

