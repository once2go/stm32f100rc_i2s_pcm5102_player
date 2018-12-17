/*
 * sd.h
 *
 *  Created on: Dec 16, 2018
 *      Author: once2go
 */

#ifndef SD_H_
#define SD_H_

#include "stm32l1xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* Card type flags (CardType) */
#define CS_SD_GPIO_PORT GPIOA
#define CS_SD_PIN GPIO_PIN_8
#define SS_SD_SELECT() HAL_GPIO_WritePin(CS_SD_GPIO_PORT, CS_SD_PIN, GPIO_PIN_RESET)
#define SS_SD_DESELECT() HAL_GPIO_WritePin(CS_SD_GPIO_PORT, CS_SD_PIN, GPIO_PIN_SET)
#define LD_ON HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
#define LD_OFF HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);

//sd card types
#define CT_MMC 0x01 /* MMC ver 3 */
#define CT_SD1 0x02 /* SD ver 1 */
#define CT_SD2 0x04 /* SD ver 2 */
#define CT_SDC (CT_SD1|CT_SD2) /* SD */
#define CT_BLOCK 0x08 /* Block addressing */


 // Definitions for MMC/SDC command
#define CMD0 (0x40+0) // GO_IDLE_STATE
#define CMD1 (0x40+1) // SEND_OP_COND (MMC)
#define ACMD41 (0xC0+41) // SEND_OP_COND (SDC)
#define CMD8 (0x40+8) // SEND_IF_COND
#define CMD9 (0x40+9) // SEND_CSD
#define CMD16 (0x40+16) // SET_BLOCKLEN
#define CMD17 (0x40+17) // READ_SINGLE_BLOCK
#define CMD24 (0x40+24) // WRITE_BLOCK
#define CMD55 (0x40+55) // APP_CMD
#define CMD58 (0x40+58) // READ_OCR

typedef struct sd_info {
	volatile uint8_t type; // sd type
} sd_info_ptr;

uint8_t sd_init();
void SPI_Release(void);
uint8_t SD_cmd(uint8_t cmd, uint32_t arg);
uint8_t SPI_ReceiveByte(void);
uint8_t SD_Read_Block(uint8_t *buff, uint32_t lba);
uint8_t SD_Write_Block(uint8_t *buff, uint32_t lba);

#endif /* SD_H_ */
