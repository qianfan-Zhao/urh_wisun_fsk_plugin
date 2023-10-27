/*
 * common helper functions for wisun fsk
 * qianfan Zhao <qianfanguijin@163.com>
 */
#include <stdio.h>
#include <stdint.h>

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
