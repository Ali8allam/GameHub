/*
 * FMI_prv.h
 *
 *  Created on: Sep 9, 2026
 *      Author: ALI & ADHM
 */

#ifndef MCAL_FMI_FMI_PRV_H_
#define MCAL_FMI_FMI_PRV_H_

#define FMI_BASE_ADRR   0x40022000U

#define KEY1   0x45670123U
#define KEY2   0xCDEF89ABU

#define BSY 0
#define STRT 6
#define PG 0
#define PER 1
#define MER 2
#define LOCK 7




typedef struct {
	u32 ACR;
	u32 KEYR;
	u32 OPTKEYR;
	u32 SR;
	u32 CR;
	u32 AR;
	u32 Reserved;
	u32 OPTCR;
	u32 OBR;
	u32 WPRP;
	}FLASH_MemMap_t;

#define FMI	((volatile FLASH_MemMap_t*)(FMI_BASE_ADRR))


#endif /* MCAL_FMI_FMI_PRV_H_ */
