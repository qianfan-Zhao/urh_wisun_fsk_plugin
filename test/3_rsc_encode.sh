# Wi-SUN RSC encode test. The test database is copied from:
# https://mentor.ieee.org/802.15/dcn/14/15-14-0225-00-0000-802-15-4-frame-examples.pdf
# Section 3.4 Example 3
#
# qianfan Zhao <qianfanguijin@163.com>

input="00001000000001110100000000000000010101100101110100101001111110100010100001001011"
expected="0000-0000-1110-1000-0010-1000-0001-1111-0011-1010-0000-1010-0000-1010-0000-1010-0011-0011-1001-0100-0001-0001-0101-0011-0000-1110-0110-1001-1101-0101-1110-1100-1010-1110-1100-1010-0011-0000-1110-0101"

echo -n "Wi-SUN FSK RSC encode testing... "
output=$(./urh_wisun_fsk --human --rsc ${input})

if [ X"${output}" != X"${expected}" ] ; then
    echo "failed"
    echo "E: ${expected}"
    echo "R: ${output}"

    exit 1
fi

echo "pass"
exit 0
