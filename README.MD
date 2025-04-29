# Where is My Boat?

![ESP32-S3 4.3inch WhereIsMyBoat](./esp32LCD.jpeg)

Ever wondered where your boat is? With this handy program, you can track your boat's current location and display it right on the touch screen of your ESP32!

# Features

* **Live AIS Tracking**: Retrieves your boat's AIS (Automatic Identification System) position from [aisstream.io](https://aisstream.io/) (via WebSocketSecure).
* **Dynamic Mapping**: Fetches map tiles from [OpenStreetMap](https://www.openstreetmap.org) for your boat’s location, converting PNGs using [Pngle](https://github.com/kikuchan/pngle) library
* **Interactive Display**: Displays the map on a [4.3" TouchScreen](https://www.waveshare.com/esp32-s3-touch-lcd-4.3.htm) or [this one](https://www.waveshare.com/esp32-s3-touch-lcd-4.3b.htm) powered by [LVGL](https://lvgl.io/)

This program combines real-time tracking and intuitive visuals to keep your boat's location just a glance away. Perfect for tech-savvy mariners!

# Setup

1. Checkout repository: `git clone --recurse-submodules https://github.com/arnegue/WhereIsMyBoat`
2. Open this Repository with VSCode
3. Install [ESP-IDF extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)
4. Install [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
5. Setup your ESP
6. Put your free aisstream-API-Key in [main/config.h](main/config.h)
    * Get it from [API-Key from aisstream.io](https://aisstream.io/apikeys) (need to be signed in)
7. Build and upload
8. Runtime Setup:
    * Setup WiFi (press first button (WiFi Icon))
    * Setup MMSI (press second button (GPS Icon))

# Colored Status-Dot meaning
In the top right corner is a colored state-marker. The color mean following:
* Black: Not connected to WiFi
* Red: Not connected to AIS-Stream
* Orange: Connected to AIS-Stream but no AIS-Data were received
* Yellow: Connected to AIS-Stream but no valid AIS-Data were received
* Green: Connected to AIS-Stream and valid AIS-Data were received at least once

# TODOs

* Get rid of main-loop
    * Only display-updates in mainloop
* Boat-Marker
    * If marker is too much on top border of a tile (e.g. 53.538158 / 9.869338) the boat is cut off. Decided whether to put tile on 1/0 or 1/1 (e.g. if boat marker is in Y < 1/3 of tile, then put tile in 1/1)
    * Rotate boat marker according to COG -> lv_img_set_angle(img, angle) | Need to maybe need to mirror when changing over 0/180? Or just -180?
* Get rid of in-code-TODOs
* Add SYMBOL_Close button to keyboard and get rid of abort buttons of wifi and mmsi setup
* Sometimes error occur like: ``{ "error":"concurrent connections per user exceeded"}``. Put this in a warning label. Look when to remove it