#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <string.h>
#include "esp_log.h"
#include <stdbool.h>

// #pragma region é o equivalente para #region so que para c

#define constrain(val, min, max) ((val)<(min)?(min):((val)>(max)?(max):(val)))

#pragma region html/webserver
esp_err_t handleRoot(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    const char* html =
        "<!DOCTYPE html>"
        "<html lang=\"pt-br\">"
        "<head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<title>esp 32 seguidor: nome</title>"
        "</head>"
        "<body>"
        "<style>"
            "body{"
                "background-color: black;"
                "display: flex;"
                "justify-content: center;"
                "align-items: center;"
            "}"
        "</style>"
        "<button onclick=\"iniciar()\" style=\"width: 30%;height: 40%;\">iniciar</button>"
        "</body>"
        "</html>";
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}
#pragma endregion html/webserver

#pragma region funcionalidade seguidor
const int SENSOR_PINS[8] = {34, 35, 32, 33, 25, 26, 27, 14};

#define MOTOR1_IN1   18
#define MOTOR1_IN2   19
#define MOTOR1_EN    21

#define MOTOR2_IN3   22
#define MOTOR2_IN4   23
#define MOTOR2_EN    16

#define PWM_CANAL_M1   LEDC_CHANNEL_0
#define PWM_CANAL_M2   LEDC_CHANNEL_1
#define PWM_FREQ       5000
#define PWM_RESOLUCAO  LEDC_TIMER_8_BIT

#define VEL_BASE    180
#define VEL_MIN      60
#define VEL_MAX     230

void lerSensores(int leituras[8]) {
    for (int i = 0; i < 8; i++) {
        leituras[i] = gpio_get_level(SENSOR_PINS[i]) == 0 ? 1 : 0; // trocar para == 1 dependendo do sensor
    }
}

bool linhaEsquerda(int leituras[8]) {
    int esq = leituras[0] + leituras[1] + leituras[2] + leituras[3];
    int dir = leituras[4] + leituras[5] + leituras[6] + leituras[7];
    return esq > dir;
}

bool linhaDireita(int leituras[8]) {
    int esq = leituras[0] + leituras[1] + leituras[2] + leituras[3];
    int dir = leituras[4] + leituras[5] + leituras[6] + leituras[7];
    return dir > esq;
}

bool linhaCentro(int leituras[8]) {
    return !linhaEsquerda(leituras) && !linhaDireita(leituras);
}

void girar(int pwm1, int pwm2) {
    // Motor 1
    pwm1 = constrain(pwm1, -255, 255);
    if (pwm1 > 0) {
        gpio_set_level(MOTOR1_IN1, 1);
        gpio_set_level(MOTOR1_IN2, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M1, pwm1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M1);
    } else if (pwm1 < 0) {
        gpio_set_level(MOTOR1_IN1, 0);
        gpio_set_level(MOTOR1_IN2, 1);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M1, -pwm1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M1);
    } else {
        gpio_set_level(MOTOR1_IN1, 0);
        gpio_set_level(MOTOR1_IN2, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M1, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M1);
    }

    // Motor 2
    pwm2 = constrain(pwm2, -255, 255);
    if (pwm2 > 0) {
        gpio_set_level(MOTOR2_IN3, 1);
        gpio_set_level(MOTOR2_IN4, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M2, pwm2);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M2);
    } else if (pwm2 < 0) {
        gpio_set_level(MOTOR2_IN3, 0);
        gpio_set_level(MOTOR2_IN4, 1);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M2, -pwm2);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M2);
    } else {
        gpio_set_level(MOTOR2_IN3, 0);
        gpio_set_level(MOTOR2_IN4, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M2, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CANAL_M2);
    }
}

void parar(){
    girar(0, 0);
}
void frente(int vel){
    girar(vel, vel);
}
void virarEsquerda(int vel_esq, int vel_dir){
    girar(vel_esq, vel_dir);
}
void virarDireita(int vel_esq, int vel_dir){
    girar(vel_esq, vel_dir);
}
#pragma endregion funcionalidade seguidor

void setup() {
    // WiFi
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = "Moto bomba",
            .password = "4ca142c8",
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();

    // GPIOs sensores
    for (int i = 0; i < 8; i++) {
        gpio_set_direction(SENSOR_PINS[i], GPIO_MODE_INPUT);
    }

    // GPIOs motores
    gpio_set_direction(MOTOR1_IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR1_IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR2_IN3, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR2_IN4, GPIO_MODE_OUTPUT);

    // PWM (LEDC)
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUCAO,
        .freq_hz         = PWM_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch1 = {
        .gpio_num   = MOTOR1_EN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_CANAL_M1,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
    };
    ledc_channel_config(&ch1);

    ledc_channel_config_t ch2 = {
        .gpio_num   = MOTOR2_EN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_CANAL_M2,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
    };
    ledc_channel_config(&ch2);

    parar();
}

void app_main(void) {
    setup();

    vTaskDelay(10000 / portTICK_PERIOD_MS);

    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_get_ip_info(netif, &ip_info);
    ESP_LOGI("WIFI", "IP: " IPSTR, IP2STR(&ip_info.ip));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server;
    httpd_start(&server, &config);

    httpd_uri_t root = {
        .uri     = "/",
        .method  = HTTP_GET,
        .handler = handleRoot,
    };
    httpd_register_uri_handler(server, &root);

    while (1) {
        int leituras[8];
        lerSensores(leituras);

        if (linhaCentro(leituras)) {
            frente(50);
        } else if (linhaDireita(leituras)) {
            virarDireita(50, 25);
        } else {
            virarEsquerda(25, 50);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}