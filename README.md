# Hachimi
A cute, low-cost quadruped robot dog built with an ESP32S3, PCA9685 servo driver, and eight coordinated servos. Its simple, customizable design combines affordable hardware with wireless control and expressive movement.

---

## Final Build

![Final Build](IMG_20260826_152818468.jpg/images/.jpg)

*Completed quadruped robot dog prototype.*

---

## What It Does

The robot can:

- Stand and sit using programmed servo positions
- Walk using coordinated hip and foot movements
- Perform a right-leg handshake
- Wave hello with its right leg
- Display movement status on an OLED
- Receive commands through a Wi-Fi webpage

---

## What Makes It Different

- **Low-cost hardware** using commonly available ESP32 and servo components
- **Eight-servo coordination** for independent hip and foot movement
- **Wireless control** without requiring an internet connection
- **Expressive movement** through handshake and hello gestures
- **Visual feedback** through an OLED display and LED indicator
- **Customizable calibration** for different chassis designs and servo positions

---

## Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32 development board | 1 | Main controller and Wi-Fi access point |
| PCA9685 servo driver | 1 | Controls the eight servos |
| Servo motors | 8 | Drive the robot’s hips and feet |
| I2C OLED display | 1 | Displays robot status |
| Robot dog chassis | 1 | Mechanical structure |
| External servo power supply | 1 | Powers the servos safely |
| LED and resistor | 1 | Hello-status indicator |
| Jumper wires | As required | Electrical connections |
| Battery or USB supply | 1 | Powers the system |

---

## Pin Connections

### ESP32 to PCA9685

| ESP32 | PCA9685 |
|---|---|
| GPIO8 | SDA |
| GPIO9 | SCL |
| 3V3 or 5V | VCC |
| GND | GND |

### OLED Display

The OLED shares the same I2C bus as the PCA9685.

| OLED Pin | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

### Status LED

| Component | ESP32 GPIO |
|---|---:|
| Hello LED | GPIO2 |

Use a current-limiting resistor, typically between 220Ω and 330Ω, in series with the LED.

---

## Servo Channels

| PCA9685 Channel | Servo |
|---:|---|
| 0 | Front-right hip |
| 1 | Front-right foot |
| 2 | Front-left hip |
| 3 | Front-left foot |
| 4 | Back-right hip |
| 5 | Back-right foot |
| 6 | Back-left hip |
| 7 | Back-left foot |

---

## How It Works

1. The ESP32 initializes the PCA9685, OLED, servos, and Wi-Fi access point.
2. The robot moves to its calibrated standing position.
3. A phone or computer connects to the robot’s Wi-Fi network.
4. The user opens the robot’s local webpage.
5. The selected command is sent to the ESP32.
6. The ESP32 runs the corresponding servo movement sequence.
7. The OLED displays the current action.
8. The robot returns to its normal standing position after greeting actions.

---

## Wi-Fi Control

The robot creates its own local Wi-Fi network:

```text
Network: hizru_dog
Password: 12345678
```

After connecting, open:

```text
http://192.168.4.1
```

The control webpage includes:

- STAND
- SIT
- WALK
- HANDSHAKE
- HELLO

The robot does not require a router or internet connection while operating in access-point mode.

---

## OLED Display

The OLED displays the robot’s current state, such as:

```text
STAND
Ready
```

```text
WALK
Moving
```

```text
HELLO
Right leg waving
```

The code checks the common OLED addresses `0x3C` and `0x3D`.

---

## Movement System

Each leg uses two servos:

- A hip servo for forward and backward movement
- A foot servo for lifting and planting the leg

A typical walking step follows this sequence:

1. Lift the foot
2. Move the hip forward
3. Place the foot on the floor
4. Push the body using the hip
5. Return the hip to its support position

Small delays between servo commands help reduce abrupt movement and jitter.

---

## Libraries Used

- [Adafruit PWM Servo Driver Library](https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
- [ESP32 Arduino WiFi](https://github.com/espressif/arduino-esp32)
- [ESP32 WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer)

---

## Power and Safety

Do not power all eight servos directly from the ESP32.

Use:

- A separate power supply for the servos
- A common ground between the ESP32, PCA9685, and servo supply
- Adequate current capacity for simultaneous servo movement
- Mechanical travel limits to prevent servo damage

Test the robot with its legs lifted from the ground before testing walking or greeting actions.

---

## Project Structure

```text
RobotDog/
├── RobotDog_WiFi_OLED_Hello.ino
├── README.md
└── images/
    └── final_build.jpg
```

---

## Status

Prototype complete — eight-servo movement, Wi-Fi webpage control, OLED feedback, walking, sitting, standing, handshake, and hello actions implemented.

---

## License

[Add your license here — for example, MIT, GPL-3.0, or Apache-2.0.]
