/*
 * BUZZER_int.h
 *
 *  Created on: Sep 9, 2026
 *      Author: bigor
 */

#ifndef HAL_BUZZER_BUZZER_INT_H_
#define HAL_BUZZER_BUZZER_INT_H_


#include "../../LIB/STD_TYPES.h"

/*******************************************************************************
 *                             Musical Note Frequencies (Hz)
 *******************************************************************************/
#define NOTE_REST   0
#define NOTE_C4     262
#define NOTE_CS4    277
#define NOTE_D4     294
#define NOTE_DS4    311
#define NOTE_E4     330
#define NOTE_F4     349
#define NOTE_FS4    370
#define NOTE_G4     392
#define NOTE_GS4    415
#define NOTE_A4     440
#define NOTE_AS4    466
#define NOTE_B4     494
#define NOTE_C5     523
#define NOTE_CS5    554
#define NOTE_D5     587
#define NOTE_DS5    622
#define NOTE_E5     659
#define NOTE_F5     698
#define NOTE_FS5    740
#define NOTE_G5     784
#define NOTE_GS5    831
#define NOTE_A5     880
#define NOTE_AS5    932
#define NOTE_B5     988
#define NOTE_C6     1047
#define NOTE_E6     1319


typedef struct {
    u8 Port;
    u8 Pin;
} BUZZER_Config_t;

typedef struct {
    u16 frequency;
    u16 duration;
} Note_t;

typedef enum {
    SFX_BUTTON_CLICK,
    SFX_VICTORY
} BUZZER_SFX_t;


void HBUZZER_vInit(const BUZZER_Config_t* A_pxConfig);
void HBUZZER_vPlayTone(const BUZZER_Config_t* A_pxConfig, u32 A_u32FreqHz, u32 A_u32DurationMs);
void HBUZZER_vPlaySFX(const BUZZER_Config_t* A_pxConfig, BUZZER_SFX_t A_xSFX);
void HBUZZER_vPlayMelody(const BUZZER_Config_t* A_pxConfig, const Note_t* A_pxMelody, u16 A_u16Length);


#endif /* HAL_BUZZER_BUZZER_INT_H_ */
