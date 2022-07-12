#pragma once

#include "CircularBuffer.h"
#include "TimeoutHelper.h"
#include "types.h"
#include <RF24.h>
#include <memory>
#include <nRF24L01.h>

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30

#define TX_BUFFER_SIZE 5

#define MAX_RESEND_COUNT 3

#ifdef OLIMEX_UEXT_MOD_NRF24LR
    // look @ https://nrf24.github.io/RF24/md_docs_arduino.html
    
    #define HOYMILES_PIN_MISO 15
    #define HOYMILES_PIN_MOSI 2
    #define HOYMILES_PIN_SCLK 14 
    #define HOYMILES_PIN_IRQ 13
    #define HOYMILES_PIN_CE 16
    #define HOYMILES_PIN_CS 5

#else

    #ifndef HOYMILES_PIN_IRQ
    #define HOYMILES_PIN_IRQ 16
    #endif

    #ifndef HOYMILES_PIN_CE
    #define HOYMILES_PIN_CE 4
    #endif

    #ifndef HOYMILES_PIN_CS
    #define HOYMILES_PIN_CS 5
    #endif

#endif

class HoymilesRadio {
public:
    void init();
    void loop();
    void setPALevel(rf24_pa_dbm_e paLevel);

    serial_u DtuSerial();
    void setDtuSerial(uint64_t serial);

    bool isIdle();
    void sendEsbPacket(serial_u target, uint8_t mainCmd, uint8_t subCmd, uint8_t payload[], uint8_t len, uint32_t timeout, bool resend = false);
    void sendRetransmitPacket(uint8_t fragment_id);
    void sendLastPacketAgain();
    bool enqueTransaction(inverter_transaction_t* transaction);

    static void u32CpyLittleEndian(uint8_t dest[], uint32_t src);

private:
    void ARDUINO_ISR_ATTR handleIntr();
    static serial_u convertSerialToRadioId(serial_u serial);
    static void convertSerialToPacketId(uint8_t buffer[], serial_u serial);
    uint8_t getRxNxtChannel();
    uint8_t getTxNxtChannel();
    void switchRxCh();
    void openReadingPipe();
    void openWritingPipe(serial_u serial);
    bool checkFragmentCrc(fragment_t* fragment);
    void dumpBuf(const char* info, uint8_t buf[], uint8_t len);

#ifdef OLIMEX_UEXT_MOD_NRF24LR
    std::unique_ptr<SPIClass> _hspi;
#endif
    std::unique_ptr<RF24> _radio;
    uint8_t _rxChLst[5] = { 3, 23, 40, 61, 75 };
    uint8_t _rxChIdx;

    uint8_t _txChLst[1] = { 40 };
    uint8_t _txChIdx;

    volatile bool _packetReceived;

    CircularBuffer<fragment_t, FRAGMENT_BUFFER_SIZE> _rxBuffer;
    TimeoutHelper _rxTimeout;

    serial_u _dtuSerial;

    bool _busyFlag = false;

    inverter_transaction_t currentTransaction;
    CircularBuffer<inverter_transaction_t, TX_BUFFER_SIZE> _txBuffer;
};
