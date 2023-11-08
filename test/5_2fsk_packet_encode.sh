# Wisun 2-FSK packet encode test scripts
# qianfan Zhao <qianfanguijin@163.com>

sequence=1

encode_test () {
    local raw=$1 expected=$2
    local decode

    shift 2

    printf "urh_wisun_fsk packet encode test ${sequence}... "

    decode=$(./urh_wisun_fsk.debug --packet --encode "$@" ${raw})

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
# ref: test/1_2fsk_packet_decode.sh
encode_test "1122" \
            "010101010101010101010101010101010101010101010101010101010101010110010000010011100000100000000110100001110011010010100101110100010101011111010111" \
            "--hexi" \
            "--preamble-size" "64" \
            "--sfd" "uncoded0" \
            "--whitening" \
            || exit $?

# # Sequence 2
encode_test "1000-1000-0100-0100" \
            "010101010101010101010101010101010101010101010101010101010101010110010000010011100000100000000110100001110011010010100101110100010101011111010111" \
            "--preamble-size" "64" \
            "--sfd" "uncoded0" \
            "--whitening" \
            || exit $?

# Sequence 3
# https://mentor.ieee.org/802.15/dcn/14/15-14-0225-00-0000-802-15-4-frame-examples.pdf
# Section 3.2 Example 1
#
# Data whitening:       Disabled
# FEC:                  Disabled
# Interleaving:         Disabled
encode_test "02006a" \
            "0101-0101-0101-0101-0101-0101-0101-0101-1001-0000-0100-1110-0000-0000-0000-0111-0100-0000-0000-0000-0101-0110-0101-1101-0010-1001-1111-1010-0010-1000" \
            "--hexi" \
            "--preamble-size" "32" \
            "--sfd" "uncoded0" \
            "--human" \
            || exit $?

# Sequence 4
encode_test "02006a" \
            "aaaaaaaa097200e002006aba945f14" \
            "--hexi" \
            "--preamble-size" "32" \
            "--sfd" "uncoded0" \
            "--hexo" \
            || exit $?

# Sequence 5
# Section 3.3 Example 2
# Data whitening:       Enabled
# FEC:                  Disabled
# Interleaving:         Disabled
encode_test "02006a" \
            "0101-0101-0101-0101-0101-0101-0101-0101-1001-0000-0100-1110-0000-1000-0000-0111-0100-1111-0111-0000-1110-0101-0011-0010-0110-1010-0110-0010-0110-0000" \
            "--hexi" \
            "--preamble-size" "32" \
            "--sfd" "uncoded0" \
            "--whitening" \
            "--human" \
            || exit $?

# Sequence 6
# Section 3.5 Example 4
# Data whitening:       Disabled
# FEC:                  NRNSC
# Interleaving:         Enabled
encode_test "02006a" \
            "0101-0101-0101-0101-0101-0101-0101-0101-0110-0011-0010-1101-1011-1111-0111-1111-0011-1111-1111-1111-1111-1100-1111-1101-1111-1100-1111-0010-0011-0111-1010-1010-1011-1100-1011-0111-0101-1110-0001-0011-1010-0100-0101-1101-1011-0010-1111-0000-1011-0100-0011-1100" \
            "--hexi" \
            "--packet" \
            "--preamble-size" "32" \
            "--sfd" "coded1" \
            "--nrnsc" \
            "--interleaving" \
            "--human" \
            || exit $?

# Sequence 7
# Section 3.4 Example 3
# Data whitening:       Enabled
# FEC:                  RSC
# Interleaving:         Enabled
encode_test "02006a" \
            "0101-0101-0101-0101-0101-0101-0101-0101-0110-1111-0100-1110-1100-0000-1110-1000-0110-1000-0000-1100-1010-0101-1101-1010-1011-0000-0110-1111-1001-0000-1001-1100-0001-1111-1110-0110-1010-1010-0100-1100-1010-0000-1110-1001-1001-1001-1001-1111-0001-0010-1001-1011" \
            "--hexi" \
            "--packet" \
            "--preamble-size" "32" \
            "--sfd" "coded0" \
            "--rsc" \
            "--interleaving" \
            "--whitening" \
            "--human" \
            || exit $?
