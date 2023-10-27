# Wi-SUN NRNSC encode test. The test database is copied from:
# https://mentor.ieee.org/802.15/dcn/14/15-14-0225-00-0000-802-15-4-frame-examples.pdf
# Section 3.5 Example 4
#
# qianfan Zhao <qianfanguijin@163.com>

input="00000000000001110100000000000000010101100101110100101001111110100010100000001011"
expected="1111111111111111111111111100011010000100001111111111111111111111110010110111100111111011101010000100111011010011011001010110000100000010110100001111111100101110"

echo -n "Wi-SUN FSK NRNSC encode testing... "
output=$(./urh_wisun_fsk --nrnsc ${input})

if [ X"${output}" != X"${expected}" ] ; then
    echo "failed"
    echo "E: ${expected}"
    echo "R: ${output}"

    exit 1
fi

echo "pass"
exit 0
