#define LOGNAME "SISWI"

#include <hk.h>
#include <hk_fascade.h>

#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/gpio.h>

const char* NAME                   = "Furboporta";
const char* ACCESSORY_NAME         = "Furboporta";
const char* ACCESSORY_MANUFACTURER = "Furberie";
const char* ACCESSORY_MODEL        = "FRB-002";
const char* ACCESSORY_SERIAL_NR    = "0";
const char* ACCESSORY_REVISION     = "R1";

const int BUTTON_GPIO = 36;

#define LED 19
bool value = false;

// --

typedef struct {
  uint8_t pin;
  bool inverted;
  uint16_t history;
  uint32_t down_time;
  uint32_t next_long_time;
} debounce_t;

static const uint16_t DEBOUNCE_MASK = 0b1111000000111111;

static bool debounce_rose(debounce_t *d) {
    if ((d->history & DEBOUNCE_MASK) == 0b0000000000111111) {
        d->history = 0xffff;
        return 1;
    }
    return 0;
}

static bool debounce_fell(debounce_t *d) {
    if ((d->history & DEBOUNCE_MASK) == 0b1111000000000000) {
        d->history = 0x0000;
        return 1;
    }
    return 0;
}

void debounce_update(debounce_t *d, bool value) {
    d->history = (d->history << 1) | value;
}

bool debounce_down(debounce_t *d) {
    if (d->inverted) return debounce_fell(d);
    return debounce_rose(d);
}

bool debounce_up(debounce_t *d) {
    if (d->inverted) return debounce_rose(d);
    return debounce_fell(d);
}

// -- 

void on_identify()
{
    ESP_LOGI(LOGNAME, "Identify");
}

esp_err_t on_write(hk_mem *request)
{
    value = *((bool *)request->ptr);
    ESP_LOGI(LOGNAME, "Setting led to %d", value);
    //gpio_set_level(LED, !value);

    return ESP_OK;
}

esp_err_t on_read(hk_mem *response)
{
    hk_mem_append_buffer(response, (char *)&value, sizeof(bool));
    return ESP_OK;
}

void app_main()
{
    ESP_LOGI(LOGNAME, "SDK version:%s\n", esp_get_idf_version());
    ESP_LOGI(LOGNAME, "Starting\n");

    // setting up led
    //gpio_pad_select_gpio(LED);
    //gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    //gpio_set_level(LED, !value);

    gpio_pad_select_gpio(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);

    hk_setup_start();

    hk_setup_add_accessory(
        ACCESSORY_NAME,
        ACCESSORY_MANUFACTURER,
        ACCESSORY_MODEL,
        ACCESSORY_SERIAL_NR,
        ACCESSORY_REVISION,
        on_identify
    );

    void *switch_on_chr;
    void *motion_on_chr;

    hk_setup_add_srv(HK_SRV_DOORBELL, false, false);
    hk_setup_add_chr(HK_CHR_PROGRAMMABLE_SWITCH_EVENT, on_read, on_write, true, &switch_on_chr);

    hk_setup_add_srv(HK_SRV_MOTION_SENSOR, false, false);
    hk_setup_add_chr(HK_CHR_MOTION_DETECTED, on_read, on_write, true, &motion_on_chr);

    hk_setup_finish();

    // to restet the device. commented by default
    // hk_reset();

    // starting homekit
    hk_init(NAME, HK_CAT_DOOR, "111-22-222");

    debounce_t debounce;

    while (true) {
        int level = gpio_get_level(BUTTON_GPIO);

        debounce_update(&debounce, level);

        if (debounce_down(&debounce)) {
            printf("down\n");

            value = !value;
            hk_notify(switch_on_chr);
            hk_notify(motion_on_chr);
        }

        if (debounce_up(&debounce)) {
            printf("up\n");
        }
        
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}
