/*
 * common wisun fsk helper functions
 * qianfan Zhao <qianfanguijin@163.com>
 */
#ifndef WISUN_FSK_COMMON_H
#define WISUN_FSK_COMMON_H

#include <stdio.h>
#include <stdint.h>

void pn9_payload_decode(uint8_t *buf, size_t byte_size);

#define RSC_INIT_M	0
#define NRNSC_INIT_M	0

uint8_t rsc_input_bit(uint8_t *m, int bi);
uint8_t nrnsc_input_bit(uint8_t *m, int bi);

#endif
