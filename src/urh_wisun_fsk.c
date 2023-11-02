/*
 * The encoder/decoder for Wisun FSK PN9(defined in 802.15.4) algo
 * qianfan Zhao <qianfanguijin@163.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include "wisun_fsk_common.h"

#define URH_WIRUN_FSK_PLUGIN_VERSION		"1.0.1"

static int option_human = 0;
static int option_hexo = 0;

#if DEBUG > 0
static const char *str01_strstr(const char *str01, const char *needle)
{
	int match_idx = 0, match_length = strlen(needle);
	const char *start = NULL;

	for (const char *s = str01; *s != '\0'; s++) {
		if (*s == '-' || *s == ':')
			continue;

		if (*s == needle[match_idx]) {
			if (match_idx == 0)
				start = s;

			if (++match_idx == match_length)
				return start;
		} else {
			match_idx = 0;
		}
	}

	return NULL;
}

static int str01_strncmp(const char *str01, const char *s2, size_t n)
{
	for (const char *s = str01; s - str01 < n; s++) {
		int diff;

		/* too short */
		if (*s == '\0')
			return -1;

		if (*s == '-' || *s == ':')
			continue;

		/* different length */
		if (*s2 == '\0')
			return -1;

		diff = *s - *s2;
		if (diff)
			return diff;

		s2++;
	}

	return 0;
}
#else
static const char *str01_strstr(const char *haystack, const char *needle)
{
	return strstr(haystack, needle);
}

static int str01_strncmp(const char *s1, const char *s2, size_t n)
{
	return strncmp(s1, s2, n);
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

static void foreach_bit_in_msbfirst_buffer(const uint8_t *buf, size_t bit_size,
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

struct wisun_2fsk_fec_encoder {
	uint8_t		m;

	/* encode buffers */
	uint8_t		*buf;
	size_t		bufsz;
	size_t		encode_bits;
	size_t		byte_idx;
	uint8_t		bit_idx;
};

static int wisun_2fsk_fec_encoder_push_2bits(struct wisun_2fsk_fec_encoder *arg,
					     uint8_t u)
{
	if (arg->byte_idx < arg->bufsz) {
		/* u is 2 bit, push it to arg->buf, align left */
		int lshift = 6 - arg->bit_idx;

		u &= 0b11;
		arg->buf[arg->byte_idx] |= (u << lshift);
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

	binary_size = strict_str01_to_buffer(str01, buf, sizeof(buf), 0);
	if (!binary_size)
		return -1;

	foreach_bit_in_msbfirst_buffer(buf, binary_size,
				       wisun_2fsk_nrnsc_input_bit, &arg);

	if (option_hexo)
		print_hex_bytes(encode_buf, roundup8(arg.encode_bits));
	else
		print_binary_bits_msbfirst(encode_buf, 0, arg.encode_bits - 1,
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

	binary_size = strict_str01_to_buffer(str01, buf, sizeof(buf), 0);
	if (!binary_size)
		return -1;

	foreach_bit_in_msbfirst_buffer(buf, binary_size,
				       wisun_2fsk_rsc_input_bit, &arg);

	if (option_hexo)
		print_hex_bytes(encode_buf, roundup8(arg.encode_bits));
	else
		print_binary_bits_msbfirst(encode_buf, 0,
					   arg.encode_bits - 1,
					   group_count);
	printf("\n");

	return 0;
}

static int wisun_fsk_interleaving(const char *str01)
{
	size_t group_count = option_human ? 4 : 0;
	uint8_t buf[4096] = { 0 };
	size_t binary_size;

	binary_size = strict_str01_to_buffer(str01, buf, sizeof(buf), 0);
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
		print_binary_bits_msbfirst(buf, 0, binary_size - 1,
					   group_count);

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
	const char *p, *preamble = str01_strstr(str01, WISUN_2FSK_PREAMBLE);
	enum wisun_2fsk_sfd_type type = WISUN_2FSK_SFD_MAX;
	size_t len_sfd = strlen(wisun_2fsk_phy_sfd_binary_streams[0]);

	if (!preamble) {
		if (next)
			*next = NULL;
		return -1;
	}

	p = preamble;
	if (next)
		*next = p + 1;

	do {
		p += strlen(WISUN_2FSK_PREAMBLE);
	} while (!str01_strncmp(p, WISUN_2FSK_PREAMBLE, strlen(WISUN_2FSK_PREAMBLE)));

	if (ret_preamble_sz)
		*ret_preamble_sz = p - preamble;

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

static int wisun_2fsk_packet_decode(const char *str01)
{
	size_t group_count = option_human ? 4 : 0;
	size_t preamble_sz, binary_size, byte_size, payload_idx;
	enum wisun_2fsk_sfd_type type;
	uint8_t buf[8192] = { 0 };
	const char *endp;
	int idx;

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

	payload_idx = preamble_sz / 8 + 2 /* SFD */ + 2 /* PHR */;
	pn9_payload_decode(&buf[payload_idx], byte_size - payload_idx);

	if (option_hexo) {
		const uint8_t *p = buf;

		print_hex_bytes(p, preamble_sz / 8);
		p += preamble_sz / 8;
		printf("-");

		printf("%04x-", buffer_peek_u16_b1b0(p)); /* sfd */
		p += 2;

		printf("%04x-", buffer_peek_u16_b1b0(p)); /* phr */
		p += 2;

		print_hex_bytes(p, byte_size - payload_idx);
	} else {
		print_binary_bits_lsbfirst(buf, 0, binary_size - 1, group_count);
	}

	printf("\n");
	return 0;
}

static struct option long_options[] = {
	/* name		has_arg,		*flag,		val */
	{ "pn9",	no_argument,		NULL,		'P' },
	{ "rsc",	no_argument,		NULL,		'R' },
	{ "nrnsc",	no_argument,		NULL,		'N' },
	{ "interleaving", no_argument,		NULL,		'i' },
	{ "help",	no_argument,		NULL,		'h' },
	{ "version",	no_argument,		NULL,		'v' },
	{ "decode",	no_argument,		NULL,		'd' },
	{ "encode",	no_argument,		NULL,		'e' },
	{ "hexo",	no_argument,		NULL,		'H' },
	{ "human",	no_argument,		NULL,		'M' },
	{ NULL,		0,			NULL,		0   },
};

static void print_usage(void)
{
	fprintf(stderr, "Usage: urh_wisun_fsk [OPTIONS] \"binary-strings\"\n");
	fprintf(stderr, "-h --help:              show this message\n");
	fprintf(stderr, "-v --version:           show plugin version\n");
	fprintf(stderr, "-d --decode:            decode a wisun 2-fsk packet\n");
	fprintf(stderr, "-e --encode:            encode a wisun 2-fsk packet\n");
	fprintf(stderr, "   --hexo:              print the encode/decode result in hex mode\n");
	fprintf(stderr, "   --human:             print the output string in human format\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Available algo for encode/decode:\n");
	fprintf(stderr, "   --pn9:               encode/decode binary strings by PN9\n");
	fprintf(stderr, "   --nrnsc:             encode binary strings by NRNSC encoder\n");
	fprintf(stderr, "   --rsc                encode binary string by RSC encoder\n");
	fprintf(stderr, "   --interleaving:      interleaving the input binary blocks\n");
}

enum {
	ALGO_PN9,
	ALGO_NRNSC,
	ALGO_RSC,
	ALGO_INTERLEAVING,
};

int main(int argc, char **argv)
{
	int algo = -1, decode = 1;
	int ret = -1;

	while (1) {
		int option_index = 0;
		int c;

		c = getopt_long(argc, argv, "hv", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'P': /* PN9 */
			algo = ALGO_PN9;
			break;
		case 'N':
			algo = ALGO_NRNSC;
			break;
		case 'R':
			algo = ALGO_RSC;
			break;
		case 'i':
			algo = ALGO_INTERLEAVING;
			break;

		case 'h':
			print_usage();
			return 0;
		case 'v':
			printf("version: %s\n", URH_WIRUN_FSK_PLUGIN_VERSION);
			return 0;
		case 'd': /* decode */
		case 'e': /* encode */
			decode = c == 'd';
			break;
		case 'H': /* hex output */
			option_hexo = 1;
			break;
		case 'M':
			option_human = 1;
			break;
		}
	}

	if (!(optind < argc)) {
		print_usage();
		return -1;
	}

	switch (algo) {
	default:
		if (decode)
			ret = wisun_2fsk_packet_decode(argv[optind]);
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
