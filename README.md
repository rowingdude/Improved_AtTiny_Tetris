# Improved AtTiny85 Tetris Game

This repository contains the source code for an improved version of the Tetris game implemented on the AtTiny85 microcontroller. The game is developed by Benjamin Cance and is licensed under the MIT license, allowing free use and modification.

Please note, this is not tested on a real board, I have been using TinkerCAD and WokWi to proof designs.

## Features

- Improved gameplay with new features and optimizations
- Support for an OLED display for visual feedback
- Random seed generation for unpredictable gameplay
- High score tracking and storage in EEPROM
- Debouncing for button inputs
- Watchdog timer for system reset in case of hang-ups

## Hardware Requirements

- AtTiny85 microcontroller
- OLED display with SDA and SCL pins (128x64 pixels recommended)
- Buttons for left, right, rotate, and drop controls

## Wiring Diagram

Connect your Tiny85 as follows:

```
+-------+
| 1   8 |---- VCC
| 2   7 |---- SCL (OLED)
| 3   6 |---- SDA (OLED)
| 4   5 |---- Rotate Button
+-------+
```

## Usage

1. Clone the repository to your local machine.
2. Install the necessary libraries (TinyWireM and Tiny4kOLED) and tools (Arduino IDE) for AtTiny85 development.
3. Open the `Improved_AtTiny85_Tetris_Game.ino` file in the Arduino IDE.
4. Select the "ATtiny85" board and the appropriate programmer from the Tools menu.
5. Upload the code to the AtTiny85 microcontroller.
6. Power up the microcontroller and enjoy playing the improved Tetris game!

## License

This project is licensed under the MIT License. See the `LICENSE` file for more details.

## Acknowledgments

- Original Tetris game concept by Alexey Pajitnov
- AtTiny85 microcontroller and related resources by SparkFun Electronics
- OLED display and related libraries by Adafruit and others
- Arduino community and resources for support and inspiration