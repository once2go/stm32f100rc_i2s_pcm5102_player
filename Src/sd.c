/*
 * sd.c
 *
 *  Created on: Dec 16, 2018
 *      Author: once2go
 */

#include "sd.h"

extern SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart1;

extern volatile uint16_t Timer1;
sd_info_ptr sdinfo;
char str1[60] = { 0 };

uint8_t SD_Read_Block(uint8_t *buff, uint32_t lba) {
	uint8_t result;
	uint16_t cnt;
	result = SD_cmd(CMD17, lba);
	if (result != 0x00) {
		return 5;
	}
	SPI_Release();
	do { // waiting when block begin
		result = SPI_ReceiveByte();
		cnt++;

	} while ((result != 0xFE) && (cnt < 0xFFFF));
	if (cnt >= 0xFFFF) {
		return 5;
	}
	for (cnt = 0; cnt < 512; cnt++) {
		buff[cnt] = SPI_ReceiveByte(); //Set buss data to buffer
	}
	SPI_Release(); //Skip checksum
	SPI_Release();
	return 0;
}

uint8_t SD_Write_Block(uint8_t *buff, uint32_t lba) {
	uint8_t result;
	uint16_t cnt;
	result = SD_cmd(CMD24, lba); //CMD24 datasheet pg. 51 & 97-98
	if (result != 0x00) {
		return 6; //return if result not 0x00
	}
	SPI_Release();
	SPI_SendByte(0xFE); //Start buffer
	for (cnt = 0; cnt < 512; cnt++) {
		SPI_SendByte(buff[cnt]); //DATA
	}
	SPI_Release(); //Skip checksum
	SPI_Release();
	result = SPI_ReceiveByte();
	if ((result & 0x05) != 0x05) {
		return 6; //return if result not 0x05 (datasheet pg 111)
	}
	cnt = 0;
	do { //Wait until BUSY state end
		result = SPI_ReceiveByte();
		cnt++;
	} while ((result != 0xFF) && (cnt < 0xFFFF));
	if (cnt >= 0xFFFF) {
		return 6;
	}
	return 0;

}

uint8_t sd_init() {
	uint8_t cmd, i;
	uint16_t tmr;
	uint8_t ocr[4];
	HAL_Delay(100);
	LD_OFF
	sdinfo.type = 0;
	SS_SD_DESELECT();
	for (int i = 0; i < 10; i++) {
		SPI_Release();
	}
	SS_SD_SELECT();
	if (SD_cmd(CMD0, 0) == 1) { // Enter Idle state
		SPI_Release();
		if (SD_cmd(CMD8, 0x1AA) == 1) { // SDv2
			for (i = 0; i < 4; i++) {
				ocr[i] = SPI_ReceiveByte();
			}
			sprintf(str1, "OCR: 0x%02X 0x%02X 0x%02X 0x%02Xrn", ocr[0], ocr[1],
					ocr[2], ocr[3]);
			HAL_UART_Transmit(&huart1, (uint8_t*) str1, strlen(str1), 0x1000);
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) { // The card can work at vdd range of 2.7-3.6V
				for (tmr = 12000; tmr && SD_cmd(ACMD41, 1UL << 30); tmr--)
					; // Wait for leaving idle state (ACMD41 with HCS bit)
				if (tmr && SD_cmd(CMD58, 0) == 0) { // Check CCS bit in the OCR
					for ( i = 0; i < 4; i++) {
						ocr[i] = SPI_ReceiveByte();
					}
					sprintf(str1, "OCR: 0x%02X 0x%02X 0x%02X 0x%02Xrn", ocr[0],
							ocr[1], ocr[2], ocr[3]);
					HAL_UART_Transmit(&huart1, (uint8_t*) str1, strlen(str1),
							0x1000);
					sdinfo.type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; // SDv2 (HC or SC)
				}
			}
		}
	} else { //SDv1 or MMCv3
		if (SD_cmd(ACMD41, 0) <= 1) {
			sdinfo.type = CT_SD1;
			cmd = ACMD41; // SDv1
		} else {
			sdinfo.type = CT_MMC;
			cmd = CMD1; // MMCv3
		}
		for (tmr = 25000; tmr && SD_cmd(cmd, 0); tmr--) { // Wait for leaving idle state

		}

		if (!tmr || SD_cmd(CMD16, 512) != 0) { // Set R/W block length to 512

		}
		sdinfo.type = 0;
		return 1;
	}
	sprintf(str1, "Type SD: 0x%02Xrn", sdinfo.type);
	HAL_UART_Transmit(&huart1, (uint8_t*) str1, strlen(str1), 0x1000);
	return 0;
}

uint8_t SD_cmd(uint8_t cmd, uint32_t arg) {
	uint8_t n, res;
	if (cmd & 0x80) {
		cmd &= 0x7F;
		res = SD_cmd(CMD55, 0);
		if (res > 1) {
			return res;
		}
	}
// Select the card
	SS_SD_DESELECT();
	SPI_ReceiveByte();
	SS_SD_SELECT();
	SPI_ReceiveByte();
// Send a command packet
	SPI_SendByte(cmd); // Start + Command index
	SPI_SendByte((uint8_t) (arg >> 24)); // Argument[31..24]
	SPI_SendByte((uint8_t) (arg >> 16)); // Argument[23..16]
	SPI_SendByte((uint8_t) (arg >> 8)); // Argument[15..8]
	SPI_SendByte((uint8_t) arg); // Argument[7..0]

	n = 0x01; // Dummy CRC + Stop
	if (cmd == CMD0) {
		n = 0x95;
	} // Valid CRC for CMD0(0)
	if (cmd == CMD8) {
		n = 0x87;
	} // Valid CRC for CMD8(0x1AA)
	SPI_SendByte(n);

// Receive a command response
	n = 10; // Wait for a valid response in timeout of 10 attempts
	do {
		res = SPI_ReceiveByte();
	} while ((res & 0x80) && --n);
	return res;
}

static void Error(void) {
	LD_ON
}

uint8_t SPIx_WriteRead(uint8_t Byte) {
	uint8_t receivedbyte = 0;
	if (HAL_SPI_TransmitReceive(&hspi2, (uint8_t*) &Byte,
			(uint8_t*) &receivedbyte, 1, 0x1000) != HAL_OK) {
		Error();
	}
	return receivedbyte;
}

void SPI_SendByte(uint8_t bt) {
	SPIx_WriteRead(bt);
}

uint8_t SPI_ReceiveByte(void) {
	uint8_t bt = SPIx_WriteRead(0xFF);
	return bt;
}

void SPI_Release(void) {
	SPIx_WriteRead(0xFF);
}
