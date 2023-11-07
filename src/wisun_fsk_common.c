/*
 * common helper functions for wisun fsk
 * qianfan Zhao <qianfanguijin@163.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "wisun_fsk_common.h"

uint8_t reverse8(uint8_t x)
{
	x = (((x & 0xaa) >> 1) | ((x & 0x55) << 1));
	x = (((x & 0xcc) >> 2) | ((x & 0x33) << 2));
	x = (((x & 0xf0) >> 4) | ((x & 0x0f) << 4));

	return x;
}

uint16_t reverse16(uint16_t x)
{
	x = (((x & 0xaaaa) >> 1) | ((x & 0x5555) << 1));
	x = (((x & 0xcccc) >> 2) | ((x & 0x3333) << 2));
	x = (((x & 0xf0f0) >> 4) | ((x & 0x0f0f) << 4));
	x = (((x & 0xff00) >> 8) | ((x & 0x00ff) << 8));

	return x;
}

uint32_t reverse32(uint32_t x)
{
	x = ((x & 0x55555555) <<  1) | ((x >>  1) & 0x55555555);
	x = ((x & 0x33333333) <<  2) | ((x >>  2) & 0x33333333);
	x = ((x & 0x0F0F0F0F) <<  4) | ((x >>  4) & 0x0F0F0F0F);
	x = (x << 24) | ((x & 0xFF00) << 8) |
		((x >> 8) & 0xFF00) | (x >> 24);

	return x;
}

static uint8_t pn9_tables[511] = { 0 };

static uint16_t pn9_shift1(uint16_t pn9, unsigned int *xor_out)
{
	unsigned int bit0 = (pn9 >> 0) & 1, bit5 = (pn9 >> 5) & 1;
	unsigned int xor;

	pn9 = pn9 >> 1;
	xor = bit0 ^ bit5;
	pn9 |= (xor << 8);

	if (xor_out)
		*xor_out = xor;

	return pn9;
}

static void pn9_table_init(void)
{
	uint16_t pn9 = 0x1ff;

	for (size_t i = 0; i < sizeof(pn9_tables); i++) {
		unsigned int xor_out;
		uint8_t n = 0;

		for (size_t bit = 0; bit < 8; bit++) {
			pn9 = pn9_shift1(pn9, &xor_out);
			n |= (xor_out << bit);
		}

		pn9_tables[i] = n;
	}
}

void pn9_payload_decode(uint8_t *buf, size_t byte_size)
{
	static int pn9_table_inited = 0;

	if (!pn9_table_inited) {
		pn9_table_inited = 1;
		pn9_table_init();
	}

	for (size_t i = 0; i < byte_size; i++)
		buf[i] ^= pn9_tables[i % sizeof(pn9_tables)];
}

/* RSC encoder for wisun fsk, defined in <802.15.4-2020.pdf> */
static uint8_t xor_bit0_bit1_bit2(uint8_t b)
{
	const uint8_t xor_result[8] = {
		[0b000] = 0,
		[0b001] = 1,
		[0b010] = 1,
		[0b011] = 0,
		[0b100] = 1,
		[0b101] = 0,
		[0b110] = 0,
		[0b111] = 1,
	};

	return xor_result[b & 0b111];
}

/* software based rsc input bit algo */
static uint8_t sw_rsc_input_bit(uint8_t *m, int bi)
{
	/* bi = bit3, bi-1 = bit2, bi-2 = bit1, bi-3 = bit0 */
	uint8_t u1, u0, new_bi1, bi2, bi3;
	uint8_t last = xor_bit0_bit1_bit2(*m);

	u0 = bi;
	new_bi1 = bi ^ last;

	bi2 = (*m >> 1) & 1;
	bi3 = (*m >> 0) & 1;
	u1 = new_bi1 ^ bi2 ^ bi3;

	*m = (*m >> 1) & 0b011;
	*m = *m | (new_bi1 << 2);

	return (u1 << 1) | u0;
}

struct rsc_table {
	uint8_t		m;
	uint8_t		next_m;
	uint8_t		u1u0;
};

uint8_t rsc_input_bit(uint8_t *m, int bi)
{
	static struct rsc_table rsc_tables_zero[8], rsc_tables_one[8];
	static int rsc_table_inited = 0;
	struct rsc_table *table;
	uint8_t u;

	if (!rsc_table_inited) {
		rsc_table_inited = 1;
		for (uint8_t i = 0; i < 8; i++) {
			rsc_tables_zero[i].m = rsc_tables_zero[i].next_m = i;
			rsc_tables_one[i].m = rsc_tables_one[i].next_m = i;

			rsc_tables_zero[i].u1u0 =
				sw_rsc_input_bit(&rsc_tables_zero[i].next_m, 0);
			rsc_tables_one[i].u1u0 =
				sw_rsc_input_bit(&rsc_tables_one[i].next_m, 1);
		}
	}

	if (bi)
		table = &rsc_tables_one[*m & 0b111];
	else
		table = &rsc_tables_zero[*m & 0b111];

	u = table->u1u0;
	*m = table->next_m;

	return u;
}

/* NRNSC encode for wisun fsk, defined in <802.15.4-2020.pdf> */
static uint8_t nrnsc_tables[16] = { 0 };

static void nrnsc_table_init(void)
{
	/* bi = bit3, bi1 = bit2 ... bi3 = bit0
	 *
	 * u0 = bi ^ bi1 ^ bi2 ^ bi3 ^ 1
	 * u1 = bi ^ bi2 ^ bi3 ^ 1
	 * table[i] = (u1 << 1) | u0
	 */
	for (size_t i = 0; i < sizeof(nrnsc_tables); i++) {
		uint8_t bi, bi1, bi2, bi3;
		uint8_t u1, u0;

		bi =  (i >> 3) & 1;
		bi1 = (i >> 2) & 1;
		bi2 = (i >> 1) & 1;
		bi3 = (i >> 0) & 1;

		u0 = bi ^ bi1 ^ bi2 ^ bi3 ^ 1;
		u1 = bi ^ bi2 ^ bi3 ^ 1;

		nrnsc_tables[i] = (u1 << 1) | u0;
	}
}

uint8_t nrnsc_input_bit(uint8_t *m, int bi)
{
	static int nrnsc_table_inited = 0;

	if (!nrnsc_table_inited) {
		nrnsc_table_inited = 1;
		nrnsc_table_init();
	}

	*m = (((*m) >> 1) | (bi << 3)) & 0x0f;
	return nrnsc_tables[*m];
}

/*
 * based on <802.15.4-2020.pdf>:
 *
 * $ python3
 * >>> for k in range(16):
 * ...     print(15 - 4 * (k % 4) - k // 4)
 * ...
 *
 * Example:
 * symbol idx order: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
 * raw symbols:      aa bb cc dd ee ff gg hh ii jj kk ll mm nn oo pp
 * result:           pp ll hh dd oo kk gg cc nn jj ff bb mm ii ee aa
 */
static const uint8_t interleaving_symbol_target[16] = {
	15,
	11,
	7,
	3,
	14,
	10,
	6,
	2,
	13,
	9,
	5,
	1,
	12,
	8,
	4,
	0,
};

void interleaving_bits(const uint8_t *buf, size_t binary_bits, uint8_t *out)
{
	const uint8_t *p_buf = buf;
	uint8_t *p_out = out;

	/* 2 bit = 1 symbol
	 * 16 symbol = 1 block
	 * -> 1block = 32bit
	 */
	for (size_t bit = 0; bit < binary_bits / 32 * 32; bit += 32) {
		uint32_t in = 0, out = 0;

		in = (p_buf[0] << 24) | (p_buf[1] << 16)
			| (p_buf[2] << 8) | p_buf[3];

		for (size_t symbol_idx = 0; symbol_idx < 16; symbol_idx++) {
			uint8_t symbol = (in >> 30) & 0b11;
			uint8_t target_idx;

			in <<= 2; /* shift out the MSB */
			target_idx = interleaving_symbol_target[symbol_idx];
			out |= (symbol << (target_idx * 2));
		}

		p_out[0] = (out >> 24) & 0xff;
		p_out[1] = (out >> 16) & 0xff;
		p_out[2] = (out >>  8) & 0xff;
		p_out[3] = (out >>  0) & 0xff;

		p_buf += 4;
		p_out += 4;
	}
}

static const uint32_t crc32_tables[] = {
	0x00000000,0x77073096,0xee0e612c,0x990951ba,0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
	0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988,0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
	0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
	0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
	0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
	0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
	0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
	0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
	0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
	0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
	0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
	0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
	0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
	0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
	0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
	0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
	0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
	0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8,0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
	0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
	0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
	0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
	0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
	0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
	0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
	0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
	0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
	0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
	0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
	0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
	0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
	0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
	0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d
};

static inline uint32_t crc32_byte(uint32_t next, uint8_t data)
{
	return (next >> 8) ^ crc32_tables[(next ^ data) & 0xff];
}

uint32_t ieee_802154_fcs32(uint32_t crc, const uint8_t *buf, size_t len)
{
	/* Uppon transmission, if the length of the calculation field is less
	 * than 4 octets, the FCS computation shall assume padding the
	 * calculation field length exactly 4 octets; howerer, these pad bits
	 * shall not be transmitted.
	 */
	for (uint32_t i = 0; i < len; i++)
		crc = crc32_byte(crc, buf[i]);

	if (len < 4) {
		for (size_t i = 0; i < 4 - len; i++)
			crc = crc32_byte(crc, 0);
	}

	return crc ^ 0xffffffff;
}

bool ieee_802154_fcs32_buf_is_good(const uint8_t *buf, size_t len)
{
	uint32_t crc = IEEE_802154_FCS32_INIT;

	if (len <= sizeof(crc))
		return false;

	if (len < 8) { /* pading */
		size_t payload_sz = len - sizeof(crc);
		uint8_t padding[8] = { 0 };

		memcpy(padding, buf, payload_sz);
		memcpy(&padding[sizeof(crc)], &buf[payload_sz], sizeof(crc));

		crc = ieee_802154_fcs32(crc, padding, sizeof(padding));
	} else {
		crc = ieee_802154_fcs32(crc, buf, len);
	}

	return crc == IEEE_802154_FCS32_GOOD;
}
