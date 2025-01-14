idf_component_register(SRCS "smallBoat.c" "aisstream.c" "tile_downloader.c" "wifi.c" "wifi_ui.c" "display.c" "main.c" "nvs_position.c" "../pngle/src/miniz.c" "../pngle/src/pngle.c"
                    INCLUDE_DIRS "." "../pngle/src"
                    REQUIRES json esp_http_client esp_wifi nvs_flash)
