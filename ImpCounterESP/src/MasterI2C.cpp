#include "MasterI2C.h"
#include "Logging.h"
#include <Wire.h>
#include <Arduino.h>

#include "setup.h"


/* Set up I2c bus */
void MasterI2C::begin() {
	Wire.begin( SDA_PIN, SCL_PIN );
	Wire.setClock( 100000L );
	Wire.setClockStretchLimit(1500L);

	mode = TRANSMIT_MODE;
	sendCmd('M');
	mode = getByte();
}


/* Send a one byte command to slave */
void MasterI2C::sendCmd( const char cmd ) {
	Wire.beginTransmission( I2C_SLAVE_ADDR );
	if (Wire.write(cmd) != 1)
	{
		LOG_ERROR("I2C", "write cmd failed");
	}
	int err = Wire.endTransmission(true);
	if (err != 0)
	{
		LOG_ERROR("I2C end", err);
	}	
	delay( 1 ); // Because attiny is running slow cpu clock speed. Give him a little time to process the command.
}


/* Tell slave that we want to read the first byte of the storage */
void MasterI2C::gotoFirstByte() {
	sendCmd( 'D' );
}


/* Get the next byte from the slave. */
byte MasterI2C::getNextByte() {
	if (Wire.requestFrom( I2C_SLAVE_ADDR, 1 ) != 1)
	{
		LOG_ERROR("I2C", "requestFrom failed");
	}
	delay( 1 );
	byte rxByte = Wire.read();
	return rxByte;
}

/* Retrieves one byte from slave*/
uint8_t MasterI2C::getByte() {
	if (Wire.requestFrom( I2C_SLAVE_ADDR, 1 ) != 1)
	{
		LOG_ERROR("I2C", "requestFrom failed");
	}
	uint8_t value = Wire.read();
	return value;
}

/* Retrieves the full slave storage byte by byte. 
   Returns the number of bytes that we received.
   Retuns 0 if there are no bytes or if it doesn't fit in our buffer */
uint16_t MasterI2C::getSlaveStorage( byte* storage, const uint16_t maxStorageSize, const uint16_t bytesToFetch ) 
{
	uint16_t storagePos;

	if ( bytesToFetch <= maxStorageSize ) {
		LOG_NOTICE( "I2C", "Polling slave for " << bytesToFetch << " bytes" );
		gotoFirstByte();
		for ( storagePos = 0; storagePos < bytesToFetch; storagePos++ ) {
			byte rxByte = getNextByte();
			LOG_DEBUG( "I2C", "Byte Number " << storagePos << ": " << (int) rxByte );
			*( storage + storagePos ) = rxByte;
		}
		LOG_NOTICE( "I2C", "Data retreived" );
		return storagePos;
	} else {
		LOG_CRITICAL( "I2C", "Slave is returning too much data" );
		return 0;
	}
}


/* Returns the statistics from the slave */
Header MasterI2C::getHeader() {
	sendCmd( 'B' );
	Header header;

	header.bytesReady = getUint();
	header.version = getByte();
	header.masterWakeEvery = getUint();
	header.vcc = getUint();
	header.service = getByte();
	header.reserved = getUint();

	return header;
}


/* Retrieves one uint from slave*/
uint16_t MasterI2C::getUint() 
{
	byte i1 = getByte();
	byte i2 = getByte();
	return i1 | (i2 << 8);
}