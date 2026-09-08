#ifndef HAL_SEVENSEGMENTSEVENSEGMENT_INT_H_
#define HAL_SEVENSEGMENTSEVENSEGMENT_INT_H_

#define SEVSEG_COMMON_CATHODE    0
#define SEVSEG_COMMON_ANODE      1

typedef struct {
    u8 Port;
    u8 StartPin;       //pins must be consecutive
    u8 DisplayType;    // SEVSEG_COMMON_CATHODE or SEVSEG_COMMON_ANODE
} SevenSegment_t;


void HSEVSEG_vInit(const SevenSegment_t *A_SegCfg);
void HSEVSEG_vDisplayNumber(const SevenSegment_t *A_SegCfg, u8 A_u8Number);



#endif /* HAL_SEVENSEGMENT_SEVENSEGMENT_INT_H_ */
