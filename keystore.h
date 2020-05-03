
#ifndef KEYSTORE_H
#define KEYSTORE_H

#include "dp3t.h"

#ifdef __cplusplus
extern "C"
{
#endif

uint8_t *dp3t_get_skt_0(void);
beacons_t *dp3t_generate_beacons(sk_t key, int day);
uint8_t *dp3t_get_ephid(uint8_t idx);
#ifdef __cplusplus
}
#endif

#endif