# Wisun 2-FSK coded packet decode test scripts.
# qianfan Zhao <qianfanguijin@163.com>

sequence=1

coded_packet_decode_test () {
    local expected=$1
    local decode

    shift 1

    printf "urh_wisun_fsk fec decode test ${sequence}... "

    decode=$(./urh_wisun_fsk.debug "$@")

    if [ X"${decode}" != X"${expected}" ] ; then
        printf "\nE: ${expected}\nR: ${decode}\n"
        printf "failed\n"
        return 1
    else
        printf "pass\n"
    fi

    let sequence++
}

# Sequence 1
# ref: 5_2fsk_packet_encode.sh Section 6 (coded1)
# Data whitening:       Disabled
# FEC:                  NRNSC
# Interleaving:         Enabled
coded_packet_decode_test \
    "aaaaaaaa-b4c6-e000-02006a-ba945f14" \
    "--packet" "--decode" \
    "--nrnsc" \
    "--interleaving" \
    "--human" "--hexo" \
    "0101-0101-0101-0101-0101-0101-0101-0101-0110-0011-0010-1101-1011-1111-0111-1111-0011-1111-1111-1111-1111-1100-1111-1101-1111-1100-1111-0010-0011-0111-1010-1010-1011-1100-1011-0111-0101-1110-0001-0011-1010-0100-0101-1101-1011-0010-1111-0000-1011-0100-0011-1100" \
    || exit $?

# Sequence 2
# ref: 5_2fsk_packet_encode.sh Section 7 (code0)
# Data whitening:       Enabled
# FEC:                  RSC
# Interleaving:         Enabled
coded_packet_decode_test \
    "aaaaaaaa-72f6-e010-02006a-ba945f14" \
    "--packet" "--decode" \
    "--rsc" \
    "--interleaving" \
    "--human" "--hexo" \
    "0101-0101-0101-0101-0101-0101-0101-0101-0110-1111-0100-1110-1100-0000-1110-1000-0110-1000-0000-1100-1010-0101-1101-1010-1011-0000-0110-1111-1001-0000-1001-1100-0001-1111-1110-0110-1010-1010-0100-1100-1010-0000-1110-1001-1001-1001-1001-1111-0001-0010-1001-1011" \
    || exit $?

# Sequence 3
# sending from Silicon Labs RAILtest demo based on EFR32FG25.
#
# Setting:
# Radio profile: Wi-SUN FAN1.1 Profile
# Wi-SUN Regulatory Domain: NA
# Wi-SUN PHY Operating Mode ID: 18
# Wi-SUN Channel Plan ID: 1
#
# The binary stream are capture with HackRF, demodulated with URH.
seq3_inputs="0010101010101010101010101010101010101010101010101010101010101010101101111010011100111001101110011001110111111001100001111110100001011101001011111111001111110001100010111001001101111110000110000011101001101011010000111100000001111101100000011"
coded_packet_decode_test \
    "aaaaaaaaaaaaaaaa-72f6-6010-1122-687d28f2" \
    "--packet" "--decode" \
    "--nrnsc" \
    "--interleaving" \
    "--human" "--hexo" \
    "${seq3_inputs}" \
    || exit $?

# Sequence 4
# similar to seq3, but add some garbage in the tail
coded_packet_decode_test \
    "aaaaaaaaaaaaaaaa-72f6-6010-1122-687d28f2" \
    "--packet" "--decode" \
    "--nrnsc" \
    "--interleaving" \
    "--human" "--hexo" \
    "${seq3_inputs}01100110" \
    || exit $?

# Sequence 5
# similar to seq3, but send 3bytes data
coded_packet_decode_test \
    "aaaaaaaaaaaaaaaa-72f6-e010-112233-58184306" \
    "--packet" "--decode" \
    "--nrnsc" \
    "--interleaving" \
    "--human" "--hexo" \
    "00000000000001010101010101010101010101010101010101010101010101010101010101010110111101001110101100110111001100111011111100110000111111010011101110010101111000001100101011010011110111100001000011001000100011101100000110011001111010101001000100110010100111111" \
    || exit $?
