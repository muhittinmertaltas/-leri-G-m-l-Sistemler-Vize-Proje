
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define LED_GPIO       4

#define I2C_PORT       I2C_NUM_0
#define I2C_SDA        21
#define I2C_SCL        22
#define MPU_ADDR       0x68

#define UART_PORT      UART_NUM_2
#define UART_TX        17
#define UART_RX        18
#define UART_BAUD      115200

#define SAMPLE_PERIOD  pdMS_TO_TICKS(20)

static uint8_t xor_checksum(const char *data)
{
    uint8_t result = 0;

    while (*data) {
        result ^= (uint8_t)*data++;
    }

    return result;
}

static esp_err_t mpu_write(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};

    return i2c_master_write_to_device(
        I2C_PORT, MPU_ADDR, data, sizeof(data),
        pdMS_TO_TICKS(100));
}

static esp_err_t mpu_read(uint8_t reg, uint8_t *data, size_t length)
{
    return i2c_master_write_read_device(
        I2C_PORT, MPU_ADDR, &reg, 1, data, length,
        pdMS_TO_TICKS(100));
}

static void i2c_init(void)
{
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &config));
    ESP_ERROR_CHECK(i2c_driver_install(
        I2C_PORT, config.mode, 0, 0, 0));
}

static void mpu_init(void)
{
    ESP_ERROR_CHECK(mpu_write(0x6B, 0x00));
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_ERROR_CHECK(mpu_write(0x1A, 0x03));
    ESP_ERROR_CHECK(mpu_write(0x19, 19));
    ESP_ERROR_CHECK(mpu_write(0x1B, 0x00));
    ESP_ERROR_CHECK(mpu_write(0x1C, 0x00));
}

static void uart_init(void)
{
    uart_config_t config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_driver_install(
        UART_PORT, 1024, 0, 0, NULL, 0));

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &config));

    ESP_ERROR_CHECK(uart_set_pin(
        UART_PORT, UART_TX, UART_RX,
        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void read_pi_commands(void)
{
    static char command[64];
    static int index = 0;

    uint8_t byte;

    while (uart_read_bytes(
        UART_PORT, &byte, 1, 0) == 1) {

        if (byte == '\n') {
            command[index] = '\0';

            char *last_comma = strrchr(command, ',');

            if (last_comma != NULL) {
                *last_comma = '\0';

                unsigned int received;

                if (sscanf(last_comma + 1, "%x", &received) == 1 &&
                    xor_checksum(command) == (uint8_t)received) {

                    if (strcmp(command, "CMD,LED,1") == 0) {
                        gpio_set_level(LED_GPIO, 1);
                        printf("ALARM LED ON\n");
                    }
                    else if (strcmp(command, "CMD,LED,0") == 0) {
                        gpio_set_level(LED_GPIO, 0);
                        printf("ALARM LED OFF\n");
                    }
                }
            }

            index = 0;
        }
        else if (index < (int)sizeof(command) - 1) {
            command[index++] = (char)byte;
        }
        else {
            index = 0;
        }
    }
}

static void send_sensor_data(void)
{
    uint8_t data[14];

    if (mpu_read(0x3B, data, sizeof(data)) != ESP_OK) {
        printf("MPU6050 okuma hatasi\n");
        return;
    }

    int16_t raw_ax = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_ay = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_az = (int16_t)((data[4] << 8) | data[5]);

    int16_t raw_gx = (int16_t)((data[8] << 8) | data[9]);
    int16_t raw_gy = (int16_t)((data[10] << 8) | data[11]);
    int16_t raw_gz = (int16_t)((data[12] << 8) | data[13]);

    float ax = raw_ax / 16384.0f;
    float ay = raw_ay / 16384.0f;
    float az = raw_az / 16384.0f;

    float gx = raw_gx / 131.0f;
    float gy = raw_gy / 131.0f;
    float gz = raw_gz / 131.0f;

    char payload[128];

    snprintf(payload, sizeof(payload),
        "DATA,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
        ax, ay, az, gx, gy, gz);

    char packet[150];

    snprintf(packet, sizeof(packet),
        "%s,%02X\n", payload, xor_checksum(payload));

    uart_write_bytes(UART_PORT, packet, strlen(packet));
}

void app_main(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    i2c_init();
    uart_init();
    mpu_init();

    printf("TILT ALARM ESP32 BASLADI\n");
    printf("MPU6050: I2C 21/22\n");
    printf("UART2: TX17 RX18, 115200 baud\n");

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        read_pi_commands();
        send_sensor_data();

        vTaskDelayUntil(&last_wake, SAMPLE_PERIOD);
    }
}
