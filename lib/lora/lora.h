//lora.h
/* EMBARCATECH - INTRODUÇÃO AO PROTOCOLO LORA - CAPÍTULO 3 / PARTE 5
 * BitDogLAB - Transmissor LoRa (TX)
 * Biblioteca com endereços do registradores do SX1276 - Módulo LoRa.
 * Prof: Ricardo Prates
 */

#ifndef LORA_INCLUDED
#define LORA_INCLUDED

#include "hardware/spi.h"
#include "pico/stdlib.h"

// Registradores
#define REG_MAX_PAYLOAD_LENGTH      0x23
#define REG_FIFO                    0x00
#define REG_OPMODE                  0x01            //IMPORTANTE
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13            //IMPORTANTE
#define REG_MODEM_CONFIG            0x1D            //IMPORTANTE
#define REG_MODEM_CONFIG2           0x1E            //IMPORTANTE
#define REG_MODEM_CONFIG3           0x26            //IMPORTANTE
#define REG_PREAMBLE_MSB            0x20
#define REG_PREAMBLE_LSB            0x21
#define REG_PAYLOAD_LENGTH          0x22            //IMPORTANTE
#define REG_HOP_PERIOD              0x24
#define REG_FREQ_ERROR              0x28
#define REG_DETECT_OPT              0x31            //IMPORTANTE
#define	REG_DETECTION_THRESHOLD     0x37            //IMPORTANTE
#define REG_DIO_MAPPING_1           0x40            //IMPORTANTE
#define REG_DIO_MAPPING_2           0x41            //IMPORTANTE

// FSK stuff
#define REG_PREAMBLE_MSB_FSK        0x25
#define REG_PREAMBLE_LSB_FSK        0x26
#define REG_PACKET_CONFIG1          0x30
#define REG_PAYLOAD_LENGTH_FSK      0x32
#define REG_FIFO_THRESH             0x35
#define REG_FDEV_MSB                0x04
#define REG_FDEV_LSB                0x05
#define REG_FRF_MSB                 0x06            //IMPORTANTE
#define REG_FRF_MID                 0x07            //IMPORTANTE
#define REG_FRF_LSB                 0x08            //IMPORTANTE
#define REG_BITRATE_MSB             0x02
#define REG_BITRATE_LSB             0x03
#define REG_IRQ_FLAGS2              0x3F

// MODOS DE OPERAÇÃO
#define RF95_MODE_RX_CONTINUOUS     0x85
#define RF95_MODE_TX                0x83
#define RF95_MODE_SLEEP             0x80
#define RF95_MODE_STANDBY           0x81

#define PAYLOAD_LENGTH              255

// CONFIGURAÇÃO DO PACOTE DE DADOS
#define EXPLICIT_MODE               0x00
#define IMPLICIT_MODE               0x01

#define ERROR_CODING_4_5            0x02
#define ERROR_CODING_4_6            0x04
#define ERROR_CODING_4_7            0x06
#define ERROR_CODING_4_8            0x08

// CONFIGURAÇÃO DA LARGURA DE BANDA (BW)
#define BANDWIDTH_7K8               0x00
#define BANDWIDTH_10K4              0x10
#define BANDWIDTH_15K6              0x20
#define BANDWIDTH_20K8              0x30
#define BANDWIDTH_31K25             0x40
#define BANDWIDTH_41K7              0x50
#define BANDWIDTH_62K5              0x60
#define BANDWIDTH_125K              0x70
#define BANDWIDTH_250K              0x80
#define BANDWIDTH_500K              0x90

// COFIGURAÇÃO DO SPREADING FACTOR (FS)
#define SPREADING_6                 0x60
#define SPREADING_7                 0x70
#define SPREADING_8                 0x80
#define SPREADING_9                 0x90
#define SPREADING_10                0xA0
#define SPREADING_11                0xB0
#define SPREADING_12                0xC0

#define CRC_OFF                     0x00
#define CRC_ON                      0x04

// POWER AMPLIFIER CONFIG
#define REG_PA_CONFIG               0x09
#define PA_MAX_BOOST                0x8F    // 100mW (max 869.4 - 869.65)
#define PA_LOW_BOOST                0x81
#define PA_MED_BOOST                0x8A
#define PA_MAX_UK                   0x88    // 10mW (max 434)
#define PA_OFF_BOOST                0x00
#define RFO_MIN                     0x00

// 20DBm
#define REG_PA_DAC                  0x4D
#define PA_DAC_20                   0x87

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23  // 0010 0011
#define LNA_OFF_GAIN                0x00

// Define uma union para converter float para um array de bytes
typedef union {
    float f;
    uint8_t b[4];
} float_to_byte;

// Estrutura para configuração do módulo LoRa
typedef struct {
    spi_inst_t *spi;
    uint8_t pin_cs;
    uint8_t pin_rst;
    uint8_t pin_sck;
    uint8_t pin_mosi;
    uint8_t pin_miso;
} lora_config_t;

// Protótipos de funções atualizados
void lora_setup(lora_config_t *config);
void lora_send_packet(lora_config_t *config, uint8_t* data, uint8_t len);
void SetFrequency(lora_config_t *config, double Frequency);
void writeRegister(lora_config_t *config, uint8_t reg, uint8_t value);
uint8_t readRegister(lora_config_t *config, uint8_t reg);
void cs_select(uint8_t pin_cs);
void cs_deselect(uint8_t pin_cs);
void lora_receive_continuous(lora_config_t *config);
bool lora_receive_packet(lora_config_t *config, uint8_t *buffer, uint8_t *len);

#endif
