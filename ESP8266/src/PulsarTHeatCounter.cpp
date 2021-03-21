#include "PulsarTHeatCounter.h"

int PulsarTHeatCounter::readActualValueF(uint8_t channel, retval_float_t* retValue)
{
    sendCommand_ReadData(HEAT_F_READ_ACTUAL_VALUES, channel);
    //delay(5000);
    *retValue = readResponseF(channel); 
    return flag_Error;
}

retval_float_t PulsarTHeatCounter::readResponseF(uint8_t channel)
{
    LOG(F("readResponse:Begin.\n"))
    resetBuffer();
    unsigned long stopTime = millis() + RESPONSE_READ_TIMEOUT_MS;
    int iParsed = ERR_PACKET_PARSE_NO_END;
    while (millis() < stopTime)// читаем порт пока не прошел таймаут
    {
        while (port->available())// read all available bytes
        {
           buff[iBuff++] = (char)port->read();
           if (iBuff>HEAT_BUF_LENGTH)
           {
               LOG("Read buffer overflow.");
               iBuff = 0;
           } 
        };
        // если байты закончились - пробуем парсить пакет.
       
        iParsed = parsePacket(channel);
        if ((iParsed == ERR_PACKET_PARSE_FAILED) || (iParsed>=0))
        {
            LOG(F("readResponse:Exit receive packet cycle.\n"))
            #ifdef PULSAR_DBG
                LOG(F("Received buffer: <<"))
                for (int a=0;a<iBuff;a++)
                    LOGF("%8X ",buff[a])
                LOG(F(">>\n"))
            #endif
            break;// весь пакет принят 
        }
    }
    
    if (iParsed >= 0)//packet parsed OK
    {
        setError(ERR_OK);
        //copy value bytes from buffer
        retval_float_t valData;
        memcpy(&valData,&buff[PACKET_I_DATA_START],sizeof valData);
        return valData;
    }
    LOG(F("readResponse: Error reading response.\n"))
    setError(iParsed);
    return -1;// some errors occured
}

int PulsarTHeatCounter::parsePacket(uint8_t channel)
{
    //wait we got length byte
    if (iBuff<=PACKET_I_PACKET_LEN)
        return ERR_PACKET_PARSE_NO_END;
    //now wait finish packet
    //if (iBuff<buff[PACKET_I_PACKET_LEN])
    //    return ERR_PACKET_PARSE_NO_END;
    
    // next byte after F is packet length
    //parse packet length
    //int len = buff[PACKET_I_PACKET_LEN];
    if (iBuff < buff[PACKET_I_PACKET_LEN])
    {
        //this is not end yet
        return ERR_PACKET_PARSE_NO_END;
    }
    else if (iBuff > buff[PACKET_I_PACKET_LEN])
    {
        LOG(F("parsePacket: Length (L) check failed, data too long.\n"));
        LOGF2("Expected:%i; Actual:%i\n",(int)buff[PACKET_I_PACKET_LEN], iBuff);
        return ERR_PACKET_PARSE_OVERSIZE; 
    }
    //parse crc
    uint16_t crc = 0;
    crc = buff[iBuff-1]<<8;
    crc |= buff[iBuff-2];
    uint16_t crcReceived = calcChecksum((uint8_t*)buff, buff[PACKET_I_PACKET_LEN] - 2);
    if (crc != crcReceived)
    {
            LOG(F("parsePacket: CRC check failed.\n"));
                for (int k=0;k<16;k+=8)
                {
                    port->write((((0x00FF<<k) & crc) >> k));
                };
                for (int k=0;k<16;k+=8)
                {
                    port->write((((0x00FF<<k) & crcReceived) >> k));
                };
            return ERR_PACKET_PARSE_CRC_FAILED; 
    }
    //parse address
    unsigned char i=0;
    for (i=0;i<HEAT_ADDR_LENGTH;i++)
    {
        if (buff[i] != address[i])
        {
            LOG(F("parsePacket: Address check failed.\n"))
            return ERR_PACKET_PARSE_ADDRESS_FAILED;
        }
    }
    //parse request code
    unsigned char curRequestCode = buff[i++];
    if (curRequestCode != requestCode)
    {
        if (curRequestCode == HEAT_F_ERROR_COMMAND_SENT)
        {
            LOG(F("parsePacket: Receiver error 'Incorrect command'.\n"))
            return ERR_PACKET_PARSE_BAD_COMMAND;    
        }    
        LOG(F("parsePacket: Request code (F) check failed"));
        LOGF2(": %8x (expected %8x)\n", curRequestCode, requestCode);
        return ERR_PACKET_PARSE_BAD_CODE;
    }
    
    // reading data
    int iDataLast = (iBuff-1)-4;// -crc16 (2 byte) - ID(2 byte)
    LOG(F("Parsed OK\n"));
    return iDataLast;
}

bool PulsarTHeatCounter::isSuccess()
{
    return flag_Error >= ERR_OK;
}

void PulsarTHeatCounter::setError(int errCode)
{
    if (flag_Error == ERR_OK)// do not clear previous error for now
        flag_Error = errCode;
}

void PulsarTHeatCounter::writeBuffer(char* data, int len)
{
    if ((iBuff + len) < HEAT_BUF_LENGTH)
    {
        memcpy (&buff[iBuff], data, len);
        iBuff+=len;
    }
    else
    {
        setError(ERR_BUFFER_OVERFLOW);
        LOG(F("writeBuffer: Buffer overflow."));
        resetBuffer();    
    }
}

void PulsarTHeatCounter::writeBuffer(char data)
{
    buff[iBuff++] = data;
    if (iBuff >= HEAT_BUF_LENGTH)
    {
        resetBuffer();        
        LOG(F("ERROR: Write buffer overflow"))
    }    
}

void PulsarTHeatCounter::resetBuffer()
{
    iBuff = 0;   
    memset(&buff, 0x00, HEAT_BUF_LENGTH);
}


void PulsarTHeatCounter::sendCommand_ReadData(uint8_t code, uint8_t channel)
{
    flag_Error = ERR_OK;// reset errors
    LOG (F("Send ReadData command\n"))
    // packet length
    uint8_t len = calcPacketLength(4); // channel mask, 4 bytes
    resetBuffer();
    writeBuffer(address, HEAT_ADDR_LENGTH);
    //fill F
    writeBuffer(code);
    requestCode = code;
    //fill L
    writeBuffer(len);
    // data; make channel mask
    //fill data
    uint32_t data = 0x01<<channel;
    writeBuffer(((char*)&data), 4);
    //fill ID
    writeBuffer(0xFA);
    writeBuffer(0xFE);
    // calc buffer crc
    uint16_t crc = calcChecksum((uint8_t*)buff,len-2);
    //write crc to buffer
    writeBuffer(((char*)&crc), 2);
   
    #ifdef PULSAR_DBG
        LOG(F("Start send buffer: <<"))
        for (int a=0;a<iBuff;a++)
        {
        LOGF("%8X ",buff[a])
        }
        LOG(F(">>\n"))
    #endif

    port->write(buff,iBuff);
    resetBuffer();
}

void PulsarTHeatCounter::begin(Stream* port, char* address)
{
  this->port = port;
  this->address = address;
}  

uint16_t PulsarTHeatCounter::calcChecksum (uint8_t *Data, uint16_t size)
{
    uint16_t w;
    uint8_t shift_cnt,f;
    uint8_t *ptrByte;
    uint16_t byte_cnt = size;
    ptrByte = Data;
    w = (uint16_t)0xffff;
    for (;byte_cnt>0;byte_cnt--)
    {
        w = (uint16_t)(w^(uint16_t)(*ptrByte++));
        for (shift_cnt = 0; shift_cnt<8; shift_cnt++)
        {
            f=(uint8_t)((w)&(0x1));
            w>>=1;
            if ((f) ==1)
            w = (uint16_t)((w)^0xa001);
        }
    }
    return w;
}

uint8_t  PulsarTHeatCounter::calcPacketLength(uint8_t dataLen)
{
    //packet: | ADDR(4) | F(1) | L(1) | DATA(n) | ID(2) | CRC16(2)|
    return HEAT_ADDR_LENGTH + dataLen + 6;
}
