#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "lib/lora/lora.h" // Registradores e constantes
#include "hardware/i2c.h"
#include "lib/sensores/aht20.h"
#include "lib/sensores/bmp280.h"

#define I2C_PORT_0_BPM280 i2c0         // i2c0 pinos 0 e 1
#define I2C_SDA_0 0                   // 0
#define I2C_SCL_0 1                   // 1

#define I2C_PORT_1_AHT20 i2c1        // i2c1 pinos 2 e 3
#define I2C_SDA_1 2                   // 2
#define I2C_SCL_1 3

// Definições de Pinos para o SPI
#define SPI_PORT spi0
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_MISO 16

// Pinos de controle do RFM95W
#define PIN_CS   17  // Chip Select
#define PIN_RST  20  // Reset

// Define uma union para converter float para um array de bytes
typedef union {
    float f;
    uint8_t b[4];
} float_to_byte;


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

    // 3.2. Definir Frequência de Operação (915 MHz)
    SetFrequency(915.0);

    // 3.3. Configuração do Rádio LoRa – BW, FS, CR, LDRO, etc.
    writeRegister(REG_MODEM_CONFIG, BANDWIDTH_125K | ERROR_CODING_4_5);
    writeRegister(REG_MODEM_CONFIG2, SPREADING_7 | CRC_OFF);
    // Configura o cabeçalho implícito (Implicit Header)
    writeRegister(REG_MODEM_CONFIG3, 0x04); // LNA gain boost

    // --- Configuração da Potência de Transmissão (TX Power) ---
    // A potência de 17 dBm é a máxima permitida no pino PA_BOOST
    // quando o registrador REG_PA_DAC não está configurado para 20 dBm.
    writeRegister(REG_PA_CONFIG, 0x8F); // PA_BOOST com a potência máxima de 17 dBm

    // 3.4. Definir tamanho do Payload.
    writeRegister(REG_PAYLOAD_LENGTH, 15);
    writeRegister(REG_MAX_PAYLOAD_LENGTH, 15);

    // 3.5. Mudar para modo STANDBY
    writeRegister(REG_OPMODE, RF95_MODE_STANDBY);
    sleep_ms(10);
}


// Função para enviar um pacote
void lora_send_packet(uint8_t* data, uint8_t len) {
    // 1. Entrar no modo STANDBY
    writeRegister(REG_OPMODE, RF95_MODE_STANDBY);
    // 2. Limpar a FIFO
    writeRegister(REG_FIFO_ADDR_PTR, 0x00);
    writeRegister(REG_FIFO_TX_BASE_AD, 0x00);
    // 3. Escrever os dados na FIFO
    for (int i = 0; i < len; i++) {
        writeRegister(REG_FIFO, data[i]);
    }
    // 4. Definir o tamanho do payload
    writeRegister(REG_PAYLOAD_LENGTH, len);
    // 5. Entrar no modo TX
    writeRegister(REG_OPMODE, RF95_MODE_TX);
    // 6. Esperar a transmissão terminar
    while ((readRegister(REG_IRQ_FLAGS) & 0x08) == 0); // Espera o TxDone
    // 7. Limpar o flag de interrupção TxDone
    writeRegister(REG_IRQ_FLAGS, 0xFF);
    // 8. Voltar para o modo STANDBY
    writeRegister(REG_OPMODE, RF95_MODE_STANDBY);
}


int main() {
    stdio_init_all();
    lora_setup(); // Executa toda a configuração inicial

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
        lora_send_packet((uint8_t*)payload, payload_len);

        sleep_ms(2000); // Aguarda 2 segundos antes de enviar o próximo pacote
    }

    return 0;
}
