/*
 * SPI_prv.h
 *
 *  Created on: Sep 2, 2026
 *      Author: ALI & ADHM
 */

#ifndef MCAL_SPI_SPI_PRV_H_
#define MCAL_SPI_SPI_PRV_H_

// CR1 important bits
#define DFF 11
#define SSM 9
#define SSI 8
#define LSBFIRST 7
#define SPE 6
#define MSTR 2
#define CPOL 1
#define CPHA 0
// SR important bits
#define TXE 1
#define RXNE 0


#define SPI1_BASE_ADDR 0x40013000U

typedef struct {
     u32 CR1;
     u32 CR2;
     u32 SR;
     u32 DR;
     u32 CRCPR;
     u32 RXCRCR;
     u32 TXCRCR;
     u32 I2SCFGR;
     u32 I2SPR;

} SPI1_MemMap_t;

#define SPI1		((volatile SPI1_MemMap_t*)(SPI1_BASE_ADDR)) // found on bus APB2


#endif /* MCAL_SPI_SPI_PRV_H_ */
