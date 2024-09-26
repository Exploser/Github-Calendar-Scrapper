# GitHub Contributions Display with ESP8266 and LED Matrix

This project uses an ESP8266 to display your GitHub contributions on an LED matrix. The device connects to WiFi, fetches your GitHub contribution data, and scrolls it on the LED display. You can configure the GitHub username and WiFi credentials through a web interface.

![Second](https://github.com/user-attachments/assets/fba468dd-7e01-4bda-92b2-8e904a56a135)

## Features

- Fetches GitHub contributions using HTTPS
- Displays the contributions on a 4x LED matrix
- WiFiManager for easy WiFi setup
- Stores GitHub username and credentials in EEPROM
- Option to reset settings via button press

## Hardware Requirements

- ESP8266 (e.g., NodeMCU)
- 4x LED matrix using the FC16_HW configuration
- Push button for resetting settings

## Libraries Used

- `ESP8266WiFi`
- `ESP8266WebServer`
- `WiFiClientSecure`
- `WiFiManager`
- `MD_MAX72xx`
- `EEPROM`
- `SPI`

## Setup Instructions

1. Clone the repository.
2. Install the necessary libraries mentioned above in the Arduino IDE.
3. Connect your ESP8266 to the LED matrix as per the pin definitions in the code.
4. Flash the code to your ESP8266.
5. The device will create a WiFi access point if it's not already connected. Connect to the access point and input your GitHub username and WiFi credentials through the web interface.
6. Your GitHub contributions will be fetched and displayed on the LED matrix.

## Resetting Settings

To reset the WiFi credentials and GitHub username, hold the reset button for 5 seconds.

---

Feel free to contribute or suggest improvements!
