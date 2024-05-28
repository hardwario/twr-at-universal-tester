#include <application.h>

#define FIRMWARE "at-universal-tester"

twr_led_t led;
twr_button_t button;
twr_tmp112_t tmp112;
twr_module_relay_t relay_0;
twr_module_relay_t relay_1;
float temperature = NAN;
bool adc_calibrated = false;

#define RELAY_ON TWR_MODULE_RELAY_STATE_TRUE
#define RELAY_OFF TWR_MODULE_RELAY_STATE_FALSE

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    if (event == TWR_BUTTON_EVENT_PRESS)
    {
        twr_atci_printfln("@BUTTON");
    }
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    if (event == TWR_TMP112_EVENT_UPDATE)
    {
        float celsius;
        temperature = twr_tmp112_get_temperature_celsius(self, &celsius) ? celsius : NAN;
    }
}

bool i2c_get_channel(twr_atci_param_t *param, twr_i2c_channel_t *channel)
{
    uint32_t ch;
    static bool i2c1_initialized = false;

    if (!twr_atci_get_uint(param, &ch))
    {
        return false;
    }

    if (ch == 0)
    {
        *channel = TWR_I2C_I2C0;
    }
    else if (ch == 1)
    {
        *channel = TWR_I2C_I2C1;
        if (!i2c1_initialized)
        {
            twr_i2c_init(TWR_I2C_I2C1, TWR_I2C_SPEED_400_KHZ);
            i2c1_initialized = true;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool atci_i2c_memory_read(twr_atci_param_t *param)
{
    twr_i2c_channel_t channel;
    char str_address[16];
    char str_memory_register[16];

    uint32_t address;
    uint32_t memory_register;
    uint32_t length;

    static uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));

    if (!i2c_get_channel(param, &channel))
    {
        return false;
    }

    if (!twr_atci_is_comma(param))
    {
        return false;
    }

    if (!twr_atci_get_string(param, str_address, sizeof(str_address)))
    {
        return false;
    }

    if (!twr_atci_is_comma(param))
    {
        return false;
    }

    if (!twr_atci_get_string(param, str_memory_register, sizeof(str_memory_register)))
    {
        return false;
    }

    if (!twr_atci_is_comma(param))
    {
        return false;
    }

    if (!twr_atci_get_uint(param, &length))
    {
        return false;
    }

    address = (uint32_t)strtol(str_address, NULL, 0);
    memory_register = (uint32_t)strtol(str_memory_register, NULL, 0);

    // twr_log_debug("i2c: memmory_read: channel: %ld address: 0x%02x, reg: 0x%02x, len: %ld", channel, address, memory_register, length);

    twr_i2c_memory_transfer_t transfer;
    transfer.device_address = address;
    transfer.memory_address = memory_register;
    transfer.buffer = buffer;
    transfer.length = length;

    bool ret = twr_i2c_memory_read(channel, &transfer);

    if (!ret)
    {
        return false;
    }

    twr_atci_printf("$I2CMR: ");

    for (uint32_t i = 0; i < length; i++)
    {
        twr_atci_printf("%02X", buffer[i]);
        // if (i != length - 1)
        // {
        //     twr_atci_printf(",");
        // }
    }

    twr_atci_printf("\r\n");

    return ret;
}

bool atci_i2c_memory_write(twr_atci_param_t *param)
{
    twr_i2c_channel_t channel;

    char str_address[16];
    char str_memory_register[16];
    char str_write_data[16];

    uint32_t address;
    uint32_t memory_register;
    uint8_t write_data[2];

    if (!i2c_get_channel(param, &channel))
    {
        twr_log_error("Invalid channel");
        return false;
    }

    if (!twr_atci_is_comma(param))
    {
        twr_log_error("Invalid parse comma");
        return false;
    }

    if (!twr_atci_get_string(param, str_address, sizeof(str_address)))
    {
        twr_log_error("Invalid parse address");
        return false;
    }

    if (!twr_atci_is_comma(param))
    {
        twr_log_error("Invalid parse comma");
        return false;
    }

    if (!twr_atci_get_string(param, str_memory_register, sizeof(str_memory_register)))
    {
        twr_log_error("Invalid parse memory register");
        return false;
    }

    if (!twr_atci_is_comma(param))
    {
        twr_log_error("Invalid parse comma");
        return false;
    }

    if (!twr_atci_get_string(param, str_write_data, sizeof(str_write_data)))
    {
        twr_log_error("Invalid parse write data");
        return false;
    }

    twr_i2c_memory_transfer_t transfer;

    address = (uint32_t)strtol(str_address, NULL, 0);
    memory_register = (uint32_t)strtol(str_memory_register, NULL, 0);

    memset(write_data, 0, sizeof(write_data));

    size_t len = strlen(str_write_data);
    char tmp[3] = {0, 0, 0};

    transfer.length = 0;

    for (size_t i = 0; i < len; i++)
    {
        tmp[i % 2] = str_write_data[i];
        if (i % 2 == 1)
        {
            write_data[transfer.length++] = (uint8_t)strtol(tmp, NULL, 16);
        }
    }

    // twr_log_debug("i2c: memmory_write: channel: %ld address: 0x%02x, reg: 0x%02x, len: %d", channel, address, memory_register, transfer.length);

    transfer.device_address = address;
    transfer.memory_address = memory_register;
    transfer.buffer = &write_data;

    bool ret = twr_i2c_memory_write(channel, &transfer);

    twr_atci_printfln("$I2CMW: %d", ret);

    return ret;
}

bool atci_i2c_scan(twr_atci_param_t *param)
{
    twr_i2c_channel_t channel;

    if (!i2c_get_channel(param, &channel))
    {
        return false;
    }

    uint8_t dummy;
    bool flag_comma = false;

    twr_i2c_memory_transfer_t transfer;
    transfer.memory_address = 0;
    transfer.buffer = &dummy;
    transfer.length = 1;

    twr_atci_printf("$SCAN: ");

    for (int i = 0; i < 127; i++)
    {
        transfer.device_address = i;
        bool ret = twr_i2c_memory_read(channel, &transfer);

        if (ret)
        {
            if (flag_comma)
            {
                twr_atci_printf(",");
            }

            twr_atci_printf("0x%02x", transfer.device_address);
            flag_comma = true;
        }
    }

    twr_atci_printf("\r\n");

    return true;
}

bool adc_get_value(uint32_t channel, uint16_t *value)
{
    if (channel >= TWR_ADC_CHANNEL_A6)
    {
        twr_log_error("Invalid channel number %lu", channel);
        return false;
    }

    twr_gpio_set_mode(channel, TWR_GPIO_MODE_ANALOG);
    twr_adc_oversampling_set(channel, TWR_ADC_OVERSAMPLING_256);
    twr_adc_resolution_set(channel, TWR_ADC_RESOLUTION_12_BIT);

    if (!twr_adc_get_value(channel, value))
    {
        twr_log_error("ADC read failed");
        return false;
    }

    return true;
}

bool atci_adc(twr_atci_param_t *param)
{
    uint32_t channel;
    uint16_t value;

    if (!twr_atci_get_uint(param, &channel))
    {
        return false;
    }

    if (!adc_get_value(channel, &value))
    {
        return false;
    }

    twr_atci_printfln("$ADC: %d", value);

    return true;
}

bool atci_adc_v(twr_atci_param_t *param)
{
    uint32_t channel;
    uint16_t value;
    float voltage;

    if (!twr_atci_get_uint(param, &channel))
    {
        twr_log_error("Invalid parse channel number");
        return false;
    }

    twr_adc_calibration();

    float vdda_voltage;

    if (!twr_adc_get_vdda_voltage(&vdda_voltage))
    {
        twr_log_error("Invalid VDDA voltage");
        return false;
    }

    if (vdda_voltage < 3.2f || vdda_voltage > 3.4f)
    {
        twr_log_error("Invalid VDDA voltage: %.3f", vdda_voltage);
        return false;
    }

    if (!adc_get_value(channel, &value))
    {
        return false;
    }

    // twr_log_debug("atci_adc_v vdda: %.3f value: %d", vdda_voltage, value);

    voltage = ((float)value * vdda_voltage) / 0xffff;

    twr_atci_printfln("$ADCV: %.3f", voltage);

    return true;
}

bool atci_relay(twr_atci_param_t *param)
{
    uint32_t relay;
    uint32_t state;

    if (!twr_atci_get_uint(param, &relay))
    {
        return false;
    }

    if (!twr_atci_is_comma(param))
    {
        twr_module_relay_state_t state;

        if (relay == 0)
        {
            state = twr_module_relay_get_state(&relay_0);
        }
        else if (relay == 1)
        {
            state = twr_module_relay_get_state(&relay_1);
        }
        else
        {
            return false;
        }

        if (state == RELAY_ON)
        {
            twr_atci_printfln("$RELAY: 1");
            return true;
        }
        else if (state == RELAY_OFF)
        {
            twr_atci_printfln("$RELAY: 0");
            return true;
        };

        return false;
    }

    if (!twr_atci_get_uint(param, &state))
    {
        return false;
    }

    if (state != 0 && state != 1)
    {
        return false;
    }

    if (relay == 0)
    {
        twr_module_relay_set_state(&relay_0, state ? RELAY_ON : RELAY_OFF);
    }
    else if (relay == 1)
    {
        twr_module_relay_set_state(&relay_1, state ? RELAY_ON : RELAY_OFF);
    }
    else
    {
        return false;
    }

    return true;
}

bool at_i(void)
{
    twr_atci_printfln("\"" FIRMWARE "-v%u.%u.%u\"",
                      (twr_info_fw_version() >> 24) & 0xff,
                      (twr_info_fw_version() >> 16) & 0xff,
                      (twr_info_fw_version() >> 8) & 0xff);
    return true;
}

bool at_reboot(void)
{
    twr_atci_printfln("OK");
    twr_system_reset();
    return true;
}

// Application initialization function which is called once after boot
void application_init(void)
{
    twr_system_pll_enable();

    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, 0);
    twr_led_pulse(&led, 2000);

    twr_adc_init();
    twr_timer_init();
    twr_module_relay_init(&relay_0, TWR_MODULE_RELAY_I2C_ADDRESS_DEFAULT);
    twr_module_relay_set_state(&relay_0, RELAY_OFF);
    twr_module_relay_init(&relay_1, TWR_MODULE_RELAY_I2C_ADDRESS_ALTERNATE);
    twr_module_relay_set_state(&relay_1, RELAY_OFF);

    static twr_atci_command_t commands[] = {
        {"I", at_i, NULL, NULL, NULL, "Request product information"},
        TWR_ATCI_COMMAND_CLAC,
        {"$I2CMR", NULL, atci_i2c_memory_read, NULL, NULL, "I2C memory read"},
        {"$I2CMW", NULL, atci_i2c_memory_write, NULL, NULL, "I2C memory write"},
        {"$I2CSCAN", NULL, atci_i2c_scan, NULL, NULL, "Scan I2C bus"},
        {"$ADCV", NULL, atci_adc_v, NULL, NULL, "Read ADC voltage"},
        {"$ADC", NULL, atci_adc, NULL, NULL, "Read ADC value"},
        {"$RELAY", NULL, atci_relay, NULL, NULL, "Control relay"},
        {"$REBOOT", at_reboot, NULL, NULL, NULL, "Reboot"},
        TWR_ATCI_COMMAND_HELP};

    twr_atci_init(commands, TWR_ATCI_COMMANDS_LENGTH(commands));
    twr_atci_set_uart_active_callback(NULL, 0); // atci is always active

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, 0);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize thermometer on core module
    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, 10000);

    twr_scheduler_plan_from_now(0, 200);
}

void application_task(void)
{
    adc_get_value(TWR_ADC_CHANNEL_A0, NULL);

    twr_atci_printfln("@BOOT: \"" FIRMWARE "-v%u.%u.%u\"",
                      (twr_info_fw_version() >> 24) & 0xff,
                      (twr_info_fw_version() >> 16) & 0xff,
                      (twr_info_fw_version() >> 8) & 0xff);
}
