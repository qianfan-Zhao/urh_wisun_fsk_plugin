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

struct bufwrite {
	uint8_t	*buf;
	size_t	len;
	size_t	size;
	int	err;
};

void bufwrite_init(struct bufwrite *b, uint8_t *buf, size_t bufsize);

uint8_t *bufwrite_push_le8(struct bufwrite *b, uint8_t u8);
uint8_t *bufwrite_push_le16(struct bufwrite *b, uint16_t le16);
uint8_t *bufwrite_push_le32(struct bufwrite *b, uint32_t le32);
uint8_t *bufwrite_push_data(struct bufwrite *b, const uint8_t *data, size_t sz);

uint8_t reverse8(uint8_t x);
uint16_t reverse16(uint16_t x);
uint32_t reverse32(uint32_t x);

void pn9_payload_decode(uint8_t *buf, size_t byte_size);

#define RSC_INIT_M	0
#define NRNSC_INIT_M	0

uint8_t rsc_input_bit(uint8_t *m, int bi);
uint8_t nrnsc_input_bit(uint8_t *m, int bi);

int rsc_decode(uint8_t *m, uint8_t *encode_buf, size_t encode_bits,
	       uint8_t *out_buf, size_t out_buf_sz, size_t *ret_decode_bits);
int nrnsc_decode(uint8_t *m, uint8_t *encode_buf, size_t encode_bits,
		 uint8_t *out_buf, size_t out_buf_sz, size_t *ret_decode_bits);

void interleaving_bits(const uint8_t *buf, size_t binary_bits, uint8_t *out);

uint16_t ieee_802154_fcs16(uint16_t crc, const uint8_t *buf, size_t sz);

#define IEEE_802154_FCS32_INIT		0xffffffff
#define IEEE_802154_FCS32_GOOD		0x2144df1c
uint32_t ieee_802154_fcs32(uint32_t crc, const uint8_t *buf, size_t sz);
bool ieee_802154_fcs32_buf_is_good(const uint8_t *buf, size_t len);

#endif
