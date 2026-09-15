/*
 * FMI_int.h
 *
 *  Created on: Sep 9, 2026
 *      Author: ALI & ADHM
 */

#ifndef MCAL_FMI_FMI_INT_H_
#define MCAL_FMI_FMI_INT_H_



void MFMI_vMassErase(void);

void MFMI_vPageErase(u32 A_u32PageAdrress);

void MFMI_vProgramFlash(u32 Au32Address , u16* A_PtrData, u16 DataLength);


#endif /* MCAL_FMI_FMI_INT_H_ */
