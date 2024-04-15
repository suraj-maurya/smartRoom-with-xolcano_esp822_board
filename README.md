# Smart Room with Xolcano-ESP8266 dev board

This Arduino sketch allows you to control your device with commands and monitor your room brightness. 

> [!WARNING]
> This code is for hobbyists for learning purposes. Not recommended for production use!!


## Set-Up Project in Anedya Dashboard -[ANEDYA](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=xolcanoESP8266)
 
Following steps ouline the overall steps to setup a project. You can read more about the steps [here](https://docs.anedya.io/getting-started/quickstart/#create-a-new-project)

  1. Create account and login
  2. Create new project.
  3. Create variables: temperature and humidity.
  4. Create a node (e.g., for home- Room1 or study room).

 > [!TIP]
 > For more details, Visit anedya [documentation](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=nodeMcu)

> [!IMPORTANT]
 > Variable Identifier is essential; fill it accurately.

## Hardware Set-Up
   - Connect LDR at A0 with voltage divider.
   ![LDR-CONNECTION DIAGRAM](/Circuit-diagram/LDR-Connection%20Diagram.png)

### Code Set-Up 

1. Replace `<PHYSICAL-DEVICE-UUID>` with your 128-bit UUID of the physical device.
2. Replace `<CONNECTION-KEY>` with your connection key, which you can obtain from the node description.
3. Set up your WiFi credentials by replacing `SSID` and `PASSWORD` with your WiFi network's SSID and password.

## Usages

  #### Commands :
  > [!NOTE]
  > Keep 'The data is base64 encoded' **UNCHECK**

  - **Fan :** Use `on/off ` command to turn on/off the Fan. (Command Name-`fan`, Command data-`on/off`) 
  - **Bulb :** Use `on/off` command to trun on/ff the Bulb. (Command Name-`bulb`, Command data-`on/off`)

## Dependencies

### ArduinoJson
This repository contains the ArduinoJson library, which provides efficient JSON parsing and generation for Arduino and other embedded systems. It allows you to easily serialize and deserialize JSON data, making it ideal for IoT projects, data logging, and more.

1. Open the Arduino IDE.
2. Go to `Sketch > Include Library > Manage Libraries...`.
3. In the Library Manager, search for "ArduinoJson".
4. Click on the ArduinoJson entry in the list.
5. Click the "Install" button to install the library.
6. Once installed, you can include the library in your Arduino sketches by adding `#include <ArduinoJson.h>` at the top of your sketch.


### Timelib
The `timelib.h` library provides functionality for handling time-related operations in Arduino sketches. It allows you to work with time and date, enabling you to synchronize events, schedule tasks, and perform time-based calculations.

To include the `timelib.h` library:

1. Open the Arduino IDE.
2. Go to `Sketch > Include Library > Manage Libraries...`.
3. In the Library Manager, search for "Time".
4. Click on the ArduinoJson entry in the list(`Time by Michael Margolis`).
5. Click the "Install" button to install the library.
6. Once installed, you can include the library in your Arduino sketches by adding `#include <TimeLib.h>` at the top of your sketch.
