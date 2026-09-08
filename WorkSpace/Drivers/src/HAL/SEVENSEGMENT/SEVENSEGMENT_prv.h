

#ifndef HAL_SEVENSEGMENT_SEVENSEGMENT_PRV_H_
#define HAL_SEVENSEGMENT_SEVENSEGMENT_PRV_H_

// Common Cathode segment patterns for digits 0-9
static const u8 K_u8SevSegPatterns[10] = {  //K for -> CONST
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};



#endif
