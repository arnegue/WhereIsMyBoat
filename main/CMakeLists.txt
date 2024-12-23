idf_component_register(SRCS "aisstream.c" "tile_downloader.c" "wifi.c" "display.c" "main.c" "pngle/miniz.c" "pngle/pngle.c"
                    INCLUDE_DIRS "." "pngle"
                    REQUIRES json esp_http_client esp_wifi nvs_flash)
