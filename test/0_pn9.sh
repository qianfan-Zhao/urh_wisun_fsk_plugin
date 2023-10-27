# Encode and decode test for `urh_wisun_fsk --pn9`
# qianfan Zhao <qianfanguijin@163.com>

# $1: the raw binary string bits
# $2: the expected binary string bits result after encoding
pn9_encode_decode_test() {
    local encode decode raw=$1 expected=$2

    encode=$(./urh_wisun_fsk --pn9 "${raw}")
    decode=$(./urh_wisun_fsk --pn9 "${expected}")

    printf "%-20s %s " "${raw}" "${expected}"
    if [ X"${expected}" != X"${encode}" ] || [ X"${raw}" != X"${decode}" ] ; then
        printf "x"
        return 1
    fi

    echo
}

# raw_string     encode_string
pn9_encode_strings="
1000100             1000011
10001000            10000111
1000100001000100    1000011100110100
"

pn9_test () {
    local retval=0

    echo "urh_wisun_fsk PN9 paylod encode/decode testing..."

    while [ $# -gt 0 ] ; do
        local raw=$1 expected=$2

        shift 2

        if ! pn9_encode_decode_test "${raw}" "${expected}" ; then
            let retval++
        fi
    done

    return ${retval}
}

pn9_test ${pn9_encode_strings} || exit $?

