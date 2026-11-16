
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gpiod.h>

#define LED_GPIO 4

int main(void)
{
    struct gpiod_chip *chip;
    struct gpiod_line_settings *settings;
    struct gpiod_line_config *line_config;
    struct gpiod_request_config *req_config;
    struct gpiod_line_request *request;

    unsigned int offsets[] = {LED_GPIO};

    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("GPIO chip acilamadi");
        return EXIT_FAILURE;
    }

    settings = gpiod_line_settings_new();
    line_config = gpiod_line_config_new();
    req_config = gpiod_request_config_new();

    if (!settings || !line_config || !req_config) {
        fprintf(stderr, "GPIO ayarlari olusturulamadi\n");
        return EXIT_FAILURE;
    }

    gpiod_line_settings_set_direction(
        settings, GPIOD_LINE_DIRECTION_OUTPUT);

    gpiod_line_config_add_line_settings(
        line_config, offsets, 1, settings);

    gpiod_request_config_set_consumer(
        req_config, "Hafta1_LED");

    request = gpiod_chip_request_lines(
        chip, req_config, line_config);

    if (!request) {
        perror("GPIO hatti istenemedi");
        return EXIT_FAILURE;
    }

    while (1) {
        gpiod_line_request_set_value(
            request, LED_GPIO, GPIOD_LINE_VALUE_ACTIVE);
        printf("LED YANIYOR\n");
        fflush(stdout);
        sleep(1);

        gpiod_line_request_set_value(
            request, LED_GPIO, GPIOD_LINE_VALUE_INACTIVE);
        printf("LED SONUYOR\n");
        fflush(stdout);
        sleep(1);
    }

    return EXIT_SUCCESS;
}
