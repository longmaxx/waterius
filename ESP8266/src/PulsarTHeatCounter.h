#ifndef PULSARTHEATCOUNTER_H
#define PULSARTHEATCOUNTER_H
#include <arduino.h>

// 32 bit float for return value
typedef float retval_float_t;
#define     RETVAL_FLOAT_SIZE (4)
// 32 bit integer for return value
typedef unsigned long retval_int_t;
#define     RETVAL_INT_SIZE (4)

#define RESPONSE_READ_TIMEOUT_MS (5000)
#define HEAT_BUF_LENGTH (50)
#define HEAT_ADDR_LENGTH (4)

//индекс начала данных в пакете
#define PACKET_I_PACKET_LEN (5)
#define PACKET_I_DATA_START (6)
// packet parse result
#define ERR_PACKET_PARSE_OVERSIZE  (-8)
#define ERR_PACKET_PARSE_CRC_FAILED (-7)
#define ERR_PACKET_PARSE_BAD_CODE  (-6)
#define ERR_PACKET_PARSE_BAD_COMMAND (-5)
#define ERR_PACKET_PARSE_ADDRESS_FAILED (-4)
#define ERR_PACKET_PARSE_NO_END   (-2)
#define ERR_PACKET_PARSE_FAILED   (-1)
#define ERR_BUFFER_OVERFLOW       (-3)
#define ERR_OK (0)

// F values
#define HEAT_F_ERROR_COMMAND_SENT (0x00)
#define HEAT_F_READ_ACTUAL_VALUES (0x01)
#define HEAT_F_READ_ACTUAL_TIME (0x04)
#define HEAT_F_WRITE_ACTUAL_TIME (0x05)
#define HEAT_F_READ_ARCHIVE_VALUES (0x06)
#define HEAT_F_READ_PREFERENCE_VALUES (0x0A)
#define HEAT_F_WRITE_PREFERENCE_VALUES (0x0B)

// бит канала чтения данных (результат float32_t)
#define HEAT_CHANNEL_T_PODVOD  (3)
#define HEAT_CHANNEL_T_OBRATKA (4)
#define HEAT_CHANNEL_T_PEREPAD  (5)
#define HEAT_CHANNEL_POWER (6)
#define HEAT_CHANNEL_ENERGY (7)
#define HEAT_CHANNEL_VOLUME (8)
#define HEAT_CHANNEL_FLOW (9)
#define HEAT_CHANNEL_IMP_IN1 (10)
#define HEAT_CHANNEL_IMP_IN2 (11)
#define HEAT_CHANNEL_IMP_IN3 (12)
#define HEAT_CHANNEL_IMP_IN4 (13)
#define HEAT_CHANNEL_FLOW_BY_ENERGY (14)
#define HEAT_CHANNEL_ENERGY_COOLING (21)
#define HEAT_CHANNEL_PRESSURE1 (22)
#define HEAT_CHANNEL_PRESSURE2 (23)
#define HEAT_CHANNEL_MASSA (24)
#define HEAT_CHANNEL_MASSA_COLD_TUBE_M2 (25)
#define HEAT_CHANNEL_MASSA_OTOBRANNOI_VODI (26)
#define HEAT_CHANNEL_VOLUME_COLD_WATER (27)
#define HEAT_CHANNEL_ENERGY_USED_WATER (28)

// бит канала чтения данных (результат unsigned int32)
#define HEAT_CHANNEL_NORMAL_WORK_TIME (20) /*uint32*/


#define PULSAR_DBG

#ifdef PULSAR_DBG
    #define LOG_PORT Serial
    #define LOG(msg) {LOG_PORT.print(msg);}
    #define LOGF(msg,val) {LOG_PORT.printf(msg,val);}
    #define LOGF2(msg,val1, val2) {LOG_PORT.printf(msg,val1, val2);}
#else    
    #define LOG(msg) {}
    #define LOGF(msg,val) {}
    #define LOGF2(msg,val) {}
 #endif   


class PulsarTHeatCounter
{  
    private: char* address; //HEAT_ADDR_LENGTH
    private: char buff[HEAT_BUF_LENGTH];
    private: int iBuff = 0;// текущий индекс ячейки записи в буфер
    private: unsigned char   requestCode;// текущий код запроса (F) 1 byte
    private: char data[];
    private: Stream* port;
    public: int flag_Error;
    
    private: void setError(int errCode);
    private: void writeBuffer(char* data, int len);
    private: void writeBuffer(char data);
    private: void resetBuffer();

    private: uint16_t calcChecksum(uint8_t* arrCmd, uint16_t len);
    private: uint8_t  calcPacketLength(uint8_t dataLen);
    private: void sendCommand_ReadData(uint8_t code, uint8_t channel);
    private: retval_float_t readResponseF(uint8_t channel);
    private: int parsePacket(uint8_t channel);

    public: void begin(Stream* port, char* address);//9600 baud, 8N1
    public: bool isSuccess();
    public: int readActualValueF(uint8_t channel, retval_float_t* retValue);
    //public: unsigned long readActualValueI(uint8_t channel);

    //public: int ActualCalories(retval_float_t* value);// текущая величина калорий
    //public: int ActualFlow(retval_float_t* value);// Расход воды (поток)
    //public: retval_float_t ActualTIncoming();// Температура воды вход
    //public: retval_float_t ActualTOutcoming();// Температура воды выход (обратка)
    //public: retval_float_t ActualTDifference();// Температура воды разница 
};


#endif
