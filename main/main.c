#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"              // for ESP_RETURN_ON_ERROR macro

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_timer.h"

#include "esp_ldo_regulator.h"      // for powering up the MIPI-DSI PHY
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_st7701.h"

#include "driver/ledc.h"            // for display backlight control
#include "driver/ppa.h"             // for pixel processing accelerator
#include "driver/usb_serial_jtag.h" // for data receiving through USB port

#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"
#include "driver/i2c_master.h"

#include "lvgl.h"

#include "icon_fuelpressure.h"
#include "icon_stall.h"
#include "icon_oilpressure.h"
#include "icon_watertemp.h"

#include "font_gear.h"
#include "font_messages.h"

#include "carprofiles.h"

static const char *TAG_IRDASH = "IRDASH_V4";
static const char *TAG_LVGL = "LVGL";
static const char *TAG_ESP_IDF = "ESP-IDF";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Display functions                                                      ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*****************************************************************/
// Guition JC4880P443C_I_W/Y (LCD - module specific)
/*****************************************************************/
// Pixel clock: 27MHz (480x800 @ ~60Hz)
// MIPI-DSI: 2-lane, 500Mbps/lane
// Color format: RGB565
// Rotation: Portrait (480x800)

#define LCD_NAME                        ST7701
#define LCD_HORIZONTAL_RESOLUTION       (480)
#define LCD_VERTICAL_RESOLUTION         (800)
#define LCD_DATA_WIDTH                  (16)              // bits per pixel

#define LCD_RST_IO                      (5)               // Set to GPIO_NUM_NC if not used

#define LCD_BL_IO                       (23)              // Set to GPIO_NUM_NC if not used
#define LCD_BL_CHANNEL                  LEDC_CHANNEL_1    // LEDC channel number used for backlight PWM control.
#define LCB_BL_TIMER                    LEDC_TIMER_1      // LEDC timer number used for backlight PWM control.
#define LCD_BL_PWM_FREQ                 (20000)           // PWM frequency for backlight control. Higher frequencies reduce visible flicker but may increase power consumption.
#define LCD_BL_ON_LEVEL                 (1)
#define LCD_BL_OFF_LEVEL                (!LCD_BL_ON_LEVEL)

#define MIPI_DSI_PHY_PWR_LDO_CHAN       (3)               // LDO_VO3 is connected to VDD_MIPI_DPHY
#define MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)
#define MIPI_DSI_PHY_LDO_ID             (3)               // Set to -1 if not used

#define LCD_DSI_LANE_NUM                (2)               // ESP32-P4 supports 1 or 2 lanes
#define LCD_DSI_LANE_RATE_MBPS          (500)             // Single lane bit rate, should consult the LCD supplier or check the LCD drive IC datasheet for the supported lane rate.
                                                          // Higher values support faster refresh rates but may cause signal integrity issues.
                                                          // ESP32-P4 supports max 1500Mbps
#define LCD_DPI_NUM_FB                  (2)               // Number of frame buffers for display. 2 buffers enable double buffering for smoother animations, but use more memory.
#define LCD_DPI_VIRTUAL_CHANNEL         (0)
#define LCD_DPI_IN_PIXEL_FORMAT         (LCD_COLOR_FMT_RGB565)
#define LCD_DPI_OUT_PIXEL_FORMAT        (LCD_COLOR_FMT_RGB565)
// Explicit DPI timing for 480x800 panel at ~60Hz (vendor reference)
#define LCD_DPI_CLK_MHZ                 (34)
#define LCD_DPI_TIMINGS_HPW             (12)
#define LCD_DPI_TIMINGS_HBP             (42)
#define LCD_DPI_TIMINGS_HFP             (42)
#define LCD_DPI_TIMINGS_VPW             (2)
#define LCD_DPI_TIMINGS_VBP             (8)
#define LCD_DPI_TIMINGS_VFP             (166)

#define CONFIG_MONITOR_REFRESH_BY_GPIO 0
#if CONFIG_MONITOR_REFRESH_BY_GPIO
  #define PIN_NUM_REFRESH_MONITOR 20            // Monitor the Refresh Rate by toggling the GPIO
#endif

//
// @brief LCD vendor initialization commands
//
// Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for
// initialization sequence code. Please uncomment and change the following macro definitions. Otherwise, the LCD driver
// will use the default initialization sequence code.
//
// The initialization sequence can be specified in two formats:
// 1. Raw format:
//    {command, (uint8_t []){data0, data1, ...}, data_size, delay_ms}
// 2. Helper macros:
//    - ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(delay_ms, command, {data0, data1, ...})
//    - ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(delay_ms, command)
//

// ST7701 vendor initialization sequence for 480x800 panel
static const st7701_lcd_init_cmd_t st7701_lcd_cmds[] = {
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t[]){0x08}, 1, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0x63, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t[]){0x10, 0x08}, 2, 0},
    {0xCC, (uint8_t[]){0x10}, 1, 0},

    {0xB0, (uint8_t[]){0x80, 0x09, 0x53, 0x0C, 0xD0, 0x07, 0x0C, 0x09, 0x09, 0x28, 0x06, 0xD4, 0x13, 0x69, 0x2B, 0x71}, 16, 0},
    {0xB1, (uint8_t[]){0x80, 0x94, 0x5A, 0x10, 0xD3, 0x06, 0x0A, 0x08, 0x08, 0x25, 0x03, 0xD3, 0x12, 0x66, 0x6A, 0x0D}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},

    {0xB0, (uint8_t[]){0x5D}, 1, 0},
    {0xB1, (uint8_t[]){0x58}, 1, 0},
    {0xB2, (uint8_t[]){0x87}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x4E}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x21}, 1, 0},
    {0xB9, (uint8_t[]){0x10, 0x1F}, 2, 0},
    {0xBB, (uint8_t[]){0x03}, 1, 0},
    {0xBC, (uint8_t[]){0x00}, 1, 0},

    {0xC1, (uint8_t[]){0x78}, 1, 0},
    {0xC2, (uint8_t[]){0x78}, 1, 0},
    {0xD0, (uint8_t[]){0x88}, 1, 0},

    {0xE0, (uint8_t[]){0x00, 0x3A, 0x02}, 3, 0},
    {0xE1, (uint8_t[]){0x04, 0xA0, 0x00, 0xA0, 0x05, 0xA0, 0x00, 0xA0, 0x00, 0x40, 0x40}, 11, 0},
    {0xE2, (uint8_t[]){0x30, 0x00, 0x40, 0x40, 0x32, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00}, 13, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t[]){0x09, 0x2E, 0xA0, 0xA0, 0x0B, 0x30, 0xA0, 0xA0, 0x05, 0x2A, 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0}, 16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t[]){0x08, 0x2D, 0xA0, 0xA0, 0x0A, 0x2F, 0xA0, 0xA0, 0x04, 0x29, 0xA0, 0xA0, 0x06, 0x2B, 0xA0, 0xA0}, 16, 0},

    {0xEB, (uint8_t[]){0x00, 0x00, 0x4E, 0x4E, 0x00, 0x00, 0x00}, 7, 0},
    {0xEC, (uint8_t[]){0x08, 0x01}, 2, 0},

    {0xED, (uint8_t[]){0xB0, 0x2B, 0x98, 0xA4, 0x56, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x65, 0x4A, 0x89, 0xB2, 0x0B}, 16, 0},
    {0xEF, (uint8_t[]){0x08, 0x08, 0x08, 0x45, 0x3F, 0x54}, 6, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},

    {0x11, (uint8_t[]){0x00}, 1, 120},
    {0x29, (uint8_t[]){0x00}, 1, 20},
};

/******************************************************************/
// ESP-IDF: Backlight control
/******************************************************************/
//
// @brief Initialize backlight PWM control
// 
// Configures LEDC timer and channel for backlight brightness control.
// Uses 10-bit resolution for smooth brightness adjustment.
//
static esp_err_t bsp_display_brightness_init(void)
{
    const ledc_channel_config_t ch = {
        .gpio_num = LCD_BL_IO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_BL_CHANNEL,
        .duty = 0,
        .timer_sel = LCB_BL_TIMER
    };
    const ledc_timer_config_t t = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LCB_BL_TIMER,
        .freq_hz = LCD_BL_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&t), TAG_ESP_IDF, "ledc timer");
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ch), TAG_ESP_IDF, "ledc ch");

    return ESP_OK;
}

esp_err_t bsp_display_backlight_on(void)
{
    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG_ESP_IDF, "backlight init");
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_BL_CHANNEL, 1023), TAG_ESP_IDF, "backlight set duty");
    
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_BL_CHANNEL);
}

esp_err_t bsp_display_brightness_set(uint8_t brightness_percent)
{
    if (brightness_percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG_ESP_IDF, "backlight init");
    
    // Convert percentage to duty cycle (0-1023 for 10-bit resolution)
    uint32_t duty = (brightness_percent * 1023) / 100;
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_BL_CHANNEL, duty), TAG_ESP_IDF, "backlight set duty");

    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_BL_CHANNEL);
}

/******************************************************************/
// ESP-IDF: Pixel-Processing Accelerator
/******************************************************************/
//https://lvgl.io/docs/open/integration/chip_vendors/espressif/hardware_accelerator_ppa
//https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ppa.html

static ppa_client_handle_t ppa_srm_client = NULL;

static void ppa_init_srm(void)
{
    ppa_client_config_t cfg = {
        .oper_type = PPA_OPERATION_SRM,      // scale / rotate / mirror
        .max_pending_trans_num = 1,
    };
    ESP_ERROR_CHECK(ppa_register_client(&cfg, &ppa_srm_client));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Touch functions                                                        ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TOUCH_SDA 7
#define TOUCH_SCL 8
#define TOUCH_RST GPIO_NUM_NC
#define TOUCH_INT GPIO_NUM_NC
#define TOUCH_FREQ 400000

esp_lcd_touch_handle_t touch_handle = NULL;

void init_touch_controller()
{
  // Initialize I2C bus
  i2c_master_bus_config_t i2c_bus_conf = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .sda_io_num = TOUCH_SDA,
    .scl_io_num = TOUCH_SCL,
    .i2c_port = I2C_NUM_0,
    .flags.enable_internal_pullup = true,
  };
  i2c_master_bus_handle_t i2c_bus;
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_conf, &i2c_bus));

  // Initialize touch IO (I2C)
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
  io_config.scl_speed_hz = TOUCH_FREQ;
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

  // Configure touch panel
  esp_lcd_touch_config_t tp_cfg = {
    .x_max = LCD_VERTICAL_RESOLUTION,   // Screen width
    .y_max = LCD_HORIZONTAL_RESOLUTION, // Screen height
    .rst_gpio_num = TOUCH_RST,          // Reset pin (or GPIO_NUM_NC)
    .int_gpio_num = TOUCH_INT,          // Interrupt pin (or GPIO_NUM_NC)
    .levels = {
        .reset = 0,
        .interrupt = 0,
    },
    .flags = {
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    },
  };

  // Initialize touch controller
  ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(io_handle, &tp_cfg, &touch_handle));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Serial USB-SERIAL-JTAG communication functions                         ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//https://docs.espressif.com/projects/esp-iot-solution/en/latest/usb/usb_overview/usb_serial_jtag.html
#define RX_BUF_SZ 2048

// structure of the incoming serial data block
// variables are aligned for 16/32 bit systems to prevent padding
typedef struct 
{
  char EngineWarnings;
  char Gear;
  bool IsInGarage;
  bool IsOnTrack;
  float Fuel;
  float RPM;
  float Speed;
  float WaterTemp;
} SIncomingData;

static const uint8_t HEADER[3] = { 3, 12, 48 };
SIncomingData InData;

/******************************************************************/
// Setup USB-JTAG communication
/******************************************************************/
void SetupUSBJTAG()
{
  usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
  cfg.rx_buffer_size = RX_BUF_SZ;
  ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));
}

static void serial_flush_input(void)
{
  uint8_t tmp[64];
  while (usb_serial_jtag_read_bytes(tmp, sizeof(tmp), 0) > 0) { }
}

/******************************************************************/
// Feeds one byte to the state machine. Returns true when a full package is received.
/******************************************************************/
static bool ParseByte(uint8_t b)
{
  static int    hdr = 0;                           // header bytes matched so far
  static size_t pos = 0;                           // payload bytes received so far
  static uint8_t payload[sizeof(SIncomingData)];

  if (hdr < 3)
  {
    if (b == HEADER[hdr]) hdr++;
    else hdr = (b == HEADER[0]) ? 1 : 0;           // resync
    
    return false;
  }

  payload[pos++] = b;
  if (pos == sizeof(payload))
  {
    memcpy(&InData, payload, sizeof(payload));    // memcpy avoids alignment problems
    hdr = 0;
    pos = 0;
    return true;
  }

  return false;
}

/******************************************************************/
// Non-blocking: returns true when a complete package has arrived.
/******************************************************************/
static bool PollSerial(void)
{
  uint8_t buf[64];
  int n;

  while ((n = usb_serial_jtag_read_bytes(buf, sizeof(buf), 0)) > 0)
  {
    for (int i = 0; i < n; i++)
    {
      if (ParseByte(buf[i])) return true;
    }
  }
  
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// LVGL support functions                                                 ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**********************************************/
// Display
/**********************************************/
//#define TFT_ROTATION   LV_DISPLAY_ROTATION_0
//#define TFT_ROTATION   LV_DISPLAY_ROTATION_90
//#define TFT_ROTATION   LV_DISPLAY_ROTATION_180
#define TFT_ROTATION   LV_DISPLAY_ROTATION_270

lv_display_t *disp;                             // handler for display device
lv_indev_t *indev;                              // handler for touch device
lv_obj_t *screen_gauges, *screen_carselection;  // handler for screen layouts

// LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes
#define LVGL_BUF_LINES 64
size_t draw_buffer_sz = LCD_VERTICAL_RESOLUTION * LVGL_BUF_LINES * 2;   // 800 px landscape width * 64 lines * RGB565 = 102400 bytes

void *rot_buf = NULL;

// ppa accelerated rotating flush function
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

    lv_area_t rotated_area = *area;
    if (rotation != LV_DISPLAY_ROTATION_0)
    {
      lv_color_format_t cf = lv_display_get_color_format(disp);
      lv_display_rotate_area(disp, &rotated_area);

      const uint32_t src_w = lv_area_get_width(area);
      const uint32_t src_h = lv_area_get_height(area);
      const uint32_t dst_w = lv_area_get_width(&rotated_area);
      const uint32_t dst_h = lv_area_get_height(&rotated_area);
      const uint32_t src_stride_px = lv_draw_buf_width_to_stride(src_w, cf) / 2;   // RGB565 = 2 bytes/px

      ppa_srm_rotation_angle_t angle;

      switch (rotation)
      {
        case LV_DISPLAY_ROTATION_90:
          angle = PPA_SRM_ROTATION_ANGLE_90;
          break;

        case LV_DISPLAY_ROTATION_180:
          angle = PPA_SRM_ROTATION_ANGLE_180;
          break;
            
        case LV_DISPLAY_ROTATION_270:
          angle = PPA_SRM_ROTATION_ANGLE_270;
          break;
            
        default:
          angle = PPA_SRM_ROTATION_ANGLE_0;
          break;
      }

      ppa_srm_oper_config_t srm = {
          .in = {
              .buffer = px_map,
              .pic_w = src_stride_px,
              .pic_h = src_h,
              .block_w = src_w,
              .block_h = src_h,
              .block_offset_x = 0,
              .block_offset_y = 0,
              .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
          },
          .out = {
              .buffer = rot_buf,
              .buffer_size = draw_buffer_sz,       // must be cache-line aligned
              .pic_w = dst_w,
              .pic_h = dst_h,
              .block_offset_x = 0,
              .block_offset_y = 0,
              .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
           },
          .rotation_angle = angle,
          .scale_x = 1.0f,
          .scale_y = 1.0f,
          .mirror_x = false,
          .mirror_y = false,
          .rgb_swap = false,
          .byte_swap = false,
          .mode = PPA_TRANS_MODE_BLOCKING,
        };
        
      ESP_ERROR_CHECK(ppa_do_scale_rotate_mirror(ppa_srm_client, &srm));
      px_map = rot_buf;
    }

    esp_lcd_panel_draw_bitmap(panel_handle, rotated_area.x1, rotated_area.y1, rotated_area.x2 + 1, rotated_area.y2 + 1, px_map); 
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

#define ALIGN_UP(num, align)    (((num) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(num, align)  ((num) & ~((align) - 1))

void rounder_flush_area_cb(lv_event_t * event)
{
    lv_area_t * area = lv_event_get_invalidated_area(event);
    area->x1 = ALIGN_DOWN(area->x1, 16);
    area->x2 = ALIGN_UP(area->x2, 16) - 1;
}

/**********************************************/
// Tick source in millisec
/**********************************************/
static uint32_t my_tick(void)
{
  return esp_timer_get_time() / 1000;
}

/**********************************************/
// Touchpad
/**********************************************/
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
  // Read data from touch controller
  esp_lcd_touch_read_data(touch_handle);
  
  esp_lcd_touch_point_data_t touch_point_data[1]; 
  uint8_t touch_cnt = 0;

  // Read only the first pressed point, GT911 can read up to 5 simultanous touch points
  ESP_ERROR_CHECK(esp_lcd_touch_get_data(touch_handle, touch_point_data, &touch_cnt, 1));

  if (touch_cnt > 0)
  {

    data->point.x = (int32_t) touch_point_data[0].x;
    data->point.y = (int32_t) touch_point_data[0].y;
    data->state = LV_INDEV_STATE_PRESSED;
    printf("Touch at: X=%d, Y=%d, Strength=%d\n", touch_point_data[0].x, touch_point_data[0].y, touch_point_data[0].strength);
  }
  else data->state = LV_INDEV_STATE_RELEASED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// LVGL colors and styles                                                 ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// color shading for engine management lights
// values between 0 - 255; 255 means black, 0 means original color
#define WARNINGLIGHT_ON 0
#define WARNINGLIGHT_OFF 170

lv_color_t color_black, color_yellow, color_red, color_green, color_darkgreen, color_darkgrey, color_blue;
lv_style_t style_backgroundline, style_SLI, style_RPM;

void SetupColorsAndStyles()
{
  //lv_color_make(RED, GREEN, BLUE)

  color_green =     lv_color_make(80,  200, 80);
  color_darkgreen = lv_color_make(40,  150, 40);
  color_yellow =    lv_color_make(255, 240,  0);
  color_red =       lv_color_make(255, 50,  50);
  color_black =     lv_color_make(0,   0,    0);
  color_darkgrey =  lv_color_make(32,  32,  32);
  color_blue =      lv_color_make(79, 144, 209);

  // Setup styles
  lv_style_init(&style_backgroundline);
  lv_style_set_line_width(&style_backgroundline, 6);
  lv_style_set_line_color(&style_backgroundline, color_darkgreen);
  lv_style_set_line_rounded(&style_backgroundline, false);

  lv_style_init(&style_SLI);
  lv_style_set_border_color(&style_SLI, color_black);

  lv_style_init(&style_RPM);
  lv_style_set_radius(&style_RPM, 10);

  // Initialize fonts
  LV_FONT_DECLARE(font_gear);
  LV_FONT_DECLARE(font_messages);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Gauge variables                                                        ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// bit fields of engine warnings for reference
/*enum irsdk_EngineWarnings 
{
  irsdk_waterTempWarning    = 0x01,
  irsdk_fuelPressureWarning = 0x02,
  irsdk_oilPressureWarning  = 0x04,
  irsdk_engineStalled     = 0x08,
  irsdk_pitSpeedLimiter   = 0x10,
  irsdk_revLimiterActive    = 0x20,
}*/

// structure to store the actual and the previous screen data
typedef struct
{
  char EngineWarnings;
  int Fuel;
  char Gear;
  int RPMgauge;
  int Speed;
  int SLI;
  int WaterTemp;
} SViewData;

unsigned char ActiveCar;

// gauge screen background
lv_obj_t *backgroundline1, *backgroundline2;

// RPM bar
lv_obj_t *RPMbar;

// SLI
lv_obj_t *SLI1, *SLI2, *SLI3, *SLI4, *SLI5, *SLI6, *SLI7, *SLI8;
int64_t BlinkStart;
bool BlinkStatus;

// Engine management lights
lv_image_dsc_t img_fuelpressure, img_oilpressure, img_stall, img_watertemp;
lv_obj_t *icon_fuelpressure, *icon_oilpressure, *icon_stall, *icon_watertemp;

// Gear number indicator
lv_obj_t *gear;

// fuel, water, speed numbers
lv_obj_t *fuel_static_text1, *speed_static_text1, *water_static_text1;
lv_obj_t *fuel_static_text2, *speed_static_text2, *water_static_text2;
lv_obj_t *fuel_value_text, *speed_value_text, *water_value_text;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Gauge positions                                                        ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define FUELX 0
#define FUELY 270
#define SPEEDX 0
#define SPEEDY 180
#define WATERX 0
#define WATERY 320
#define DELTA1 260  // offset for the unit
#define DELTA2 170  // offset for the actual value

#define GEARX 630
#define GEARY 180

#define RPMY 80

#define SLIY 10
#define SLIBLINKRATE 300000  // how long the light will stay on or off in microseconds

#define EML_FUELPRESSX 0
#define EML_FUELPRESSY 400
#define EML_OILPRESSX 80
#define EML_OILPRESSY 400
#define EML_WATERTEMPX 160
#define EML_WATERTEMPY 400
#define EML_STALLX 240
#define EML_STALLY 400

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Car profile selection screen - part 1 of 2                             ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// car button matrix
lv_obj_t *carselectionmatrix[NUMOFCARS];
lv_obj_t *carselectionmatrix_label[NUMOFCARS];

// car selection menu button
lv_obj_t *actualcarbutton;
lv_obj_t *actualcarbutton_label;

void DrawCarSelectionMenu()
{
  lv_screen_load(screen_carselection);
}

// LVGL event handler for actual car button
static void actualcar_handler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);

  switch (code)
  {
    case LV_EVENT_CLICKED:
      DrawCarSelectionMenu();
      break;

    default:
        break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Gauge screen background and instruments                                ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**********************************************/
// Draw the background for the instruments
/**********************************************/
void SetupGaugesScreen()
{
  screen_gauges = lv_obj_create(NULL);
  
  // Create an array for the points of the background lines
  static lv_point_precise_t line_points1[] = { {0, 150}, {799, 150} };
  static lv_point_precise_t line_points2[] = { {0, 390}, {799, 400} };

  // set background color
  lv_obj_set_style_bg_color(screen_gauges, color_black, 0);

  // Draw background lines and apply the new style
  backgroundline1 = lv_line_create(screen_gauges);
  backgroundline2 = lv_line_create(screen_gauges);

  lv_line_set_points(backgroundline1, line_points1, 2);
  lv_line_set_points(backgroundline2, line_points2, 2);
  lv_obj_add_style(backgroundline1, &style_backgroundline, 0);
  lv_obj_add_style(backgroundline2, &style_backgroundline, 0);

  // Draw button for car selection menu
  actualcarbutton = lv_button_create(screen_gauges);
  actualcarbutton_label = lv_label_create(actualcarbutton);
  lv_obj_center(actualcarbutton_label);

  // Setup the car selection menu button
  lv_obj_add_event_cb(actualcarbutton, actualcar_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_press_lock(actualcarbutton, false);
  lv_obj_set_size(actualcarbutton, 100, 50);
  lv_obj_set_pos(actualcarbutton, 700, 420);
  lv_obj_set_style_text_color(actualcarbutton, color_black, 0);
}

// customize background for the actual car profile
void DrawGaugesScreen(unsigned char ID)
{
  // display the name of actual profile on car selection menu button
  lv_label_set_text(actualcarbutton_label, CarProfile[ID].CarName);
  lv_screen_load(screen_gauges);
}

/**********************************************/
// Draw RPM gauge
/**********************************************/
void SetupRPM()
{
  RPMbar = lv_bar_create(screen_gauges);

  lv_obj_set_size(RPMbar, 800, 50);
  lv_obj_set_pos(RPMbar, 0, RPMY);
  lv_bar_set_range(RPMbar, 0, CarProfile[DEFAULTCAR].RPM);
  lv_bar_set_value(RPMbar, 3000, LV_ANIM_OFF);
  
  lv_obj_add_style(RPMbar, &style_RPM, 0);
  lv_obj_add_style(RPMbar, &style_RPM, LV_PART_INDICATOR);
}

void AdjustMaxRPM(unsigned char ID)
{
  lv_bar_set_range(RPMbar, 0, CarProfile[ID].RPM);
}

void DrawRPM(int RPM, int RPMPrev)
{
  lv_bar_set_value(RPMbar, RPM, LV_ANIM_OFF);
}

/**********************************************/
// Draw shift light indicator
/**********************************************/
void SetupSLI()
{
  // draw rectangles
  SLI1 = lv_obj_create(screen_gauges);
  SLI2 = lv_obj_create(screen_gauges);
  SLI3 = lv_obj_create(screen_gauges);
  SLI4 = lv_obj_create(screen_gauges);
  SLI5 = lv_obj_create(screen_gauges);
  SLI6 = lv_obj_create(screen_gauges);
  SLI7 = lv_obj_create(screen_gauges);
  SLI8 = lv_obj_create(screen_gauges);

  lv_obj_set_style_bg_color(SLI1, color_darkgrey, 0);
  lv_obj_set_size(SLI1 , 100, 50);
  lv_obj_set_pos(SLI1 , 0, SLIY);
  lv_obj_add_style(SLI1, &style_SLI, 0);
  
  lv_obj_set_style_bg_color(SLI2, color_darkgrey, 0);
  lv_obj_set_size(SLI2 , 100, 50);
  lv_obj_set_pos(SLI2 , 100, SLIY);
  lv_obj_add_style(SLI2, &style_SLI, 0);

  lv_obj_set_style_bg_color(SLI3, color_darkgrey, 0);
  lv_obj_set_size(SLI3 , 100, 50);
  lv_obj_set_pos(SLI3 , 200, SLIY);
  lv_obj_add_style(SLI3, &style_SLI, 0);

  lv_obj_set_style_bg_color(SLI4, color_darkgrey, 0);
  lv_obj_set_size(SLI4 , 100, 50);
  lv_obj_set_pos(SLI4 , 300, SLIY);
  lv_obj_add_style(SLI4, &style_SLI, 0);

  lv_obj_set_style_bg_color(SLI5, color_darkgrey, 0);
  lv_obj_set_size(SLI5 , 100, 50);
  lv_obj_set_pos(SLI5 , 400, SLIY);
  lv_obj_add_style(SLI5, &style_SLI, 0);

  lv_obj_set_style_bg_color(SLI6, color_darkgrey, 0);
  lv_obj_set_size(SLI6 , 100, 50);
  lv_obj_set_pos(SLI6 , 500, SLIY);
  lv_obj_add_style(SLI6, &style_SLI, 0);

  lv_obj_set_style_bg_color(SLI7, color_darkgrey, 0);
  lv_obj_set_size(SLI7 , 100, 50);
  lv_obj_set_pos(SLI7 , 600, SLIY);
  lv_obj_add_style(SLI7, &style_SLI, 0);

  lv_obj_set_style_bg_color(SLI8, color_darkgrey, 0);
  lv_obj_set_size(SLI8 , 100, 50);
  lv_obj_set_pos(SLI8 , 700, SLIY);
  lv_obj_add_style(SLI8, &style_SLI, 0);
}

void DrawSLI(int SLI, int SLIPrev, char Limiter, char LimiterPrev)
{
  int64_t Timer;

  if (Limiter == 0) // limiter is off
  {
    if (LimiterPrev != 0) // limiter was just switched off, color all indicator to current state
    {
      lv_obj_set_style_bg_color(SLI1, color_darkgrey, 0);
      lv_obj_set_style_bg_color(SLI2, color_darkgrey, 0);
      lv_obj_set_style_bg_color(SLI7, color_darkgrey, 0);
      lv_obj_set_style_bg_color(SLI8, color_darkgrey, 0);

      for (int i=0; i<= SLI; i++)
      {
        switch (i)
        {
          case 1: lv_obj_set_style_bg_color(SLI1, color_green, 0);
                  break;
          case 2: lv_obj_set_style_bg_color(SLI2, color_green, 0);
                  break;
          case 3: lv_obj_set_style_bg_color(SLI3, color_green, 0);
                  break;
          case 4: lv_obj_set_style_bg_color(SLI4, color_green, 0);
                  break;
          case 5: lv_obj_set_style_bg_color(SLI5, color_yellow, 0);
                  break;
          case 6: lv_obj_set_style_bg_color(SLI6, color_yellow, 0);
                  break;
          case 7: lv_obj_set_style_bg_color(SLI7, color_red, 0);
                  break;
          case 8: lv_obj_set_style_bg_color(SLI8, color_red, 0);
                  break;
        }
      }
    }
    
    if (SLI < SLIPrev)  // clear only the disappeared indicators
    {
      for (int i=SLI; i<=SLIPrev-1; i++)
      {
        switch (i)
        {
          case 0: lv_obj_set_style_bg_color(SLI1, color_darkgrey, 0);
                  break;
          case 1: lv_obj_set_style_bg_color(SLI2, color_darkgrey, 0);
                  break;
          case 2: lv_obj_set_style_bg_color(SLI3, color_darkgrey, 0);
                  break;
          case 3: lv_obj_set_style_bg_color(SLI4, color_darkgrey, 0);
                  break;
          case 4: lv_obj_set_style_bg_color(SLI5, color_darkgrey, 0);
                  break;
          case 5: lv_obj_set_style_bg_color(SLI6, color_darkgrey, 0);
                  break;
          case 6: lv_obj_set_style_bg_color(SLI7, color_darkgrey, 0);
                  break;
          case 7: lv_obj_set_style_bg_color(SLI8, color_darkgrey, 0);
                  break;                
        }
      }
    }
    
    if (SLI > SLIPrev) // draw only the appeared indicators
    {
      for (int i=SLIPrev+1; i<= SLI; i++)
       {
        switch (i)
        {
          case 1: lv_obj_set_style_bg_color(SLI1, color_green, 0);
                  break;
          case 2: lv_obj_set_style_bg_color(SLI2, color_green, 0);
                  break;
          case 3: lv_obj_set_style_bg_color(SLI3, color_green, 0);
                  break;
          case 4: lv_obj_set_style_bg_color(SLI4, color_green, 0);
                  break;
          case 5: lv_obj_set_style_bg_color(SLI5, color_yellow, 0);
                  break;
          case 6: lv_obj_set_style_bg_color(SLI6, color_yellow, 0);
                  break;
          case 7: lv_obj_set_style_bg_color(SLI7, color_red, 0);
                  break;
          case 8: lv_obj_set_style_bg_color(SLI8, color_red, 0);
                  break;
      }
     }
    }
  }
  else // limiter is on
  {
    Timer = esp_timer_get_time();

    if (LimiterPrev == 0) // limiter was just switched on
    {
      BlinkStart = Timer;
      BlinkStatus = true;
      lv_obj_set_style_bg_color(SLI1, color_blue, 0);
      lv_obj_set_style_bg_color(SLI2, color_blue, 0);
      lv_obj_set_style_bg_color(SLI3, color_darkgrey, 0);
      lv_obj_set_style_bg_color(SLI4, color_darkgrey, 0);
      lv_obj_set_style_bg_color(SLI5, color_darkgrey, 0);
      lv_obj_set_style_bg_color(SLI6, color_darkgrey, 0);

      lv_obj_set_style_bg_color(SLI7, color_blue, 0);
      lv_obj_set_style_bg_color(SLI8, color_blue, 0);
    }

    // Check if enough time has elapsed and then do the blinking
    if ((Timer - BlinkStart) > SLIBLINKRATE)
    {
      BlinkStart = Timer;
      if (BlinkStatus == true)
      {
        BlinkStatus = false;
        lv_obj_set_style_bg_color(SLI1, color_darkgrey, 0);
        lv_obj_set_style_bg_color(SLI2, color_darkgrey, 0);
        lv_obj_set_style_bg_color(SLI7, color_darkgrey, 0);
        lv_obj_set_style_bg_color(SLI8, color_darkgrey, 0);
      }
      else
      {
        BlinkStatus = true;
        lv_obj_set_style_bg_color(SLI1, color_blue, 0);
        lv_obj_set_style_bg_color(SLI2, color_blue, 0);
        lv_obj_set_style_bg_color(SLI7, color_blue, 0);
        lv_obj_set_style_bg_color(SLI8, color_blue, 0);
      }
    }
  }
}

/**********************************************/
// Draw actual gear number
/**********************************************/
void SetupGear()
{
  gear = lv_label_create(screen_gauges);
  lv_label_set_text(gear, "N");
  lv_obj_set_pos(gear, GEARX, GEARY);
  
  lv_obj_set_style_text_font(gear, &font_gear, 0);
  lv_obj_set_style_text_color(gear, color_green, 0);
}

void DrawGear(signed char GearNum, signed char GearNumPrev)
{
  if (GearNum != GearNumPrev)
  {
    switch (GearNum)
    {
      case -1:
        lv_label_set_text(gear, "R");
        break;
      case 0:
        lv_label_set_text(gear, "N");
        break;
      default:
        lv_label_set_text_fmt(gear, "%d", GearNum);
        break;
    }
  }
}

/**********************************************/
// Draw the engine warning icons
/**********************************************/
void SetupEngineWarnings()
{
  img_fuelpressure.header.cf = LV_COLOR_FORMAT_RGB565A8;
  img_fuelpressure.header.magic = LV_IMAGE_HEADER_MAGIC;
  img_fuelpressure.header.w = 80;
  img_fuelpressure.header.h = 80;
  img_fuelpressure.data_size = 6400 * 3;
  img_fuelpressure.data = fuelpressure_map;
  img_fuelpressure.header.flags = 0;
  img_fuelpressure.header.reserved_2 = 0;
  img_fuelpressure.header.stride = 0;
  img_fuelpressure.reserved = NULL;

  img_oilpressure.header.cf = LV_COLOR_FORMAT_RGB565A8;
  img_oilpressure.header.magic = LV_IMAGE_HEADER_MAGIC;
  img_oilpressure.header.w = 80;
  img_oilpressure.header.h = 80;
  img_oilpressure.data_size = 6400 * 3;
  img_oilpressure.data = oilpressure_map;
  img_oilpressure.header.flags = 0;
  img_oilpressure.header.reserved_2 = 0;
  img_oilpressure.header.stride = 0;
  img_oilpressure.reserved = NULL;

  img_stall.header.cf = LV_COLOR_FORMAT_RGB565A8;
  img_stall.header.magic = LV_IMAGE_HEADER_MAGIC;
  img_stall.header.w = 80;
  img_stall.header.h = 80;
  img_stall.data_size = 6400 * 3;
  img_stall.data = stall_map;
  img_stall.header.flags = 0;
  img_stall.header.reserved_2 = 0;
  img_stall.header.stride = 0;
  img_stall.reserved = NULL;

  img_watertemp.header.cf = LV_COLOR_FORMAT_RGB565A8;
  img_watertemp.header.magic = LV_IMAGE_HEADER_MAGIC;
  img_watertemp.header.w = 80;
  img_watertemp.header.h = 80;
  img_watertemp.data_size = 6400 * 3;
  img_watertemp.data = watertemp_map;
  img_watertemp.header.flags = 0;
  img_watertemp.header.reserved_2 = 0;
  img_watertemp.header.stride = 0;
  img_watertemp.reserved = NULL;

  icon_fuelpressure = lv_image_create(screen_gauges);
  lv_image_set_src(icon_fuelpressure, &img_fuelpressure);
  lv_obj_set_pos(icon_fuelpressure, EML_FUELPRESSX, EML_FUELPRESSY);

  icon_oilpressure = lv_image_create(screen_gauges);
  lv_image_set_src(icon_oilpressure, &img_oilpressure);
  lv_obj_set_pos(icon_oilpressure, EML_OILPRESSX, EML_OILPRESSY);

  icon_watertemp = lv_image_create(screen_gauges);
  lv_image_set_src(icon_watertemp, &img_watertemp);
  lv_obj_set_pos(icon_watertemp, EML_WATERTEMPX, EML_WATERTEMPY);

  icon_stall = lv_image_create(screen_gauges);
  lv_image_set_src(icon_stall, &img_stall);
  lv_obj_set_pos(icon_stall, EML_STALLX, EML_STALLY);

  lv_obj_set_style_image_recolor_opa(icon_watertemp, WARNINGLIGHT_OFF, 0);
  lv_obj_set_style_image_recolor_opa(icon_stall, WARNINGLIGHT_OFF, 0);
}

void DrawEngineWarnings(char Warning, char WarningPrev)
{
  char Filtered, FilteredPrev;

  // set water temp light
  Filtered = Warning & 0x01;
  FilteredPrev = WarningPrev & 0x01;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) lv_obj_set_style_image_recolor_opa(icon_watertemp, WARNINGLIGHT_ON, 0);
    else lv_obj_set_style_image_recolor_opa(icon_watertemp, WARNINGLIGHT_OFF, 0);
  }

  // set fuel pressure light
  Filtered = Warning & 0x02;
  FilteredPrev = WarningPrev & 0x02;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) lv_obj_set_style_image_recolor_opa(icon_fuelpressure, WARNINGLIGHT_ON, 0);
    else lv_obj_set_style_image_recolor_opa(icon_fuelpressure, WARNINGLIGHT_OFF, 0);
  }

  // set oil pressure light
  Filtered = Warning & 0x04;
  FilteredPrev = WarningPrev & 0x04;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) lv_obj_set_style_image_recolor_opa(icon_oilpressure, WARNINGLIGHT_ON, 0);
    else lv_obj_set_style_image_recolor_opa(icon_oilpressure, WARNINGLIGHT_OFF, 0);
  }

  // set stall sign light
  Filtered = Warning & 0x08;
  FilteredPrev = WarningPrev & 0x08;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) lv_obj_set_style_image_recolor_opa(icon_stall, WARNINGLIGHT_ON, 0);
    else lv_obj_set_style_image_recolor_opa(icon_stall, WARNINGLIGHT_OFF, 0);
  }

  // draw limiter sign
  //Filtered = Warning & 0x10;
  //FilteredPrev = WarningPrev & 0x10;
}

/**********************************************/
// Draw water temperature gauge
/**********************************************/
void SetupWater()
{
  water_static_text1 = lv_label_create(screen_gauges);
  water_static_text2 = lv_label_create(screen_gauges);
  water_value_text = lv_label_create(screen_gauges);
  lv_label_set_text(water_static_text1, "Water:");
  lv_label_set_text(water_static_text2, "C");
  lv_label_set_text(water_value_text, "0");
  lv_obj_set_pos(water_static_text1, WATERX, WATERY);
  lv_obj_set_pos(water_static_text2, WATERX + DELTA1, WATERY);
  lv_obj_set_pos(water_value_text, WATERX + DELTA2, WATERY);
  
  lv_obj_set_style_text_font(water_static_text1, &font_messages, 0);
  lv_obj_set_style_text_font(water_static_text2, &font_messages, 0);
  lv_obj_set_style_text_font(water_value_text, &font_messages, 0);
  lv_obj_set_style_text_color(water_static_text1, color_green, 0);
  lv_obj_set_style_text_color(water_static_text2, color_green, 0);
  lv_obj_set_style_text_color(water_value_text, color_green, 0);
}

void DrawWater(int Water, int WaterPrev)
{
  if (Water != WaterPrev)
  { 
    if (Water >= CarProfile[ActiveCar].WaterTemp ) lv_obj_set_style_text_color(water_value_text, color_red, 0);
    else lv_obj_set_style_text_color(water_value_text, color_green, 0);
    lv_label_set_text_fmt(water_value_text, "%d", Water);
  }
}

/**********************************************/
// Draw fuel gauge
/**********************************************/
void SetupFuel()
{
  fuel_static_text1 = lv_label_create(screen_gauges);
  fuel_static_text2 = lv_label_create(screen_gauges);
  fuel_value_text = lv_label_create(screen_gauges);
  lv_label_set_text(fuel_static_text1, "Fuel:");
  lv_label_set_text(fuel_static_text2, "L");
  lv_label_set_text(fuel_value_text, "0.0");
  lv_obj_set_pos(fuel_static_text1, FUELX, FUELY);
  lv_obj_set_pos(fuel_static_text2, FUELX + DELTA1, FUELY);
  lv_obj_set_pos(fuel_value_text, FUELX + DELTA2, FUELY);
  
  lv_obj_set_style_text_font(fuel_static_text1, &font_messages, 0);
  lv_obj_set_style_text_font(fuel_static_text2, &font_messages, 0);
  lv_obj_set_style_text_font(fuel_value_text, &font_messages, 0);
  lv_obj_set_style_text_color(fuel_static_text1, color_green, 0);
  lv_obj_set_style_text_color(fuel_static_text2, color_green, 0);
  lv_obj_set_style_text_color(fuel_value_text, color_green, 0);
}

void DrawFuel(int Fuel, int FuelPrev)
{
  // received value is multiplied by 10 to keep the first fraction
  if(Fuel != FuelPrev)
  {
    if (Fuel <= CarProfile[ActiveCar].Fuel) lv_obj_set_style_text_color(fuel_value_text, color_red, 0);
    else lv_obj_set_style_text_color(fuel_value_text, color_green, 0);
    lv_label_set_text_fmt(fuel_value_text, "%d.%d", Fuel / 10, Fuel % 10);
  }
}

/**********************************************/
// Draw speed gauge
/**********************************************/
void SetupSpeed()
{
  speed_static_text1 = lv_label_create(screen_gauges);
  speed_static_text2 = lv_label_create(screen_gauges);
  speed_value_text = lv_label_create(screen_gauges);
  lv_label_set_text(speed_static_text1, "Speed:");
  lv_label_set_text(speed_static_text2, "Km/h");
  lv_label_set_text(speed_value_text, "0");
  lv_obj_set_pos(speed_static_text1, SPEEDX, SPEEDY);
  lv_obj_set_pos(speed_static_text2, SPEEDX + DELTA1, SPEEDY);
  lv_obj_set_pos(speed_value_text, SPEEDX + DELTA2, SPEEDY);
  
  lv_obj_set_style_text_font(speed_static_text1, &font_messages, 0);
  lv_obj_set_style_text_font(speed_static_text2, &font_messages, 0);
  lv_obj_set_style_text_font(speed_value_text, &font_messages, 0);
  lv_obj_set_style_text_color(speed_static_text1, color_green, 0);
  lv_obj_set_style_text_color(speed_static_text2, color_green, 0);
  lv_obj_set_style_text_color(speed_value_text, color_green, 0);
}

void DrawSpeed(int Speed, int SpeedPrev)
{
  if (Speed != SpeedPrev) lv_label_set_text_fmt(speed_value_text, "%d", Speed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Car profile selection screen - part 2 of 2                             ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// LVGL event handler for car button matrix
static void carselectionmatrix_handler(lv_event_t * e)
{
  lv_obj_t *pressedbutton;
  char *ID;
  
  lv_event_code_t code = lv_event_get_code(e);
  switch (code)
  {
    case LV_EVENT_CLICKED:
      pressedbutton = (lv_obj_t*)lv_event_get_current_target(e);
      ID = (char*)lv_obj_get_user_data(pressedbutton);     // get the car ID from the pressed button
      
      ActiveCar = *ID;
      AdjustMaxRPM(ActiveCar);
      DrawGaugesScreen(ActiveCar);
      break;

    default:
        break;
  }
}

void SetupCarSelectionMenu()
{
  screen_carselection = lv_obj_create(NULL);

  // set background color
  lv_obj_set_style_bg_color(screen_carselection, color_black, 0);

  for (int i=0; i<NUMOFCARS; i++)
  {
    carselectionmatrix[i] = lv_button_create(screen_carselection);
    lv_obj_set_user_data(carselectionmatrix[i], &CarProfile[i].ID);     // link the car ID to the button
    carselectionmatrix_label[i] = lv_label_create(carselectionmatrix[i]);
    
    lv_obj_center(carselectionmatrix_label[i]);
    lv_label_set_text(carselectionmatrix_label[i], CarProfile[i].CarName);

    // Setup the buttons
    lv_obj_add_event_cb(carselectionmatrix[i], carselectionmatrix_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_press_lock(carselectionmatrix[i], false);
    lv_obj_set_size(carselectionmatrix[i], 100, 50);
    lv_obj_set_pos(carselectionmatrix[i], ((i%7)*110)+20, ((i/7)*60)+20);
    lv_obj_set_style_text_color(carselectionmatrix[i], color_black, 0);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Hardware and LVGL initial setup                                        ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  bool err = false;

  ESP_LOGI(TAG_IRDASH, "setup() started");
  
/******************************************************************/
// ESP-IDF: Print chip information
/******************************************************************/    
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  esp_chip_info(&chip_info);
  printf("ESP-IDF: This is %s chip with %d CPU core(s), %s%s%s%s, ",
         CONFIG_IDF_TARGET,
         chip_info.cores,
         (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
         (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
         (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  printf("silicon revision v%d.%d, ", major_rev, minor_rev);
  if(esp_flash_get_size(NULL, &flash_size) != ESP_OK)
  {
    ESP_LOGI(TAG_ESP_IDF, "Get flash size failed");
    err = true;
  }

  if (err == false)
  {
    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

      printf("ESP-IDF: Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
  }

/******************************************************************/
// ESP_IDF: Power up MIPI PHY
/******************************************************************/
  if (err == false)
  {
     esp_err_t LDO_err;
    static esp_ldo_channel_handle_t phy_pwr_chan = NULL;

    // Initialize LDO
    // Ensure MIPI DPHY is powered before creating the bus
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };

    // Power up MIPI DSI PHY
    LDO_err = esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr_chan);
    if (LDO_err == ESP_OK) ESP_LOGI(TAG_ESP_IDF, "MIPI DSI PHY powered on");
    else
    {
        err = true;
        ESP_LOGI(TAG_ESP_IDF, "MIPI DSI PHY powering failed");
    }
  
    // Give PHY a moment to stabilize before DBI transfers
    vTaskDelay(pdMS_TO_TICKS(50));
  }

/******************************************************************/
// ESP-IDF: Initialize ST7701 and DSI bus
/******************************************************************/
  esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
  esp_lcd_panel_io_handle_t mipi_dbi_io;
  esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

  if (err == false)
  {
      // DSI bus and DBI IO for ST7701
      esp_lcd_dsi_bus_config_t bus_config = {
          .bus_id = 0,
          .num_data_lanes = LCD_DSI_LANE_NUM,
          .lane_bit_rate_mbps = LCD_DSI_LANE_RATE_MBPS
        };

      ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));
      // Give PHY a moment to stabilize before DBI transfers
      vTaskDelay(pdMS_TO_TICKS(50));
  }

  if (err == false)
  {
      // we use DBI interface to send LCD commands and parameters
      esp_lcd_dbi_io_config_t dbi_config = {
          .virtual_channel = 0,
          .lcd_cmd_bits = 8,    // according to the LCD spec
          .lcd_param_bits = 8,  // according to the LCD spec
      };
      ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io));
  }

  ESP_LOGI(TAG_ESP_IDF, "Install MIPI DSI LCD data panel");
  esp_lcd_dpi_panel_config_t dpi_config = {
      .virtual_channel = LCD_DPI_VIRTUAL_CHANNEL,
      .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
      .dpi_clock_freq_mhz = LCD_DPI_CLK_MHZ,
      .in_color_format = LCD_DPI_IN_PIXEL_FORMAT,
      .num_fbs = LCD_DPI_NUM_FB,
      .video_timing = {
          .h_size = LCD_HORIZONTAL_RESOLUTION,
          .v_size = LCD_VERTICAL_RESOLUTION,
          .hsync_pulse_width = LCD_DPI_TIMINGS_HPW,
          .hsync_back_porch = LCD_DPI_TIMINGS_HBP,
          .hsync_front_porch = LCD_DPI_TIMINGS_HFP,
          .vsync_pulse_width = LCD_DPI_TIMINGS_VPW,
          .vsync_back_porch = LCD_DPI_TIMINGS_VBP,
          .vsync_front_porch = LCD_DPI_TIMINGS_VFP,
      }
  };

  st7701_vendor_config_t vendor_config = {
      .mipi_config = {
          .dsi_bus = mipi_dsi_bus,
          .dpi_config = &dpi_config,
      },
      .init_cmds = st7701_lcd_cmds,
      .init_cmds_size = sizeof(st7701_lcd_cmds) / sizeof(st7701_lcd_cmds[0]),
      .flags = { .use_mipi_interface = 1 }
  };
    
  esp_lcd_panel_dev_config_t lcd_dev_config = {
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = LCD_DATA_WIDTH,
      .reset_gpio_num = LCD_RST_IO,
      .vendor_config = &vendor_config,
  };

  if (err == false)
  {
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(mipi_dbi_io, &lcd_dev_config, &mipi_dpi_panel));

    // use DMA2D to copy draw buffer into frame buffer
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_enable_dma2d(mipi_dpi_panel));
    ESP_LOGI(TAG_ESP_IDF, "DPI panel added DMA2D draw bitmap hook");

    ESP_ERROR_CHECK(esp_lcd_panel_reset(mipi_dpi_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(mipi_dpi_panel));
    // Ensure panel display is turned on (some sequences rely on explicit ON)
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(mipi_dpi_panel, true));
  }

/******************************************************************/
// ESP_IDF: initialize pixel-processing accelerator(s)
/******************************************************************/  
  ppa_init_srm();
  ESP_LOGI(TAG_ESP_IDF, "DPI panel added SRM pixel processing accelerator");

/******************************************************************/
// ESP-IDF: Initialize GT911 touch device
/******************************************************************/
init_touch_controller();
ESP_LOGI(TAG_ESP_IDF, "GT911 touch controller initialized");

/******************************************************************/
// LVGL: Initialize library
/******************************************************************/
  if (err == false)  
  {
    ESP_LOGI(TAG_LVGL, "Library V%d.%d.%d", lv_version_major(), lv_version_minor(), lv_version_patch());
    lv_init();

    // Set a tick source so that LVGL will know how much time elapsed.
    lv_tick_set_cb(my_tick);
  }

/******************************************************************/
// LVGL: Create a display and set orientation
/******************************************************************/  
  if (err == false)
  {
    disp = lv_display_create(LCD_HORIZONTAL_RESOLUTION, LCD_VERTICAL_RESOLUTION);
    if (disp != NULL)
    {
        // associate the mipi panel handle to the display
        lv_display_set_user_data(disp, mipi_dpi_panel);
    
        // set color depth
        lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
      
        ESP_LOGI(TAG_LVGL, "Display init finished");
    }
    else
    {
      ESP_LOGI(TAG_LVGL, "Display init failed");
      err = true;
    }
  }

  ESP_LOGI(TAG_LVGL, "Register event callback for LVGL flush area rounding");
  lv_display_add_event_cb(disp, rounder_flush_area_cb, LV_EVENT_INVALIDATE_AREA, NULL);

  lv_display_set_rotation(disp, TFT_ROTATION);

/******************************************************************/
// LVGL: Initialize draw buffers
/******************************************************************/  
  //size_t alignment = SOC_GDMA_EXT_MEM_ENC_ALIGNMENT; // SOC_GDMA_EXT_MEM_ENC_ALIGNMENT equals to 16
  size_t alignment = 64;  // needs 64 to function correctly
  void *buf1 = NULL;
  void *buf2 = NULL;

  if (err == false)
  {
    buf1 = heap_caps_aligned_calloc(alignment, 1, draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf1);
    buf2 = heap_caps_aligned_calloc(alignment, 1, draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf2);

    lv_display_set_buffers(disp, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
  
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    ESP_LOGI(TAG_LVGL, "Register DPI panel event callback for LVGL flush ready notification");
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
          .on_color_trans_done = notify_lvgl_flush_ready,
        #if CONFIG_MONITOR_REFRESH_BY_GPIO
          .on_refresh_done = monitor_refresh_rate,
        #endif
      };
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(mipi_dpi_panel, &cbs, disp));
  }

  rot_buf = heap_caps_aligned_calloc(alignment, 1, draw_buffer_sz, MALLOC_CAP_SPIRAM);
  assert(rot_buf);

/******************************************************************/
// LVGL: Initialize the input device driver
/******************************************************************/
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); // Touchpad should have POINTER type
  lv_indev_set_read_cb(indev, my_touchpad_read);
  ESP_LOGI(TAG_LVGL, "Touch init finished");

  ESP_LOGI(TAG_IRDASH, "setup() finished");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// app_main()                                                             ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void app_main(void)
{
  SViewData Screen[2];          // store the actual and the previous screen data
  int rpm_int;
  unsigned char gear;

  setup();

  bsp_display_backlight_on();
  ESP_LOGI(TAG_IRDASH, "bsp_display_backlight_on() finished");

  SetupUSBJTAG();
  ESP_LOGI(TAG_ESP_IDF, "USB-SERIAL-JTAG communication initialized");

  // Reset internal data
  Screen[0].EngineWarnings = 0;
  Screen[0].Fuel = 0;
  Screen[0].Gear = -1;
  Screen[0].RPMgauge = 0;
  Screen[0].Speed = -1;
  Screen[0].SLI = 0;
  Screen[0].WaterTemp = 0;

  UploadCarProfiles();

  SetupColorsAndStyles();

  SetupGaugesScreen();
  SetupCarSelectionMenu();

  SetupRPM();
  SetupSLI();
  SetupGear();
  SetupEngineWarnings();
  SetupWater();
  SetupFuel();
  SetupSpeed();

  DrawGaugesScreen(DEFAULTCAR);
  ActiveCar = DEFAULTCAR;
  ESP_LOGI(TAG_IRDASH, "DrawGaugesScreen(DEFAULTCAR) finished");

/******************************************************************/
// Main loop
/******************************************************************/  
  while(1)
  {
    // check if data is available to process, execute when complete block has been received
    if (PollSerial())
    {
      // calculate and draw Engine Warning lights
      Screen[1].EngineWarnings = InData.EngineWarnings;
      if (Screen[0].EngineWarnings != Screen[1].EngineWarnings) DrawEngineWarnings(Screen[1].EngineWarnings, Screen[0].EngineWarnings);

      // calculate and draw RPM gauge
      Screen[1].RPMgauge = (int)InData.RPM;
      if (Screen[0].RPMgauge != Screen[1].RPMgauge) DrawRPM(Screen[1].RPMgauge, Screen[0].RPMgauge);

      // calculate and draw gear number
      Screen[1].Gear = InData.Gear;
      DrawGear(Screen[1].Gear, Screen[0].Gear);

      // calculate and draw fuel level gauge
      Screen[1].Fuel = (int)(InData.Fuel*10); // convert float data to int and keep the first digit of the fractional part also
      DrawFuel(Screen[1].Fuel, Screen[0].Fuel);

      // calculate and draw speed gauge
      Screen[1].Speed = (int)(InData.Speed*3.6);  // convert m/s to km/h
      DrawSpeed(Screen[1].Speed, Screen[0].Speed);

      // calculate and draw water temperature gauge
      Screen[1].WaterTemp = (int)InData.WaterTemp;
      DrawWater(Screen[1].WaterTemp, Screen[0].WaterTemp);
                
      // calculate and draw shift light indicator
      rpm_int = (int)InData.RPM;
      gear = InData.Gear+1;  // "-1" corresponds to reverse but index must start with "0"
      if (rpm_int <= CarProfile[ActiveCar].SLI[gear][0]) Screen[1].SLI = 0; // determine how many light to be activated for the current gear
      else
      {
        if (rpm_int > CarProfile[ActiveCar].SLI[gear][7]) Screen[1].SLI = 8;
          else if (rpm_int > CarProfile[ActiveCar].SLI[gear][6]) Screen[1].SLI = 7;
            else if (rpm_int > CarProfile[ActiveCar].SLI[gear][5]) Screen[1].SLI = 6;
              else if (rpm_int > CarProfile[ActiveCar].SLI[gear][4]) Screen[1].SLI = 5;
                else if (rpm_int > CarProfile[ActiveCar].SLI[gear][3]) Screen[1].SLI = 4;
                  else if (rpm_int > CarProfile[ActiveCar].SLI[gear][2]) Screen[1].SLI = 3;
                    else if (rpm_int > CarProfile[ActiveCar].SLI[gear][1]) Screen[1].SLI = 2;
                      else if (rpm_int > CarProfile[ActiveCar].SLI[gear][0]) Screen[1].SLI = 1;
      }
      DrawSLI(Screen[1].SLI, Screen[0].SLI, Screen[1].EngineWarnings & 0x10, Screen[0].EngineWarnings & 0x10);

      // update old screen data
      Screen[0].EngineWarnings = Screen[1].EngineWarnings;
      Screen[0].Fuel           = Screen[1].Fuel;
      Screen[0].Gear           = Screen[1].Gear;
      Screen[0].RPMgauge       = Screen[1].RPMgauge;
      Screen[0].Speed          = Screen[1].Speed;
      Screen[0].WaterTemp      = Screen[1].WaterTemp;
      Screen[0].SLI            = Screen[1].SLI;
    
      //uart_flush_input(PC_UART);        // drop whatever piled up meanwhile
      serial_flush_input();               // drop whatever piled up meanwhile
    }

    lv_timer_periodic_handler();        // use the sleep time automatically calculated by LVGL
  }

  // leftover from hello world example
  /*for (int i = 10; i >= 0; i--) {
      lv_timer_periodic_handler(); // use the sleep time automatically calculated by LVGL

      printf("Restarting in %d seconds...\n", i);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  printf("Restarting now.\n");
  fflush(stdout);
  esp_restart();*/
}
