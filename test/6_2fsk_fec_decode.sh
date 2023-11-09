# Wisun 2-FSK RSC/NRNSC decode test scripts
# qianfan Zhao <qianfanguijin@163.com>

sequence=1

fec_test () {
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
# Ref: test/2_nrnsc_encode.sh
fec_test "00000000000001110100000000000000010101100101110100101001111110100010100000001011" \
         --nrnsc --decode \
        "1111111111111111111111111100011010000100001111111111111111111111110010110111100111111011101010000100111011010011011001010110000100000010110100001111111100101110" \
        || exit $?

# Sequence 2
# Ref: test/3_rsc_encode.sh
fec_test "00001000000001110100000000000000010101100101110100101001111110100010100001001011" \
         --rsc --decode \
        "0000-0000-1110-1000-0010-1000-0001-1111-0011-1010-0000-1010-0000-1010-0000-1010-0011-0011-1001-0100-0001-0001-0101-0011-0000-1110-0110-1001-1101-0101-1110-1100-1010-1110-1100-1010-0011-0000-1110-0101" \
        || exit $?
