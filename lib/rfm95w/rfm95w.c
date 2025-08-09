#include "rfm95w.h"

// --- Funções Privadas de Baixo Nível ---

static void lora_select(lora_t *lora) {
    gpio_put(lora->cs_pin, 0);
}

static void lora_deselect(lora_t *lora) {
    gpio_put(lora->cs_pin, 1);
}

static uint8_t lora_read_reg(lora_t *lora, uint8_t reg) {
    uint8_t val;
    lora_select(lora);
    reg &= 0x7F; // Garante que o bit de escrita seja 0
    spi_write_blocking(lora->spi_instance, &reg, 1);
    spi_read_blocking(lora->spi_instance, 0x00, &val, 1);
    lora_deselect(lora);
    return val;
}

static void lora_write_reg(lora_t *lora, uint8_t reg, uint8_t val) {
    lora_select(lora);
    reg |= 0x80; // Seta o bit de escrita para 1
    uint8_t buffer[2] = {reg, val};
    spi_write_blocking(lora->spi_instance, buffer, 2);
    lora_deselect(lora);
}

// --- Funções Públicas ---

void lora_sleep(lora_t *lora) {
    lora_write_reg(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

void lora_idle(lora_t *lora) {
    lora_write_reg(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void lora_set_frequency(lora_t *lora, long frequency) {
    lora->frequency = frequency;
    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
    lora_write_reg(lora, REG_FRF_MSB, (uint8_t)(frf >> 16));
    lora_write_reg(lora, REG_FRF_MID, (uint8_t)(frf >> 8));
    lora_write_reg(lora, REG_FRF_LSB, (uint8_t)(frf >> 0));
}

int lora_init(lora_t *lora, spi_inst_t *spi_instance, long frequency, uint cs_pin, uint rst_pin, uint dio0_pin) {
    lora->spi_instance = spi_instance;
    lora->cs_pin = cs_pin;
    lora->rst_pin = rst_pin;
    lora->dio0_pin = dio0_pin;

    // Configura os pinos de controle
    gpio_init(lora->cs_pin);
    gpio_set_dir(lora->cs_pin, GPIO_OUT);
    gpio_put(lora->cs_pin, 1);

    if (lora->rst_pin != -1) {
        gpio_init(lora->rst_pin);
        gpio_set_dir(lora->rst_pin, GPIO_OUT);
        // Reset do módulo
        gpio_put(lora->rst_pin, 0);
        sleep_ms(10);
        gpio_put(lora->rst_pin, 1);
        sleep_ms(10);
    }

    // Verifica a versão do chip
    uint8_t version = lora_read_reg(lora, REG_VERSION);
    if (version != 0x12) {
        return 0; // Falha na comunicação
    }

    // Coloca em modo sleep para configurar
    lora_sleep(lora);

    // Configura a frequência
    lora_set_frequency(lora, frequency);

    // Configura os endereços da FIFO
    lora_write_reg(lora, REG_FIFO_TX_BASE_ADDR, 0);
    lora_write_reg(lora, REG_FIFO_RX_BASE_ADDR, 0);

    // Configura LNA (Low Noise Amplifier)
    lora_write_reg(lora, REG_LNA, lora_read_reg(lora, REG_LNA) | 0x03);

    // Habilita o Auto AGC
    lora_write_reg(lora, REG_MODEM_CONFIG_3, 0x04);

    // Configura a potência de transmissão
    lora_write_reg(lora, REG_PA_CONFIG, 0x8F); // PA_BOOST, 17dBm

    // Coloca em modo standby
    lora_idle(lora);

    return 1;
}

void lora_send_packet(lora_t *lora, uint8_t *buffer, int size) {
    lora_idle(lora);

    // Posiciona o ponteiro da FIFO no início da área de TX
    lora_write_reg(lora, REG_FIFO_ADDR_PTR, 0);

    // Escreve os dados na FIFO
    for (int i = 0; i < size; i++) {
        lora_write_reg(lora, REG_FIFO, buffer[i]);
    }

    // Define o tamanho do payload
    lora_write_reg(lora, REG_PAYLOAD_LENGTH, size);

    // Coloca em modo de transmissão e aguarda o envio
    lora_write_reg(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
    while ((lora_read_reg(lora, REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0) {
        // Aguarda a transmissão ser concluída
    }

    // Limpa a flag de interrupção
    lora_write_reg(lora, REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);

    lora_idle(lora);
}

void lora_receive_mode(lora_t *lora) {
    // Mapeia a interrupção DIO0 para o evento RxDone
    lora_write_reg(lora, REG_DIO_MAPPING_1, 0x00);
    // Coloca em modo de recepção contínua
    lora_write_reg(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

int lora_receive_packet(lora_t *lora, uint8_t *buffer, int max_size) {
    int len = 0;
    uint8_t irq_flags = lora_read_reg(lora, REG_IRQ_FLAGS);

    // Limpa as flags de interrupção
    lora_write_reg(lora, REG_IRQ_FLAGS, irq_flags);

    // Verifica se um pacote foi recebido e se o CRC é válido
    if ((irq_flags & IRQ_RX_DONE_MASK) && ((irq_flags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0)) {
        len = lora_read_reg(lora, REG_RX_NB_BYTES);

        // Posiciona o ponteiro da FIFO no início da área de RX
        lora_write_reg(lora, REG_FIFO_ADDR_PTR, lora_read_reg(lora, REG_FIFO_RX_CURRENT_ADDR));

        // Lê os dados da FIFO
        for (int i = 0; i < len; i++) {
            if (i < max_size) {
                buffer[i] = lora_read_reg(lora, REG_FIFO);
            } else {
                lora_read_reg(lora, REG_FIFO); // Descarta o byte extra
            }
        }
    }

    return len;
}

int lora_packet_rssi(lora_t *lora) {
    // A fórmula para o RSSI depende da frequência
    int rssi_offset = (lora->frequency < 868E6) ? -164 : -157;
    return lora_read_reg(lora, REG_PKT_RSSI_VALUE) + rssi_offset;
}
