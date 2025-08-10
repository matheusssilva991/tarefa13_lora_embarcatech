#include "lora.h"

// Funções auxiliares para o SPI
void cs_select(uint8_t pin) {
    gpio_put(pin, 0);
}

void cs_deselect(uint8_t pin) {
    gpio_put(pin, 1);
}

// Funções para leitura e escrita de registradores do RFM95W
void writeRegister(lora_config_t *config, uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg | 0x80; // Bit 7 = 1 para escrita
    buf[1] = data;
    cs_select(config->pin_cs);
    spi_write_blocking(config->spi, buf, 2);
    cs_deselect(config->pin_cs);
}

// Função para ler um registrador
uint8_t readRegister(lora_config_t *config, uint8_t reg) {
    uint8_t buf_in[1];
    uint8_t buf_out[1];
    buf_out[0] = reg & 0x7F; // Bit 7 = 0 para leitura
    cs_select(config->pin_cs);
    spi_write_blocking(config->spi, buf_out, 1);
    spi_read_blocking(config->spi, 0, buf_in, 1);
    cs_deselect(config->pin_cs);
    return buf_in[0];
}

// Função para definir a frequência
void SetFrequency(lora_config_t *config, double Frequency) {
    unsigned long FrequencyValue;
    Frequency = Frequency * 7110656 / 434;
    FrequencyValue = (unsigned long)(Frequency);
    writeRegister(config, REG_FRF_MSB, (FrequencyValue >> 16) & 0xFF);
    writeRegister(config, REG_FRF_MID, (FrequencyValue >> 8) & 0xFF);
    writeRegister(config, REG_FRF_LSB, FrequencyValue & 0xFF);
}

// Função para configurar o rádio LoRa
void lora_setup(lora_config_t *config) {
    // 1. Inicializar GPIOs – CS e RST
    gpio_init(config->pin_cs);
    gpio_set_dir(config->pin_cs, GPIO_OUT);
    cs_deselect(config->pin_cs); // Garante que o CS está desativado inicialmente

    gpio_init(config->pin_rst);
    gpio_set_dir(config->pin_rst, GPIO_OUT);
    gpio_put(config->pin_rst, 0); // Ativa o reset por um momento
    sleep_ms(10);
    gpio_put(config->pin_rst, 1); // Libera o reset
    sleep_ms(10);

    // 2. Inicializar Interface SPI
    spi_init(config->spi, 1000000); // 1 MHz
    gpio_set_function(config->pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(config->pin_mosi, GPIO_FUNC_SPI);
    gpio_set_function(config->pin_miso, GPIO_FUNC_SPI);

    // 3. Configuração Inicial do Rádio LoRa
    // 3.1. Definir Modo LoRa (REG 0x01) MODE-SLEEP
    writeRegister(config, REG_OPMODE, RF95_MODE_SLEEP);
    sleep_ms(10);
    // Definir o modo LoRa no registrador de operação
    writeRegister(config, REG_OPMODE, RF95_MODE_SLEEP | 0x80); // 0x80 = bit 7 em 1 para LoRa
    sleep_ms(10);

    // 3.2. Definir Frequência de Operação (915 MHz)
    SetFrequency(config, 915.0);

    // 3.3. Configuração do Rádio LoRa – BW, FS, CR, LDRO, etc.
    writeRegister(config, REG_MODEM_CONFIG, BANDWIDTH_125K | ERROR_CODING_4_5);
    writeRegister(config, REG_MODEM_CONFIG2, SPREADING_7 | CRC_OFF);
    // Configura o cabeçalho implícito (Implicit Header)
    writeRegister(config, REG_MODEM_CONFIG3, 0x04); // LNA gain boost

    // --- Configuração da Potência de Transmissão (TX Power) ---
    writeRegister(config, REG_PA_CONFIG, 0x8F); // PA_BOOST com a potência máxima de 17 dBm

    // 3.4. Definir tamanho do Payload.
    writeRegister(config, REG_PAYLOAD_LENGTH, 15);
    writeRegister(config, REG_MAX_PAYLOAD_LENGTH, 15);

    // 3.5. Mudar para modo STANDBY
    writeRegister(config, REG_OPMODE, RF95_MODE_STANDBY);
    sleep_ms(10);
}

// Função para enviar um pacote
void lora_send_packet(lora_config_t *config, uint8_t* data, uint8_t len) {
    // 1. Entrar no modo STANDBY
    writeRegister(config, REG_OPMODE, RF95_MODE_STANDBY);
    // 2. Limpar a FIFO
    writeRegister(config, REG_FIFO_ADDR_PTR, 0x00);
    writeRegister(config, REG_FIFO_TX_BASE_AD, 0x00);
    // 3. Escrever os dados na FIFO
    for (int i = 0; i < len; i++) {
        writeRegister(config, REG_FIFO, data[i]);
    }
    // 4. Definir o tamanho do payload
    writeRegister(config, REG_PAYLOAD_LENGTH, len);
    // 5. Entrar no modo TX
    writeRegister(config, REG_OPMODE, RF95_MODE_TX);
    // 6. Esperar a transmissão terminar
    while ((readRegister(config, REG_IRQ_FLAGS) & 0x08) == 0); // Espera o TxDone
    // 7. Limpar o flag de interrupção TxDone
    writeRegister(config, REG_IRQ_FLAGS, 0xFF);
    // 8. Voltar para o modo STANDBY
    writeRegister(config, REG_OPMODE, RF95_MODE_STANDBY);
}

void lora_receive_continuous(lora_config_t *config) {
    // 1. Entrar no modo STANDBY
    writeRegister(config, REG_OPMODE, RF95_MODE_STANDBY);
    // 2. Limpar a FIFO
    writeRegister(config, REG_FIFO_ADDR_PTR, 0x00);
    // 3. Configurar o modo de recepção contínua
    writeRegister(config, REG_OPMODE, RF95_MODE_RX_CONTINUOUS);
    // 4. Habilitar interrupções de recepção
    writeRegister(config, REG_IRQ_FLAGS_MASK, 0x7F); // Habilita todas as interrupções menos TxDone
}

bool lora_receive_packet(lora_config_t *config, uint8_t* buffer, uint8_t* len) {
    // Verifica flags de interrupção
    uint8_t irq_flags = readRegister(config, REG_IRQ_FLAGS);

    // Verifica se um pacote foi recebido (bit RxDone = 1)
    if ((irq_flags & 0x40) != 0) {
        // Limpar flags de interrupção
        writeRegister(config, REG_IRQ_FLAGS, 0xFF);

        // Obter o tamanho do pacote recebido
        uint8_t packet_len = readRegister(config, REG_RX_NB_BYTES);
        *len = packet_len;

        // Obter o endereço atual do ponteiro FIFO RX
        uint8_t current_addr = readRegister(config, REG_FIFO_RX_CURRENT_ADDR);

        // Configurar o ponteiro FIFO para ler os dados
        writeRegister(config, REG_FIFO_ADDR_PTR, current_addr);

        // Ler os dados da FIFO
        for (int i = 0; i < packet_len; i++) {
            buffer[i] = readRegister(config, REG_FIFO);
        }

        // Terminar o buffer com null para tratá-lo como string
        buffer[packet_len] = '\0';

        return true;
    }

    return false;
}

