#ifndef __MBOX_H__
#define __MBOX_H__

#include <stdint.h>

#define MBOX_REQUEST          0x00000000
#define MBOX_RESPONSE_SUCCESS 0x80000000

#define MBOX_CH_POWER 0
#define MBOX_CH_FB    1
#define MBOX_CH_VUART 2
#define MBOX_CH_VCHIQ 3
#define MBOX_CH_LEDS  4
#define MBOX_CH_BTNS  5
#define MBOX_CH_TOUCH 6
#define MBOX_CH_PROP  8

#define MBOX_TAG_GETREVISION 0x00010002
#define MBOX_TAG_GETSERIAL   0x00010004
#define MBOX_TAG_LAST        0x00000000


uint32_t mbox_compose(uint8_t ch, uint32_t data);
void mbox_put(uint32_t mail);
uint32_t mbox_get(void);

#endif
