#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "lib/lora/lora.h" // Registradores e constantes

// Definições de Pinos para o SPI
#define SPI_PORT spi0
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_MISO 16

// Pinos de controle do RFM95W
#define PIN_CS   17  // Chip Select
#define PIN_RST  20  // Reset

// Funções auxiliares para o SPI
static void cs_select() {
    gpio_put(PIN_CS, 0);
}

static void cs_deselect() {
    gpio_put(PIN_CS, 1);
}

// Funções para leitura e escrita de registradores do RFM95W
static void writeRegister(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg | 0x80; // Bit 7 = 1 para escrita
    buf[1] = data;
    cs_select();
    spi_write_blocking(SPI_PORT, buf, 2);
    cs_deselect();
}

// Função para ler um registrador
static uint8_t readRegister(uint8_t reg) {
    uint8_t buf_in[1];
    uint8_t buf_out[1];
    buf_out[0] = reg & 0x7F; // Bit 7 = 0 para leitura
    cs_select();
    spi_write_blocking(SPI_PORT, buf_out, 1);
    spi_read_blocking(SPI_PORT, 0, buf_in, 1);
    cs_deselect();
    return buf_in[0];
}

// Função para definir a frequência
void SetFrequency(double Frequency) {
    unsigned long FrequencyValue;
    Frequency = Frequency * 7110656 / 434;
    FrequencyValue = (unsigned long)(Frequency);
    writeRegister(REG_FRF_MSB, (FrequencyValue >> 16) & 0xFF);
    writeRegister(REG_FRF_MID, (FrequencyValue >> 8) & 0xFF);
    writeRegister(REG_FRF_LSB, FrequencyValue & 0xFF);
}

// Função para configurar o rádio LoRa
void lora_setup() {
    // 1. Inicializar GPIOs – CS e RST
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    cs_deselect(); // Garante que o CS está desativado inicialmente

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 0); // Ativa o reset por um momento
    sleep_ms(10);
    gpio_put(PIN_RST, 1); // Libera o reset
    sleep_ms(10);

    // 2. Inicializar Interface SPI
    spi_init(SPI_PORT, 1000000); // 1 MHz
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // 3. Configuração Inicial do Rádio LoRa
    // 3.1. Definir Modo LoRa (REG 0x01) MODE-SLEEP
    writeRegister(REG_OPMODE, RF95_MODE_SLEEP);
    sleep_ms(10);
    // Definir o modo LoRa no registrador de operação
    writeRegister(REG_OPMODE, RF95_MODE_SLEEP | 0x80); // 0x80 = bit 7 em 1 para LoRa
    sleep_ms(10);

    // 3.2. Definir Frequência de Operação (915 MHz) - DEVE SER A MESMA DO TRANSMISSOR
    SetFrequency(915.0);

    // 3.3. Configuração do Rádio LoRa – BW, FS, CR, LDRO, etc.
    writeRegister(REG_MODEM_CONFIG, BANDWIDTH_125K | ERROR_CODING_4_5);
    writeRegister(REG_MODEM_CONFIG2, SPREADING_7 | CRC_OFF);
    writeRegister(REG_MODEM_CONFIG3, 0x04); // LNA gain boost

    // 3.4. Definir tamanho do Payload máximo para recepção
    writeRegister(REG_PAYLOAD_LENGTH, 64);
    writeRegister(REG_MAX_PAYLOAD_LENGTH, 64);

    // 3.5. Mudar para modo STANDBY
    writeRegister(REG_OPMODE, RF95_MODE_STANDBY);
    sleep_ms(10);
}

// Função para iniciar a recepção contínua
void lora_receive_continuous() {
    // Configurar ponteiro de FIFO para início da área de recepção
    writeRegister(REG_FIFO_ADDR_PTR, 0);
    writeRegister(REG_FIFO_RX_BASE_AD, 0);

    // Colocar no modo de recepção contínua
    writeRegister(REG_OPMODE, RF95_MODE_RX_CONTINUOUS);
}

// Função para verificar e receber pacotes
bool lora_receive_packet(uint8_t* buffer, uint8_t* len) {
    // Verifica flags de interrupção
    uint8_t irq_flags = readRegister(REG_IRQ_FLAGS);

    // Verifica se um pacote foi recebido (bit RxDone = 1)
    if ((irq_flags & 0x40) != 0) {
        // Limpar flags de interrupção
        writeRegister(REG_IRQ_FLAGS, 0xFF);

        // Obter o tamanho do pacote recebido
        uint8_t packet_len = readRegister(REG_RX_NB_BYTES);
        *len = packet_len;

        // Obter o endereço atual do ponteiro FIFO RX
        uint8_t current_addr = readRegister(REG_FIFO_RX_CURRENT_ADDR);

        // Configurar o ponteiro FIFO para ler os dados
        writeRegister(REG_FIFO_ADDR_PTR, current_addr);

        // Ler os dados da FIFO
        for (int i = 0; i < packet_len; i++) {
            buffer[i] = readRegister(REG_FIFO);
        }

        // Terminar o buffer com null para tratá-lo como string
        buffer[packet_len] = '\0';

        return true;
    }

    return false;
}

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

    lora_setup(); // Executa toda a configuração inicial
    printf("Receptor LoRa configurado e pronto para receber!\n");

    // Configurar para modo de recepção contínua
    lora_receive_continuous();

    uint8_t buffer[65]; // 64 bytes + 1 para null terminator
    uint8_t len = 0;
    float temp_bmp, temp_aht, hum_aht;

    while (1) {
        // Verifica se um pacote foi recebido
        if (lora_receive_packet(buffer, &len)) {
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
            lora_receive_continuous();
        }

        // Pequeno delay para não sobrecarregar o CPU
        sleep_ms(10);
    }

    return 0;
}
