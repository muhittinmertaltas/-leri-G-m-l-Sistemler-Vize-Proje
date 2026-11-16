
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc.h"
#include "soc/gpio_reg.h"

#define LED_GPIO 4

void app_main(void)
{
    REG_WRITE(GPIO_ENABLE_W1TS_REG, (1U << LED_GPIO));

    while (1) {
        REG_WRITE(GPIO_OUT_W1TS_REG, (1U << LED_GPIO));
        printf("LED YANIYOR\n");
        vTaskDelay(pdMS_TO_TICKS(1000));

        REG_WRITE(GPIO_OUT_W1TC_REG, (1U << LED_GPIO));
        printf("LED SONUYOR\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
