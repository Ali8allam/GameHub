#ifndef HAL_DAC_INT_H_
#define HAL_DAC_INT_H_

#include "../../LIB/STD_TYPES.h"
#include "../../MCAL/GPIO/GPIO_int.h"

/* DAC Initialization and Low-Level API.
 * A_xPins must list the 8 DAC data pins ordered LSB -> MSB (bit 0 of the
 * sample drives A_xPins[0]). */
void HDAC_vInit(const GPIOx_PinConfig_t *A_xPins, u8 A_u8PinsNo);
void HDAC_vOutputByte(u8 A_u8Sample);
void HDAC_vSendSample(const u8 *A_u8Ptr, u32 A_u32Index);
void HDAC_vStopStreaming(void);

/* Blocking audio playback (blocks for the full length of the clip). */
void HDAC_vPlaySoundSync(const u8 *A_u8Samples, u32 A_u32Length);
void HDAC_vPlayStart(void);
void HDAC_vPlayDeath(void);
void HDAC_vPlayPowerPellet(void);

/* Per-tick audio streaming during the game loop (one call per 50 ms tick).
 * frightened_mode loops the power-pellet siren, otherwise pacman_moving
 * loops waka-waka, otherwise the DAC is held at silence. */
void HDAC_vTickAudio(u8 pacman_moving, u8 frightened_mode);

#endif /* HAL_DAC_INT_H_ */