// /**
//  * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
//  *
//  * SPDX-License-Identifier: BSD-3-Clause
//  */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
// adicionando imports especificos
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include <string.h>

const int PIN_TRIGGER = 14;
const int PIN_ECHO = 15;
const int T_TRIGGER = 10; // tempo do pulso no trigger é de 10us

volatile int start_time;
volatile int end_time;
volatile bool timer_fired = false;

void callback_echo(uint gpio, uint32_t events)
{
    if (events & GPIO_IRQ_EDGE_RISE)
    {
        start_time = to_us_since_boot(get_absolute_time());
    }
    else if (events & GPIO_IRQ_EDGE_FALL)
    {
        end_time = to_us_since_boot(get_absolute_time());
    }
}

int64_t callback_alarme(alarm_id_t id, void *user_data)
{
    timer_fired = true;
    return 0;
}

void inicia_trigger()
{
    gpio_put(PIN_TRIGGER, 1);
    sleep_us(T_TRIGGER);
    gpio_put(PIN_TRIGGER, 0);
    sleep_ms(500);
}

int main()
{
    stdio_init_all();

    gpio_init(PIN_TRIGGER);
    gpio_init(PIN_ECHO);

    gpio_set_dir(PIN_TRIGGER, GPIO_OUT);
    gpio_set_dir(PIN_ECHO, GPIO_IN);

    gpio_set_irq_enabled_with_callback(
        PIN_ECHO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &callback_echo);

    datetime_t t_0 = {
        .year = 2024,
        .month = 03,
        .day = 17,
        .dotw = 1, // 0 is Sunday, so 3 is Wednesday
        .hour = 13,
        .min = 56,
        .sec = 00};

    rtc_init();
    rtc_set_datetime(&t_0);

    alarm_id_t alarme;
    bool run = false;

    while (true)
    {
        char caracter = getchar_timeout_us(100);

        // alarme = add_alarm_in_ms(5000, callback_alarme, NULL, false);

        if (caracter == 's')
        {
            run = true;
            printf("Iniciando medição\n");
        }
        else if (caracter == 'p')
        {
            run = false;
            printf("Parando medição\n");
        }

        if (run)
        {
            inicia_trigger();
            alarme = add_alarm_in_ms(1000, callback_alarme, NULL, false);
            while (end_time == 0 && timer_fired == false)
            {
            }

            if (!timer_fired)
            {
                cancel_alarm(alarme);
                uint32_t duracao_us = end_time - start_time;
                float distancia = (duracao_us / 2.0) * 0.0343;
                rtc_get_datetime(&t_0);
                printf("%02d:%02d:%02d - %.2f cm\n", t_0.hour, t_0.min, t_0.sec, distancia);
            }
            else
            {
                rtc_get_datetime(&t_0);
                printf("%02d:%02d:%02d - Falha\n", t_0.hour, t_0.min, t_0.sec);
                timer_fired = false;
                cancel_alarm(alarme);
            }

            end_time = 0;
        }
        else
        {
            sleep_ms(100);
        }
    }
}
