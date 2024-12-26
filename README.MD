# Where is My Boat
Are you wondering where your boat is? This small program puts your current boat location on your touch screen of your ESP32!

What does it do?

* Gets a ship's/boat's AIS position from [aisstream.io](https://aisstream.io/) (wss)
* Downloads the map (tiles) from [OpenStreetMap](https://www.openstreetmap.org) (converting PNGs with [Pngle](https://github.com/kikuchan/pngle))
* Puts it on a [4.3" TouchScreen](https://www.waveshare.com/esp32-s3-touch-lcd-4.3.htm) using [LVGL](https://lvgl.io/)

# Setup

1. Open this Repository with VSCode
2. Install [ESP-IDF extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)
3. Install [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
4. Setup your ESP
5. Setup [main/config.h](main/config.h)
    1. WIFI-Credentials
    2. Put in your (free) [API-Key from aisstream.io](https://aisstream.io/apikeys) (need to be signed in)
    3. Put in your ship's MMSI
5. Build and upload

# TODOs

* Put screenshot/photo in Readme
* WiFi reconnection
* Put WiFi-state in screen
* Better behavior with timeouts on websockets? (Takes some minutes until valid data is reeived)
* AISSteam can not show the most recent state. Means: If the ship is not sending info, we can't see it
