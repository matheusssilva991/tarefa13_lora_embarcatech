#ifndef RFM95W_H
#define RFM95W_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

// --- REGISTRADORES DO SX127X ---
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_LNA                  0x0C
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1A
#define REG_MODEM_CONFIG_1       0x1D
#define REG_MODEM_CONFIG_2       0x1E
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4D

// --- MODOS DE OPERAÇÃO ---
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05

// --- CONFIGURAÇÃO PA (Power Amplifier) ---
#define PA_BOOST                 0x80

// --- FLAGS DE INTERRUPÇÃO (IRQ) ---
#define IRQ_TX_DONE_MASK         0x08
#define IRQ_RX_DONE_MASK         0x40
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20

// --- Estrutura para o dispositivo LoRa ---
typedef struct {
    spi_inst_t *spi_instance;
    uint cs_pin;
    uint rst_pin;
    uint dio0_pin;
    long frequency;
} lora_t;

/**
 * @brief Inicializa o módulo LoRa.
 * * @param lora Ponteiro para a estrutura lora_t.
 * @param spi_instance Instância do SPI (spi0 ou spi1).
 * @param frequency Frequência de operação em Hz (ex: 915E6 para 915 MHz).
 * @param cs_pin Pino de Chip Select.
 * @param rst_pin Pino de Reset.
 * @param dio0_pin Pino de interrupção DIO0.
 * @return int 1 se a inicialização for bem-sucedida, 0 caso contrário.
 */
int lora_init(lora_t *lora, spi_inst_t *spi_instance, long frequency, uint cs_pin, uint rst_pin, uint dio0_pin);

/**
 * @brief Define a frequência de operação.
 * * @param lora Ponteiro para a estrutura lora_t.
 * @param frequency Frequência em Hz.
 */
void lora_set_frequency(lora_t *lora, long frequency);

/**
 * @brief Envia um pacote de dados.
 * * @param lora Ponteiro para a estrutura lora_t.
 * @param buffer Ponteiro para os dados a serem enviados.
 * @param size Tamanho dos dados em bytes.
 */
void lora_send_packet(lora_t *lora, uint8_t *buffer, int size);

/**
 * @brief Verifica e recebe um pacote de dados.
 * * @param lora Ponteiro para a estrutura lora_t.
 * @param buffer Buffer para armazenar os dados recebidos.
 * @param max_size Tamanho máximo do buffer.
 * @return int O número de bytes recebidos, ou 0 se nenhum pacote foi recebido.
 */
int lora_receive_packet(lora_t *lora, uint8_t *buffer, int max_size);

/**
 * @brief Retorna o RSSI (Received Signal Strength Indicator) do último pacote.
 * * @param lora Ponteiro para a estrutura lora_t.
 * @return int Valor do RSSI em dBm.
 */
int lora_packet_rssi(lora_t *lora);

/**
 * @brief Coloca o módulo em modo de sono para economizar energia.
 * * @param lora Ponteiro para a estrutura lora_t.
 */
void lora_sleep(lora_t *lora);

/**
 * @brief Coloca o módulo em modo de espera (standby).
 * * @param lora Ponteiro para a estrutura lora_t.
 */
void lora_idle(lora_t *lora);

/**
 * @brief Coloca o módulo em modo de recepção contínua.
 * * @param lora Ponteiro para a estrutura lora_t.
 */
void lora_receive_mode(lora_t *lora);

#endif // RFM95W_H
