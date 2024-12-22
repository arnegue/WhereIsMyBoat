#include "display.h"

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_touch_gt911.h"
#include "lvgl.h"

#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))

#define GPIO_INPUT_IO_4 4
#define GPIO_INPUT_PIN_SEL 1ULL << GPIO_INPUT_IO_4

static const char *TAG = "example";

esp_lcd_panel_handle_t panel_handle = NULL; // Handle of panel
lv_disp_t *disp = NULL;                     // Display handle

#define LCD_PIXEL_CLOCK_HZ (18 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL
#define PIN_NUM_BK_LIGHT -1
#define PIN_NUM_HSYNC 46
#define PIN_NUM_VSYNC 3
#define PIN_NUM_DE 5
#define PIN_NUM_PCLK 7
#define PIN_NUM_DATA0 14  // B3
#define PIN_NUM_DATA1 38  // B4
#define PIN_NUM_DATA2 18  // B5
#define PIN_NUM_DATA3 17  // B6
#define PIN_NUM_DATA4 10  // B7
#define PIN_NUM_DATA5 39  // G2
#define PIN_NUM_DATA6 0   // G3
#define PIN_NUM_DATA7 45  // G4
#define PIN_NUM_DATA8 48  // G5
#define PIN_NUM_DATA9 47  // G6
#define PIN_NUM_DATA10 21 // G7
#define PIN_NUM_DATA11 1  // R3
#define PIN_NUM_DATA12 2  // R4
#define PIN_NUM_DATA13 42 // R5
#define PIN_NUM_DATA14 41 // R6
#define PIN_NUM_DATA15 40 // R7
#define PIN_NUM_DISP_EN -1

// The pixel number in horizontal and vertical
#define LCD_H_RES 800
#define LCD_V_RES 480

#define LCD_NUM_FB 1

#define LVGL_TICK_PERIOD_MS 2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY 2

// Function to flush the display
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_p);
    lv_disp_flush_ready(disp);
}

// Timer callback function
static void update_timer_cb(void *arg)
{
    // display_zoomed_array(PIXEL_BUFFER, ZOOM_FACTOR); // 8x zoom factor
}

void gpio_init(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // bit mask of the pins, use GPIO6 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // enable pull-up mode
    //  io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

static void increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void display_init()
{
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
        .num_fbs = LCD_NUM_FB,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = PIN_NUM_DISP_EN,
        .pclk_gpio_num = PIN_NUM_PCLK,
        .vsync_gpio_num = PIN_NUM_VSYNC,
        .hsync_gpio_num = PIN_NUM_HSYNC,
        .de_gpio_num = PIN_NUM_DE,
        .data_gpio_nums = {
            PIN_NUM_DATA0,
            PIN_NUM_DATA1,
            PIN_NUM_DATA2,
            PIN_NUM_DATA3,
            PIN_NUM_DATA4,
            PIN_NUM_DATA5,
            PIN_NUM_DATA6,
            PIN_NUM_DATA7,
            PIN_NUM_DATA8,
            PIN_NUM_DATA9,
            PIN_NUM_DATA10,
            PIN_NUM_DATA11,
            PIN_NUM_DATA12,
            PIN_NUM_DATA13,
            PIN_NUM_DATA14,
            PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            // The following parameters should refer to LCD spec
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .hsync_pulse_width = 4,
            .vsync_back_porch = 16,
            .vsync_front_porch = 16,
            .vsync_pulse_width = 4,
            .flags.pclk_active_neg = true,
        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(TAG, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    gpio_init();
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    void *buf1 = NULL;
    void *buf2 = NULL;
    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers from PSRAM");
    buf1 = heap_caps_malloc(LCD_H_RES * 100 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * 100);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Install LVGL tick timer");

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"};

    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 100));
}

void display_image()
{
    //LV_IMG_DECLARE(TestOS);
    //lv_obj_t *img1 = lv_img_create(lv_scr_act());
    //lv_img_set_src(img1, &TestOS);
}

esp_lcd_panel_handle_t init_display()
{
    display_init();
    esp_timer_handle_t periodic_timer;
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &update_timer_cb,
        .name = "periodic_update"};
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000000)); // 1 second
    ESP_LOGI(TAG, "Update timer started");
    return panel_handle;
}

void update_display()
{
    lv_timer_handler();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Adjust for the actual delay if necessary
}
