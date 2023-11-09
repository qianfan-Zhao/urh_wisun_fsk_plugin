Universal Radio Hacker plugin for Wi-SUN 2-FSK
==============================================

urh has a wiki about how to import custom encode/decode plugin, and you can read
it from [github wiki](https://github.com/jopohl/urh/wiki/Decodings)

features:
(x means support, ! means not support)

| feature        | encode | decode |
| -------------- | ------ | ------ |
| data whitening | x      | x      |
| RSC            | x      | x      |
| NRNSC          | x      | x      |
| interleaving   | x      | x      |

More command line examples, please reference [test](./test) cases.

Note:

At present the viterbi decoding algorithm has not been implemented, we use
a very pool decoding method named as `fec_replay_decode` instead, and we can
not fix the error bit now.
