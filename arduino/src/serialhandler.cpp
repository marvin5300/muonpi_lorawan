/*
 *  Baud rate = 9600Hz
 * Baud lenght= 1/9600=104µs
 *
 * ASIC reads on rising clock edge
 *
 *
 *
 * message Protocol arduino - raspi, every value equals one byte.
 * raspi -> arduino: <header> <multiplexer> <4command_bits+00+data_bit[9]+data_bit[8]> <data_lsb> <chipID> <chkA> <chkB>
 * arduino -> raspi: <header> <multiplexer> <4command_bits+00+data_bit[9]+data_bit[8]> <data_lsb> <chipID> <chkA> <chkB>
 *
 * checksum bytes are created from string NOT containing the header byte
 */
#include "serialhandler.h"
#include <Arduino.h>
#include <stdint.h>
#include <lmic.h>
#include <hal/hal.h>

const uint8_t MESSAGE_HEADER = 0xf9u;
constexpr size_t buffer_size = 0xff;

bool SerialHandler::read(String &str) {
	while (Serial.available() > 0)
	{
		uint8_t incomingByte = Serial.read();
		buf += static_cast<char>(incomingByte);
	}

	if (buf.length() < 4) {
		return false;
	}
	for (size_t i = 0; i < buf.length(); i++) {
		if (static_cast<uint8_t>(buf[i]) == MESSAGE_HEADER && buf.length() >= i + 4) { // header, size, data block, chkA, chkB => length >= 5
			uint8_t payload_size = static_cast<uint8_t>(buf[i+1]);
			if (buf.length() >= i + 4 + payload_size){
				str = "";
				for (size_t j = i + 2; j < i + 2 + payload_size; j++) {
					str += buf[j];
				}
				uint8_t chkA, chkB;
				fletcherChkSum(str, chkA, chkB);
				if (chkA != static_cast<uint8_t>(buf[i + 2 + payload_size])
				|| chkB != static_cast<uint8_t>(buf[i + 3 + payload_size])) {
					return false;
				}
				String temp = "";
				for (int k = i + 5 + payload_size; k < buf.length(); k++) {
					temp += buf[k];
				}
				buf = temp;

				// do stuff with the data
				return true;
			}
		}
	}
	return false;
}

void SerialHandler::fletcherChkSum(const String&  buf, uint8_t& chkA, uint8_t& chkB)
{
    // calc Fletcher checksum, ignore the message header (b5 62)
	chkA = 0;
    chkB = 0;
    for (size_t i = 0; i < buf.length(); i++)
    {
        chkA += static_cast<uint8_t>(buf[i]);
        chkB += chkA;
    }
}

bool SerialHandler::send(const char *data){
	return send(String(data));
}

bool SerialHandler::send(const String &data) {
	if (data.length() > 255u){
		return false;
	}
	String txBuf{""};
	txBuf += static_cast<char>(MESSAGE_HEADER);
	txBuf += static_cast<char>(data.length());
	txBuf += data;
	uint8_t chkA, chkB;
	fletcherChkSum(data, chkA, chkB);
	txBuf += static_cast<char>(chkA);
	txBuf += static_cast<char>(chkB);
	Serial.write(txBuf.c_str());
	// delay(100);
	// Serial.flush();
	return true;
}