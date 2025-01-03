# Where is My Boat?
Ever wondered where your boat is? With this handy program, you can track your boat's current location and display it right on the touch screen of your ESP32!

**Features**

* **Live AIS Tracking**: Retrieves your boat's AIS (Automatic Identification System) position from [aisstream.io](https://aisstream.io/) (via WebSocketSecure).
* **Dynamic Mapping**: Fetches map tiles from  [OpenStreetMap](https://www.openstreetmap.org) for your boat’s location, converting PNGs using the efficient [Pngle](https://github.com/kikuchan/pngle) library
* **Interactive Display**: Displays the map on a  [4.3" TouchScreen](https://www.waveshare.com/esp32-s3-touch-lcd-4.3.htm) powered by the versatile [LVGL](https://lvgl.io/) graphics library.

This program combines real-time tracking and intuitive visuals to keep your boat's location just a glance away. Perfect for tech-savvy mariners!

# Setup

1. Open this Repository with VSCode
2. Install [ESP-IDF extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)
3. Install [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
4. Setup your ESP
5. Setup [main/config.h](main/config.h)
    1. WiFi-Credentials
    2. (Free) [API-Key from aisstream.io](https://aisstream.io/apikeys) (need to be signed in)
    3. Ship's MMSI
5. Build and upload

# TODOs

* Put screenshot/photo in Readme
* add a http buffer to each tile, to download and convert tiles in parallel
* WiFi reconnection
* Connecting to Android AP not working?
* Put WiFi-state in screen
* Better behavior with timeouts on websockets? (Takes some minutes until valid data is received)
* AISSteam can not show the most recent state. Means: If the ship is not sending info, we can't see it
    * Store last known position in EEPROM
* If a new position is there but no new tile is downloaded, the ship-marker won't change (see when get_pixel_coordinates's coordinates get updated)

# Troubleshoot
* submodule `Pngle` CMake-Errors "'WhereIsMyBoat/pngle/src' is not a directory."
    * `git submodule init`
    * `git submodule update`
