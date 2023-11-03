/*
 * common wisun fsk helper functions
 * qianfan Zhao <qianfanguijin@163.com>
 */
#ifndef WISUN_FSK_COMMON_H
#define WISUN_FSK_COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define ARRAY_SIZE(a)			(sizeof(a) / sizeof(a[0]))

uint8_t reverse8(uint8_t x);
uint16_t reverse16(uint16_t x);
uint32_t reverse32(uint32_t x);

void pn9_payload_decode(uint8_t *buf, size_t byte_size);

#define RSC_INIT_M	0
#define NRNSC_INIT_M	0

uint8_t rsc_input_bit(uint8_t *m, int bi);
uint8_t nrnsc_input_bit(uint8_t *m, int bi);

void interleaving_bits(const uint8_t *buf, size_t binary_bits, uint8_t *out);

uint16_t ieee_802154_fcs16(uint16_t crc, const uint8_t *buf, size_t sz);

#define IEEE_802154_FCS32_INIT		0xffffffff
#define IEEE_802154_FCS32_GOOD		0x2144df1c
uint32_t ieee_802154_fcs32(uint32_t crc, const uint8_t *buf, size_t sz);
bool ieee_802154_fcs32_buf_is_good(const uint8_t *buf, size_t len);

#endif
