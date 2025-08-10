#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "lib/lora/lora.h" // Registradores e constantes

lora_config_t lora_config = {
    .spi = spi0,
    .pin_cs = 17,    // GPIO5 para CS
    .pin_rst = 20,   // GPIO6 para RST
    .pin_sck = 18,   // GPIO2 para SCK
    .pin_mosi = 19,  // GPIO3 para MOSI
    .pin_miso = 16   // GPIO4 para MISO
};

// Função para extrair valores da string
bool parse_data_string(const char* data, float* temp_bmp, float* temp_aht, float* hum_aht) {
    // Formato esperado: "T1:XX.XX,T2:XX.XX,H:XX.XX"
    int result = sscanf(data, "T1:%f,T2:%f,H:%f", temp_bmp, temp_aht, hum_aht);
    return (result == 3); // Retorna true se conseguiu extrair os 3 valores
}

int main() {
    stdio_init_all();

    // Aguarda inicialização da porta serial para garantir que vemos todos os logs
    sleep_ms(2000);

    printf("Inicializando receptor LoRa...\n");

    lora_setup(&lora_config); // Executa toda a configuração inicial
    printf("Receptor LoRa configurado e pronto para receber!\n");

    // Configurar para modo de recepção contínua
    lora_receive_continuous(&lora_config);

    uint8_t buffer[65]; // 64 bytes + 1 para null terminator
    uint8_t len = 0;
    float temp_bmp, temp_aht, hum_aht;

    while (1) {
        // Verifica se um pacote foi recebido
        if (lora_receive_packet(&lora_config, buffer, &len)) {
            printf("\n-----------PACOTE RECEBIDO-----------------\n");
            printf("Comprimento: %d bytes\n", len);
            printf("Dados: %s\n", buffer);

            // Processa os dados recebidos
            if (parse_data_string((char*)buffer, &temp_bmp, &temp_aht, &hum_aht)) {
                printf("-----------DADOS DECODIFICADOS-----------------\n");
                printf("Temperatura BMP280: %.2f °C\n", temp_bmp);
                printf("Temperatura AHT20: %.2f °C\n", temp_aht);
                printf("Umidade AHT20: %.2f %%\n", hum_aht);
            } else {
                printf("Erro ao decodificar os dados recebidos!\n");
            }

            // Voltar ao modo de recepção contínua para o próximo pacote
            lora_receive_continuous(&lora_config);
        }

        // Pequeno delay para não sobrecarregar o CPU
        sleep_ms(10);
    }

    return 0;
}
