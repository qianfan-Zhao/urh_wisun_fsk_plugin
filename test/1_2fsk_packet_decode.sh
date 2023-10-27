# Wisun 2-FSK packet decode test scripts
# qianfan Zhao <qianfanguijin@163.com>

# Preamble + sfd + phr + payload
# 10001000 <=> 10000111
# 1000100001000100 <=> 1000011100110100
simple_2fsk_packet="
01010101-1001000001001110-0000000000000001-10000111
01010101-1001000001001110-0000000000000001-10001000

0101010101010101-1001000001001110-0000000000000001-10000111
0101010101010101-1001000001001110-0000000000000001-10001000

01010101010101010101010101010101-1001000001001110-0000000000000001-10000111
01010101010101010101010101010101-1001000001001110-0000000000000001-10001000

01010101-1001000001001110-0000000000000010-1000011100110100
01010101-1001000001001110-0000000000000010-1000100001000100

011001-01010101-1001000001001110-0000000000000010-1000011100110100
01010101-1001000001001110-0000000000000010-1000100001000100

011001-01010101-101010-0101010101010101-1001000001001110-0000000000000010-1000011100110100
01010101-1001000001001110-0000000000000010-1000100001000100
"

# $1: the raw binary string bits
# $2: the expected binary string bits result after encoding
decode_2fsk_packet_test() {
    local decode raw=$1 expected=$(echo $2 | tr -d '-')

    printf "P: ${raw}\nE: ${expected}\n"

    decode=$(./urh_wisun_fsk.debug --decode "${raw}")
    if [ X"${expected}" != X"${decode}" ] ; then
        printf "R: ${decode}\nx\n"
        return 1
    fi
}

decode_test () {
    local retval=0

    echo "urh_wisun_fsk packet decode testing..."

    while [ $# -gt 0 ] ; do
        local raw=$1 expected=$2

        shift 2

        if ! decode_2fsk_packet_test "${raw}" "${expected}" ; then
            let retval++
        fi
    done

    return ${retval}
}

decode_hexo_test () {
    local raw="010101010101010101010101010101010101010101010101010101010101010110010000010011100000100000000110100001110011010010100101110100010101011111010111"
    local expected="aaaaaaaaaaaaaaaa-7209-6010-1122687d28f2"
    local decode

    printf "\nurh_wisun_fsk packet decode with --hexo testing... "

    # A packet copied from urh when RAILtest follow this configurations:
    # > settxlength 4
    # {{(settxlength)}{TxLength:4}{TxLength Written:4}}
    # > set802154phr 1 0 1
    # {{(set802154phr)}{PhrSize:2}{PHR:0x6010}}
    # {{(set802154phr)}{len:4}{payload: 0x10 0x60 0x11 0x22}}
    decode=$(./urh_wisun_fsk.debug --hexo --decode ${raw})

    if [ X"${decode}" != X"${expected}" ] ; then
        printf "\nE: ${expected}\nR: ${decode}\n"
        printf "failed\n"
        return 1
    else
        printf "pass\n"
    fi
}

decode_test ${simple_2fsk_packet} || exit $?
decode_hexo_test || exit $?
