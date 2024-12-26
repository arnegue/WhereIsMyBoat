# Where is My Boat
Are you wondering where your boat is? This small program puts your current boat location on your touch screen of your ESP32!

What does it do?

* Gets a ship's/boat's AIS position from [aisstream.io](https://aisstream.io/) (wss)
* Downloads the map (tiles) from [OpenStreetMap](https://www.openstreetmap.org) for that position (converting PNGs with [Pngle](https://github.com/kikuchan/pngle))
* Puts it on a [4.3" TouchScreen](https://www.waveshare.com/esp32-s3-touch-lcd-4.3.htm) using [LVGL](https://lvgl.io/)

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
