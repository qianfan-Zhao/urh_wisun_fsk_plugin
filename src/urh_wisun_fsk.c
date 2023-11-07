/*
 * The encoder/decoder for Wisun FSK PN9(defined in 802.15.4) algo
 * qianfan Zhao <qianfanguijin@163.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include "wisun_fsk_common.h"

#define URH_WIRUN_FSK_PLUGIN_VERSION		"1.0.2"

static int option_verbose = 0;
static int option_human = 0;
static int option_hexo = 0;

#if DEBUG > 0
static const char *str01_strstr_endp(const char *str01, const char **endp,
				     const char *needle)
{
	int match_length = strlen(needle);

	for (const char *from = str01; *from != '\0'; from++) {
		const char *start = NULL;
		int match_idx = 0;

		for (const char *s = from; *s != '\0'; s++) {
			if (*s == '-' || *s == ':')
				continue;

			if (*s == needle[match_idx]) {
				if (match_idx == 0)
					start = s;

				if (++match_idx == match_length) {
					if (endp) {
						s++;
						/* skip any spliter */
						while (*s != '\0') {
							if (*s == '-'
								|| *s == ':') {
								s++;
								continue;
							}
							break;
						}
						*endp = s;
					}
					return start;
				}
			} else {
				break;
			}
		}
	}

	return NULL;
}

static const char *str01_strstr(const char *str01, const char *needle)
{
	return str01_strstr_endp(str01, NULL, needle);
}

static int str01_strncmp_endp(const char *str01, const char *s2, size_t n,
			      const char **endp)
{
	const char *s = str01;
	size_t count = 0;

	while (count < n) {
		int diff;

		/* too short */
		if (*s == '\0')
			return -1;

		if (*s == '-' || *s == ':') {
			s++;
			continue;
		} else if (*s2 == '-' || *s2 == ':') {
			s2++;
			continue;
		}

		/* different length */
		if (*s2 == '\0')
			return -1;

		diff = *s - *s2;
		if (diff)
			return diff;

		s++;
		s2++;
		count++;
	}

	if (endp) {
		/* skip any spliter */
		while (*s != '\0') {
			if (*s == '-'
				|| *s == ':') {
				s++;
				continue;
			}
			break;
		}
		*endp = s;
	}

	return 0;
}

static int str01_strncmp(const char *str01, const char *s2, size_t n)
{
	return str01_strncmp_endp(str01, s2, n, NULL);
}
#else
static const char *str01_strstr_endp(const char *haystack, const char **endp,
				     const char *needle)
{
	const char *s = strstr(haystack, needle);

	if (s && endp)
		*endp = s + strlen(needle);

	return s;
}

static int str01_strncmp(const char *s1, const char *s2, size_t n)
{
	return strncmp(s1, s2, n);
}

static int str01_strncmp_endp(const char *s1, const char *s2, size_t n,
			      const char **endp)
{
	int ret = strncmp(s1, s2, n);

	if (ret == 0 && endp)
		*endp = s1 + n;

	return ret;
}
#endif

static size_t str01_to_buffer(const char *s, const char **endp, uint8_t *buf,
			      size_t bufsz, int lsb_first)
{
	size_t bytes = 0, binary_counts = 0;

	for (; *s != '\0' && bytes < bufsz; s++) {
		uint8_t bit = binary_counts % 8;
		int n = *s - '0';

	#if DEBUG > 0
		/* ignore the spliter in debug mode */
		if (*s == '-' || *s == ':')
			continue;
	#endif

		if (n != 0 && n != 1)
			break;

		if (lsb_first)
			buf[bytes] |= (n << bit);
		else
			buf[bytes] |= (n << (7 - bit));

		binary_counts++;
		if ((binary_counts % 8) == 0) {
			bytes++;
			if (bytes < bufsz)
				buf[bytes] = 0;
		}
	}

	if (endp)
		*endp = s;

	return binary_counts;
}

static size_t strict_str01_to_buffer(const char *str01, uint8_t *buf,
				     size_t bufsz, int lsbfirst)
{
	size_t binary_size;
	const char *endp;

	binary_size = str01_to_buffer(str01, &endp, buf, bufsz, lsbfirst);
	if (*endp != '\0') {
		fprintf(stderr, "input binary string is bad after:\n");
		fprintf(stderr, "%s\n", endp);
		return 0;
	}

	return binary_size;
}

static void print_hex_bytes(const uint8_t *buf, size_t byte_size)
{
	for (size_t i = 0; i < byte_size; i++)
		printf("%02x", buf[i]);
}

static uint16_t buffer_peek_u16_b1b0(const uint8_t *buf)
{
	return (buf[1] << 8) | buf[0];
}

static void print_bit(int b, size_t *bit_idx, size_t split_group_count)
{
	/* print binary string like this: 0000-0010-0011 */
	if (split_group_count) {
		if (*bit_idx != 0 && *bit_idx % split_group_count == 0)
			putchar('-');
		(*bit_idx)++;
	}

	printf("%d", !!b);
}

static void print_binary_bits_lsbfirst(const uint8_t *buf, size_t start_bit,
				       size_t end_bit,
				       size_t split_group_count)
{
	size_t start_byte = start_bit / 8, end_byte = (end_bit + 1) / 8;
	size_t idx = 0;

	if (start_bit % 8) {
		for (size_t i = start_bit % 8; i < 8; i++)
			print_bit(!!(buf[start_byte] & (1 << i)),
				  &idx,
				  split_group_count);
		start_byte++;
	}

	for (size_t byte = start_byte; byte < end_byte; byte++) {
		uint8_t b = buf[byte];

		/* should be lsb first */
		for (int bit = 0; bit < 8; bit++)
			print_bit((b >> bit) & 1, &idx, split_group_count);
	}

	if ((end_bit + 1) % 8) {
		for (size_t i = 0; i <= end_bit % 8; i++)
			print_bit(!!(buf[end_byte] & (1 << i)),
				  &idx,
				  split_group_count);
	}
}

static void print_binary_bits_msbfirst(const uint8_t *buf, size_t start_bit,
				       size_t end_bit,
				       size_t split_group_count)
{
	size_t start_byte = start_bit / 8, end_byte = (end_bit + 1) / 8;
	size_t idx = 0;

	if (start_bit % 8) {
		for (size_t i = start_bit % 8; i < 8; i++)
			print_bit(!!(buf[start_byte] & (1 << (7 - i))),
				  &idx,
				  split_group_count);
		start_byte++;
	}

	for (size_t byte = start_byte; byte < end_byte; byte++) {
		uint8_t b = buf[byte];

		/* should be msb first */
		for (int bit = 7; bit >= 0; bit--)
			print_bit((b >> bit) & 1, &idx, split_group_count);
	}

	if ((end_bit + 1) % 8) {
		for (size_t i = 0; i <= end_bit % 8; i++)
			print_bit(!!(buf[end_byte] & (1 << (7 - i))),
				  &idx,
				  split_group_count);
	}
}

static size_t roundup8(size_t n)
{
	return (n / 8) + ((n % 8) != 0);
}

static int wisun_fsk_pn9_encode_data_payload(const char *str01)
{
	size_t group_count = option_human ? 4 : 0;
	size_t binary_size, byte_size;
	uint8_t buf[16384] = { 0 };

	binary_size = strict_str01_to_buffer(str01, buf, sizeof(buf), 1);
	if (!binary_size)
		return -1;

	byte_size = roundup8(binary_size);
	pn9_payload_decode(buf, byte_size);

	if (option_hexo)
		print_hex_bytes(buf, byte_size);
	else
		print_binary_bits_lsbfirst(buf, 0, binary_size - 1, group_count);
	printf("\n");
	return 0;
}

static void foreach_bit_in_buffer_msbfirst(const uint8_t *buf, size_t bit_size,
			void (*f)(int b, size_t bit_idx, void *private_data),
			void *private_data)
{
	const uint8_t *p_buffer = buf;
	size_t bit_idx = 0;

	for (size_t i = 0; i < bit_size; i++) {
		int b = (((*p_buffer) >> (7 - bit_idx))) & 1;

		f(b, i, private_data);

		if (++bit_idx == 8) {
			p_buffer++;
			bit_idx = 0;
		}
	}
}

static void foreach_bit_in_buffer_lsbfirst(const uint8_t *buf, size_t bit_size,
			void (*f)(int b, size_t bit_idx, void *private_data),
			void *private_data)
{
	const uint8_t *p_buffer = buf;
	size_t bit_idx = 0;

	for (size_t i = 0; i < bit_size; i++) {
		int b = (((*p_buffer) >> bit_idx)) & 1;

		f(b, i, private_data);

		if (++bit_idx == 8) {
			p_buffer++;
			bit_idx = 0;
		}
	}
}

struct wisun_2fsk_fec_encoder {
	uint8_t		m;

	/* encode buffers */
	uint8_t		*buf;
	size_t		bufsz;
	size_t		encode_bits;
	size_t		byte_idx;
	uint8_t		bit_idx;
};

/* push 2bit in lsb first mode */
static int wisun_2fsk_fec_encoder_push_2bits(struct wisun_2fsk_fec_encoder *arg,
					     uint8_t u)
{
	static const uint8_t reverse2_tables[] = {
		[0b00] = 0b00,
		[0b01] = 0b10,
		[0b10] = 0b01,
		[0b11] = 0b11,
	};

	if (arg->byte_idx < arg->bufsz) {
		u = reverse2_tables[u & 0b11];

		if (arg->bit_idx == 0)
			arg->buf[arg->byte_idx] = 0;

		arg->buf[arg->byte_idx] |= (u << arg->bit_idx);
		arg->bit_idx += 2;

		if (arg->bit_idx == 8) {
			arg->byte_idx++;
			arg->bit_idx = 0;
		}

		arg->encode_bits += 2;
		return 0;
	}

	return -1;
}

static void wisun_2fsk_nrnsc_input_bit(int b, size_t bit_idx, void *private_data)
{
	struct wisun_2fsk_fec_encoder *arg = private_data;
	uint8_t u;

	u = nrnsc_input_bit(&arg->m, b);
	wisun_2fsk_fec_encoder_push_2bits(arg, u);
}

static void wisun_2fsk_rsc_input_bit(int b, size_t bit_idx, void *private_data)
{
	struct wisun_2fsk_fec_encoder *arg = private_data;
	uint8_t u;

	u = rsc_input_bit(&arg->m, b);
	wisun_2fsk_fec_encoder_push_2bits(arg, u);
}

static int wisun_fsk_encode_nrnsc(const char *str01)
{
	struct wisun_2fsk_fec_encoder arg = { .m = NRNSC_INIT_M };
	uint8_t buf[4096] = { 0 }, encode_buf[sizeof(buf) * 2];
	size_t group_count = option_human ? 4 : 0;
	size_t binary_size;

	arg.buf = encode_buf;
	arg.bufsz = sizeof(encode_buf);

	binary_size = strict_str01_to_buffer(str01, buf, sizeof(buf), 1);
	if (!binary_size)
		return -1;

	foreach_bit_in_buffer_lsbfirst(buf, binary_size,
				       wisun_2fsk_nrnsc_input_bit, &arg);

	if (option_hexo)
		print_hex_bytes(encode_buf, roundup8(arg.encode_bits));
	else
		print_binary_bits_lsbfirst(encode_buf, 0, arg.encode_bits - 1,
					   group_count);
	printf("\n");

	return 0;
}

static int wisun_fsk_encode_rsc(const char *str01)
{
	struct wisun_2fsk_fec_encoder arg = { .m = RSC_INIT_M };
	uint8_t buf[4096] = { 0 }, encode_buf[sizeof(buf) * 2];
	size_t group_count = option_human ? 4 : 0;
	size_t binary_size;

	arg.buf = encode_buf;
	arg.bufsz = sizeof(encode_buf);

	binary_size = strict_str01_to_buffer(str01, buf, sizeof(buf), 1);
	if (!binary_size)
		return -1;

	foreach_bit_in_buffer_lsbfirst(buf, binary_size,
				       wisun_2fsk_rsc_input_bit, &arg);

	if (option_hexo)
		print_hex_bytes(encode_buf, roundup8(arg.encode_bits));
	else
		print_binary_bits_lsbfirst(encode_buf, 0,
					   arg.encode_bits - 1,
					   group_count);
	printf("\n");

	if (option_verbose > 0) {
		/* m.bit2 -> M0,
		 * m.bit1 -> M1,
		 * m.bit0 -> M2
		 */
		printf("Memory state(M0-M2): %d%d%d\n",
			(arg.m >> 2) & 1,
			(arg.m >> 1) & 1,
			(arg.m >> 0) & 1);
	}

	return 0;
}

static int wisun_fsk_interleaving(const char *str01)
{
	size_t group_count = option_human ? 4 : 0;
	uint8_t buf[4096] = { 0 };
	size_t binary_size;

	binary_size = strict_str01_to_buffer(str01, buf, sizeof(buf), 1);
	if (!binary_size)
		return -1;

	/* 2-bit u1u0 as one symbol, 16 symbol as one block */
	if (binary_size % 32) {
		fprintf(stderr, "the input is not block group data\n");
		return -1;
	}

	interleaving_bits(buf, binary_size, buf);

	if (option_hexo)
		print_hex_bytes(buf, binary_size / 8);
	else
		print_binary_bits_lsbfirst(buf, 0, binary_size - 1,
					   group_count);
	putchar('\n');

	return 0;
}

/* Wisun 2-FSK packet format:
 * SHR: PREAMBLE * n + SFD
 * PHY Header
 * PN9 payload
 */
#define WISUN_2FSK_PREAMBLE	"01010101"

enum wisun_2fsk_sfd_type {
	WISUN_2FSK_SFD_CODED0,		/* phySunFskSfd = 0, coded   format */
	WISUN_2FSK_SFD_UNCODED0,	/* phySunFskSfd = 0, uncoded format */
	WISUN_2FSK_SFD_CODED1,		/* phySunFskSfd = 1, coded   format */
	WISUN_2FSK_SFD_UNCODED1,	/* phySunFskSfd = 1, uncoded format */
	WISUN_2FSK_SFD_MAX,
};

static const char *wisun_2fsk_phy_sfd_binary_streams[WISUN_2FSK_SFD_MAX] = {
	/* b0-b15 */
	[WISUN_2FSK_SFD_CODED0]   = "0110111101001110",
	[WISUN_2FSK_SFD_UNCODED0] = "1001000001001110",
	[WISUN_2FSK_SFD_CODED1]   = "0110001100101101",
	[WISUN_2FSK_SFD_UNCODED1] = "0111101000001110",
};

/* find a vaild SHR(including preamble and sfd) from the 01 binary streams.
 * Return the preamble index in 01 binary streams.
 * -1 if not found.
 */
static int _wisun_2fsk_str01_find_shr(const char *str01, size_t *ret_preamble_sz,
				      enum wisun_2fsk_sfd_type *ret_sfd_type,
				      const char **next)
{
	const char *p, *endp, *preamble;
	enum wisun_2fsk_sfd_type type = WISUN_2FSK_SFD_MAX;
	size_t len_sfd = strlen(wisun_2fsk_phy_sfd_binary_streams[0]);
	size_t preamble_sz = 0;

	preamble = str01_strstr_endp(str01, &endp, WISUN_2FSK_PREAMBLE);
	if (!preamble) {
		if (next)
			*next = NULL;
		return -1;
	}

	p = preamble;
	if (next)
		*next = p + 1;

	do {
		preamble_sz += strlen(WISUN_2FSK_PREAMBLE);
		p = endp;
	} while (!str01_strncmp_endp(p, WISUN_2FSK_PREAMBLE,
				     strlen(WISUN_2FSK_PREAMBLE),
				     &endp));

	if (ret_preamble_sz)
		*ret_preamble_sz = preamble_sz;

	for (enum wisun_2fsk_sfd_type t = 0; t < WISUN_2FSK_SFD_MAX; t++) {
		if (!str01_strncmp(p, wisun_2fsk_phy_sfd_binary_streams[t],
				   len_sfd)) {
			type = t;
			p += len_sfd;
			break;
		}
	}

	if (type == WISUN_2FSK_SFD_MAX)
		return -1;

	if (ret_sfd_type)
		*ret_sfd_type = type;

	return preamble - str01;
}

static int wisun_2fsk_str01_find_shr(const char *str01, size_t *ret_preamble_sz,
				     enum wisun_2fsk_sfd_type *ret_sfd_type)
{
	const char *p = str01;

	while (p && *p != '\0') {
		const char *next;
		int idx;

		idx = _wisun_2fsk_str01_find_shr(p, ret_preamble_sz,
						 ret_sfd_type,
						 &next);
		if (!(idx < 0))
			return p - str01 + idx;

		p = next;
	}

	return -1;
}

#define WISUN_2FSK_PHR_MODE_SWITCH	(1 << 0)
#define WISUN_2FSK_PHR_FCS_TYPE_CRC16	(1 << 3)
#define WISUN_2FSK_PHR_DATA_WHITENING	(1 << 4)

static uint16_t wisun_2fsk_fix_phr_order(uint16_t phr)
{
	uint16_t phr_msbfirst = reverse16(phr);

	/* phr field format
	 * bit0:    mode switch
	 * bit3:    fcs type, 0 = CRC32, 1 = CRC16
	 * bit4:    data whitening
	 * bit5-15: frame length, transmitted MSB first.
	 */

	return (phr_msbfirst << 5) | (phr & 0x1f);
}

static int wisun_2fsk_packet_recovery(uint8_t *p_sfd, size_t binary_size,
				      int skip_verify,
				      size_t *ret_frame_length)
{
	uint16_t phr, frame_length;
	uint8_t *p = p_sfd;

	if (binary_size <= 32) /* bit length of SFD + PHR */
		return -1;

	/* skip sfd */
	binary_size -= 16;
	p += 2;

	phr = wisun_2fsk_fix_phr_order(buffer_peek_u16_b1b0(p));
	binary_size -= 16;
	p += 2;

	frame_length = phr >> 5;
	if (binary_size < frame_length * 8)
		return -1;

	if (phr & WISUN_2FSK_PHR_DATA_WHITENING)
		pn9_payload_decode(p, frame_length);

	if (!skip_verify) {
		bool good = false;

		if (!(phr & WISUN_2FSK_PHR_FCS_TYPE_CRC16))
			good = ieee_802154_fcs32_buf_is_good(p, frame_length);
		else
			fprintf(stderr, "warnning: FCS16 is not supported\n");

		if (!good)
			return -1;
	}

	*ret_frame_length = frame_length;
	return 0;
}


static void wisun_2fsk_print_packet(const uint8_t *buf, size_t binary_size,
				    size_t preamble_sz,
				    enum wisun_2fsk_sfd_type type,
				    size_t frame_length)
{
	size_t byte_size;

	byte_size = binary_size / 8;
	if (binary_size % 8)
		byte_size++;

	if (option_hexo) {
		if (option_human) {
			const uint8_t *p = buf;
			size_t crc_sz = 4;
			uint16_t phr;

			print_hex_bytes(p, preamble_sz / 8);
			p += preamble_sz / 8;
			printf("-");

			printf("%04x-", buffer_peek_u16_b1b0(p)); /* sfd */
			p += 2;

			phr = buffer_peek_u16_b1b0(p);
			printf("%04x-", phr);
			p += 2;

			if (frame_length < 2)
				goto print_remain;

			if (phr & WISUN_2FSK_PHR_FCS_TYPE_CRC16) {
				if (frame_length <= 2)
					goto print_remain;
				crc_sz = 2;
			} else if (frame_length <= 4) {
				goto print_remain;
			}

			print_hex_bytes(p, frame_length - crc_sz);
			p += (frame_length - crc_sz);
			frame_length = crc_sz;
			printf("-");

		print_remain:
			print_hex_bytes(p, frame_length);
		} else {
			print_hex_bytes(buf, byte_size);
		}
	} else {
		size_t group_sz = option_human ? 4 : 0;

		print_binary_bits_lsbfirst(buf, 0, binary_size - 1, group_sz);
	}

	printf("\n");
}

static int wisun_2fsk_packet_decode(const char *str01, int skip_verify)
{
	size_t preamble_sz, binary_size, byte_size, frame_len;
	enum wisun_2fsk_sfd_type type;
	uint8_t buf[8192] = { 0 };
	const char *endp;
	int idx, ret;

	idx = wisun_2fsk_str01_find_shr(str01, &preamble_sz, &type);
	if (idx < 0) {
		fprintf(stderr, "2-FSK SHR is not found\n");
		return idx;
	}

	binary_size = str01_to_buffer(str01 + idx, &endp, buf, sizeof(buf), 1);
	if (*endp != '\0') {
		fprintf(stderr, "input binary string is bad after:\n");
		fprintf(stderr, "%s\n", endp);
		return -1;
	}

	byte_size = binary_size / 8;
	if (binary_size % 8)
		byte_size++;

	/* frame_len: total number of octets contained in the PSDU.
	 *            including PHY payload and CRC.
	 *            doesn't include PHR.
	 */
	ret = wisun_2fsk_packet_recovery(buf + preamble_sz / 8,
					 binary_size - preamble_sz,
					 skip_verify,
					 &frame_len);
	if (ret < 0) {
		fprintf(stderr, "verify 802.15.4 packet failed\n");
		return ret;
	}

	wisun_2fsk_print_packet(buf, binary_size, preamble_sz, type, frame_len);
	return 0;
}

enum {
	OPTION_PACKET,
	OPTION_PN9,
	OPTION_RSC,
	OPTION_NRNSC,
	OPTION_INTERLEAVING,
	OPTION_HELP,
	OPTION_VERBOSE,
	OPTION_VERSION,
	OPTION_DECODE,
	OPTION_ENCODE,
	OPTION_HEXO,
	OPTION_HUMAN,
	OPTION_SKIP_VERIFY,
};

static struct option long_options[] = {
	/* name			has_arg,		*flag,		val */
	{ "packet",		no_argument,		NULL,		OPTION_PACKET	},
	{ "pn9",		no_argument,		NULL,		OPTION_PN9	},
	{ "rsc",		no_argument,		NULL,		OPTION_RSC	},
	{ "nrnsc",		no_argument,		NULL,		OPTION_NRNSC	},
	{ "interleaving",	no_argument,		NULL,		OPTION_INTERLEAVING	},
	{ "help",		no_argument,		NULL,		OPTION_HELP	},
	{ "verbose",		no_argument,		NULL,		OPTION_VERBOSE	},
	{ "version",		no_argument,		NULL,		OPTION_VERSION	},
	{ "decode",		no_argument,		NULL,		OPTION_DECODE	},
	{ "encode",		no_argument,		NULL,		OPTION_ENCODE	},
	{ "hexo",		no_argument,		NULL,		OPTION_HEXO	},
	{ "human",		no_argument,		NULL,		OPTION_HUMAN	},
	{ "skip-verify",	no_argument,		NULL,		OPTION_SKIP_VERIFY	},
	{ NULL,			0,			NULL,		0   },
};

static void print_usage(void)
{
	fprintf(stderr, "Usage: urh_wisun_fsk [OPTIONS] \"binary-strings\"\n");
	fprintf(stderr, "-h --help:              show this message\n");
	fprintf(stderr, "-V --version:           show plugin version\n");
	fprintf(stderr, "-v --verbose:           verbose mode\n");
	fprintf(stderr, "-d --decode:            decode a wisun 2-fsk packet\n");
	fprintf(stderr, "-e --encode:            encode a wisun 2-fsk packet\n");
	fprintf(stderr, "   --hexo:              print the encode/decode result in hex mode\n");
	fprintf(stderr, "   --human:             print the output string in human format\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Available algo for encode/decode:\n");
	fprintf(stderr, "   --packet:            encode/decode the binary string as a full packet.\n");
	fprintf(stderr, "                        (the default algo)\n");
	fprintf(stderr, "   --pn9:               encode/decode binary strings by PN9\n");
	fprintf(stderr, "   --nrnsc:             encode binary strings by NRNSC encoder\n");
	fprintf(stderr, "   --rsc                encode binary string by RSC encoder\n");
	fprintf(stderr, "   --interleaving:      interleaving the input binary blocks\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options for encode/decode packet(--packet):\n");
	fprintf(stderr, "   --skip-verify:       do not verify 802.15.4 packet\n");
}

enum {
	ALGO_PACKET,
	ALGO_PN9,
	ALGO_NRNSC,
	ALGO_RSC,
	ALGO_INTERLEAVING,
};

#if DEBUG > 0
static void test_str01_strstr(void)
{
	const char *base = "0-0-1-0-1-0-1";
	const char *s;

	assert(str01_strstr("0-0-1-0-1", "0101") != NULL);

	s = str01_strstr(base, "0101");
	assert(s != NULL);
	assert(s - base == 2);
	assert(str01_strncmp(s, "0101", 4) == 0);
	assert(str01_strncmp(s, "0-1-0-1", 4) == 0);
}

static void test_wisun_2fsk_str01_find_shr(void)
{
	const char *base;
	enum wisun_2fsk_sfd_type type;
	size_t preamble_sz;
	int idx;

	type = WISUN_2FSK_SFD_MAX;
	base = "01010101010101010101010101010101" "1001000001001110";
	idx = wisun_2fsk_str01_find_shr(base, &preamble_sz, &type);
	assert(idx == 0);
	assert(preamble_sz == 32);
	assert(type == WISUN_2FSK_SFD_UNCODED0);

	type = WISUN_2FSK_SFD_MAX;
	base = "0101-0101-0101-0101-0101-0101-0101-0101" "-:-:-"
	       "1001-0000-0100-1110";
	idx = wisun_2fsk_str01_find_shr(base, &preamble_sz, &type);
	assert(idx == 0);
	assert(preamble_sz == 32);
	assert(type == WISUN_2FSK_SFD_UNCODED0);
}

static void test_rsc_input_bit(void)
{
	struct wisun_2fsk_fec_encoder arg = { .m = RSC_INIT_M };
	uint8_t buf[2] = { 0x11, 0x22 }, encode_buf[sizeof(buf) * 2];
	const uint8_t expected[sizeof(encode_buf)] = { 0x17, 0x03, 0x5c, 0x0c };

	memset(encode_buf, 0, sizeof(encode_buf));
	memset(&arg, 0, sizeof(arg));
	arg.buf = encode_buf;
	arg.bufsz = sizeof(encode_buf);

	foreach_bit_in_buffer_lsbfirst(buf, sizeof(buf) * 8,
				       wisun_2fsk_rsc_input_bit, &arg);
	assert(memcmp(encode_buf, expected, sizeof(expected)) == 0);

	memset(encode_buf, 0, sizeof(encode_buf));
	memset(&arg, 0, sizeof(arg));
	arg.buf = encode_buf;
	arg.bufsz = sizeof(encode_buf);

	foreach_bit_in_buffer_lsbfirst(&buf[0], 8,
				       wisun_2fsk_rsc_input_bit, &arg);
	foreach_bit_in_buffer_lsbfirst(&buf[1], 8,
				       wisun_2fsk_rsc_input_bit, &arg);
	assert(memcmp(encode_buf, expected, sizeof(expected)) == 0);
}

static void test_nrnsc_input_bit(void)
{
	struct wisun_2fsk_fec_encoder arg = { .m = RSC_INIT_M };
	uint8_t buf[2] = { 0x11, 0x22 }, encode_buf[sizeof(buf) * 2];
	const uint8_t expected[sizeof(encode_buf)] = { 0x04, 0x04, 0x13, 0x10 };

	memset(encode_buf, 0, sizeof(encode_buf));
	memset(&arg, 0, sizeof(arg));
	arg.buf = encode_buf;
	arg.bufsz = sizeof(encode_buf);

	foreach_bit_in_buffer_lsbfirst(buf, sizeof(buf) * 8,
				       wisun_2fsk_nrnsc_input_bit, &arg);
	assert(memcmp(encode_buf, expected, sizeof(expected)) == 0);

	memset(encode_buf, 0, sizeof(encode_buf));
	memset(&arg, 0, sizeof(arg));
	arg.buf = encode_buf;
	arg.bufsz = sizeof(encode_buf);

	foreach_bit_in_buffer_lsbfirst(&buf[0], 8,
				       wisun_2fsk_nrnsc_input_bit, &arg);
	foreach_bit_in_buffer_lsbfirst(&buf[1], 8,
				       wisun_2fsk_nrnsc_input_bit, &arg);
	assert(memcmp(encode_buf, expected, sizeof(expected)) == 0);
}

static void self_test(void)
{
	test_str01_strstr();
	test_wisun_2fsk_str01_find_shr();
	test_rsc_input_bit();
	test_nrnsc_input_bit();
}
#endif

int main(int argc, char **argv)
{
	int algo = ALGO_PACKET, decode = 1, skip_verify = 0;
	int ret = -1;

#if DEBUG > 0
	self_test();
#endif

	while (1) {
		int option_index = 0;
		int c;

		c = getopt_long(argc, argv, "hvVde", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case OPTION_PACKET:
			algo = ALGO_PACKET;
			break;
		case OPTION_PN9:
			algo = ALGO_PN9;
			break;
		case OPTION_NRNSC:
			algo = ALGO_NRNSC;
			break;
		case OPTION_RSC:
			algo = ALGO_RSC;
			break;
		case OPTION_INTERLEAVING:
			algo = ALGO_INTERLEAVING;
			break;

		case 'v':
		case OPTION_VERBOSE:
			option_verbose++;
			break;
		case 'h':
		case OPTION_HELP:
			print_usage();
			return 0;
		case 'V':
		case OPTION_VERSION:
			printf("version: %s\n", URH_WIRUN_FSK_PLUGIN_VERSION);
			return 0;
		case OPTION_DECODE:
		case OPTION_ENCODE:
			decode = c == OPTION_DECODE;
			break;
		case 'd':
		case 'e':
			decode = c == 'd';
			break;
		case OPTION_HEXO: /* hex output */
			option_hexo = 1;
			break;
		case OPTION_HUMAN:
			option_human = 1;
			break;
		case OPTION_SKIP_VERIFY:
			skip_verify = 1;
			break;
		}
	}

	if (!(optind < argc)) {
		print_usage();
		return -1;
	}

	switch (algo) {
	default: /* ALGO_PACKET */
		if (decode)
			ret = wisun_2fsk_packet_decode(argv[optind],
						       skip_verify);
		break;
	case ALGO_PN9:
		ret = wisun_fsk_pn9_encode_data_payload(argv[optind]);
		break;
	case ALGO_NRNSC:
		ret = wisun_fsk_encode_nrnsc(argv[optind]);
		break;
	case ALGO_RSC:
		ret = wisun_fsk_encode_rsc(argv[optind]);
		break;
	case ALGO_INTERLEAVING:
		ret = wisun_fsk_interleaving(argv[optind]);
		break;
	}

	return ret;
}
