#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "lib/lora/lora.h" // Registradores e constantes
#include "hardware/i2c.h"
#include "lib/aht20/aht20.h"
#include "lib/bmp280/bmp280.h"

#define I2C_PORT_0_BPM280 i2c0         // i2c0 pinos 0 e 1
#define I2C_SDA_0 0                   // 0
#define I2C_SCL_0 1                   // 1

#define I2C_PORT_1_AHT20 i2c1        // i2c1 pinos 2 e 3
#define I2C_SDA_1 2                   // 2
#define I2C_SCL_1 3

lora_config_t lora_config = {
    .spi = spi0,
    .pin_cs = 17,    // GPIO5 para CS
    .pin_rst = 20,   // GPIO6 para RST
    .pin_sck = 18,   // GPIO2 para SCK
    .pin_mosi = 19,  // GPIO3 para MOSI
    .pin_miso = 16   // GPIO4 para MISO
};

int main() {
    stdio_init_all();
    lora_setup(&lora_config); // Executa toda a configuração inicial

    // Inicializa o I2C_0 para aht20
    i2c_init(I2C_PORT_0_BPM280, 400 * 1000);
    gpio_set_function(I2C_SDA_0, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_0, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_0);
    gpio_pull_up(I2C_SCL_0);

    // Inicializa o I2C_1 para bpm280
    i2c_init(I2C_PORT_1_AHT20, 400 * 1000);
    gpio_set_function(I2C_SDA_1, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_1, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_1);
    gpio_pull_up(I2C_SCL_1);

    // Inicializa o BMP280
    bmp280_init(I2C_PORT_0_BPM280);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C_PORT_0_BPM280, &params);

    // Inicializa o AHT20
    aht20_reset(I2C_PORT_1_AHT20);
    aht20_init(I2C_PORT_1_AHT20);

    // Estrutura para armazenar os dados do sensor
    AHT20_Data data;
    int32_t raw_temp_bmp;
    int32_t raw_pressure;

    sleep_ms(2000); // Aguarda 1 segundo para estabilizar
    printf("Transmissor LoRa pronto para enviar dados.\n");
    sleep_ms(5000); // Aguarda 2 segundos antes de iniciar a transmissão

    while (1) {

        // Leitura do BMP280
        bmp280_read_raw(I2C_PORT_0_BPM280, &raw_temp_bmp, &raw_pressure);
        int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params);

        float temp_bmp = temperature / 100.0f; // Temperatura em graus Celsius

        /* // PRINTS PARA DEPURAÇÃO NO TERMINAL//////////////////////////
        printf("-----------BMP280 LEITURAS-----------------\n");
        printf("Pressao = %.3f kPa\n", pressure / 1000.0);
        printf("Temperatura BMP: = %.2f C\n", temp_bmp); */

        // Leitura do AHT20
        float temp_aht = 0.0f;
        float hum_aht = 0.0f;

        if (aht20_read(I2C_PORT_1_AHT20, &data)){
            temp_aht = data.temperature;
            hum_aht = data.humidity;
            /* printf("----------AHT LEITURAS------------------\n");
            printf("Temperatura : %.2f C\n", temp_aht);
            printf("Umidade: %.2f %%\n\n\n", hum_aht); */
        }

        // Formata os dados como string
        char payload[64]; // Buffer para a string
        int payload_len = sprintf(payload, "T1:%.2f,T2:%.2f,H:%.2f",
                               temp_bmp, temp_aht, hum_aht);

        // Imprime a string que será enviada (para depuração)
        printf("Enviando: %s\n", payload);

        // Envia a string como um pacote LoRa
        lora_send_packet(&lora_config, (uint8_t*)payload, payload_len);

        sleep_ms(2000); // Aguarda 2 segundos antes de enviar o próximo pacote
    }

    return 0;
}
