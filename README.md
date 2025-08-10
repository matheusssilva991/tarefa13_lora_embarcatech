# Sistema LoRa com Sensores Ambientais para Raspberry Pi Pico

Projeto embarcado para comunicação sem fio via LoRa utilizando Raspberry Pi Pico, com aquisição de dados ambientais (temperatura, umidade e pressão) através dos sensores AHT20 e BMP280.

## 📋 Descrição do Projeto

O sistema é composto por dois programas principais:
- **Transmissor (main_tx.c):** lê dados dos sensores ambientais (AHT20 e BMP280) e envia via rádio LoRa (RFM95W).
- **Receptor (main_rx.c):** recebe os dados LoRa e exibe no terminal serial.

Ideal para aplicações de telemetria, monitoramento ambiental e projetos de IoT de longo alcance.

## ⚡ Funcionalidades

- Comunicação LoRa ponto-a-ponto (RFM95W/SX1276)
- Leitura de temperatura e umidade (AHT20)
- Leitura de pressão e temperatura (BMP280)
- Transmissão e recepção de dados ambientais
- Código modular e fácil de adaptar

## 🛠️ Hardware Utilizado

- **Microcontrolador:** Raspberry Pi Pico
- **Módulo LoRa:** RFM95W (SX1276)
- **Sensor de Temperatura/Umidade:** AHT20 (I2C)
- **Sensor de Pressão/Temperatura:** BMP280 (I2C)

### Ligações Básicas

**SPI (LoRa RFM95W):**
| Pico | RFM95W |
|------|--------|
| 16   | MISO   |
| 17   | CS     |
| 18   | SCK    |
| 19   | MOSI   |
| 20   | RST    |

**I2C0 (BMP280):**
| Pico | BMP280 |
|------|--------|
| 0    | SDA    |
| 1    | SCL    |

**I2C1 (AHT20):**
| Pico | AHT20  |
|------|--------|
| 2    | SDA    |
| 3    | SCL    |

## 🚀 Como Compilar e Rodar

### Pré-requisitos
- Raspberry Pi Pico
- SDK C/C++ do Raspberry Pi Pico instalado
- CMake e ferramentas de build (ninja ou make)

### Passos
```bash
git clone https://github.com/matheusssilva991/tarefa13_lora_embarcatech.git
cd tarefa13_lora_embarcatech
mkdir build
cd build
cmake ..
ninja   # ou make
# Conecte o Pico em modo BOOTSEL
# Copie main.uf2 para o dispositivo RPI-RP2
```

### Executando
- Para transmitir: grave o `main.uf2` gerado a partir de `main.c` no Pico conectado ao transmissor.
- Para receber: grave o `main.uf2` gerado a partir de `main_rx.c` no Pico conectado ao receptor.
- Use um monitor serial (baudrate 115200) para visualizar os dados recebidos.

## 📁 Estrutura do Projeto

```
tarefa13_lora_embarcatech/
│
├── main.c           # Código do transmissor LoRa (envia dados dos sensores)
├── main_rx.c        # Código do receptor LoRa (recebe e exibe dados)
├── lib/
│   ├── lora/        # Definições e registradores LoRa
│   ├── rfm95w/      # Driver do módulo LoRa RFM95W
│   └── sensores/    # Drivers dos sensores AHT20 e BMP280
├── CMakeLists.txt   # Configuração do projeto
└── README.md        # Documentação
```

## �️ Diagrama Simplificado

```
┌──────────────┐      SPI      ┌──────────────┐
│ Raspberry Pi │◄────────────►│   RFM95W     │
│    Pico      │              └──────────────┘
│              │
│   I2C0  I2C1 │
│   │      │   │
│  BMP280 AHT20│
└────┬─────┬───┘
    │     │
  Pressão  │
 Temperatura/Umidade
```

## 👤 Desenvolvedor

- [Matheus Santos Silva](https://github.com/matheusssilva991)
- [Leonardo Bonifácio Vieira Santos](https://github.com/LeonardoBonifacio)
