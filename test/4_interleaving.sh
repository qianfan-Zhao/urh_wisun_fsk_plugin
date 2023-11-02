# Wi-SUN interleaving test. The test database is copied from:
# https://mentor.ieee.org/802.15/dcn/14/15-14-0225-00-0000-802-15-4-frame-examples.pdf
# Section 3.4.6
#
# qianfan Zhao <qianfanguijin@163.com>

input="0000-0000-1110-1000-0010-1000-0001-1111-0011-1010-0000-1010-0000-1010-0000-1010-0011-0011-1001-0100-0001-0001-0101-0011-0000-1110-0110-1001-1101-0101-1110-1100-1010-1110-1100-1010-0011-0000-1110-0101"
expected="1100-0000-1110-1000-0110-1000-0000-1100-1010-1010-1010-1010-0000-0011-0000-0000-1101-0011-0000-0100-0101-0111-0100-1000-0001-0110-1101-1011-1001-1000-1111-0100-0100-1010-0100-1011-1011-0010-1100-1110"

interleaving_test () {
    local i=$1 e=$2
    local out

    out=$(./urh_wisun_fsk.debug --human --interleaving ${i})
    if [ X"${out}" != X"${e}" ] ; then
        echo
        echo "E: ${e}"
        echo "R: ${out}"
        echo "failed"

        return 1
    fi
}

echo -n "Wi-SUN interleaving testing... "
interleaving_test ${input} ${expected} || exit $?
interleaving_test ${expected} ${input} || exit $?
echo "pass"
