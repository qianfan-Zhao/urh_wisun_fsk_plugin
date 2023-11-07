# Simple Makefile for urh_wisun_fsk_plugin project
# qianfan Zhao <qianfanguijin@163.com>

all: urh_wisun_fsk urh_wisun_fsk.debug

clean:
	@rm -f urh_wisun_fsk
	@rm -f urh_wisun_fsk.debug

COMMON_FILE=src/wisun_fsk_common.c

urh_wisun_fsk.debug: src/urh_wisun_fsk.c ${COMMON_FILE}
	${CC} -Wall -g -O0 -Wno-unused-function -DDEBUG=1 $^ -o $@

urh_wisun_fsk: src/urh_wisun_fsk.c ${COMMON_FILE}
	${CC} -Wall -O2 -Wno-unused-function $^ -o $@

test: urh_wisun_fsk urh_wisun_fsk.debug
	@for script in ./test/*.sh ; do \
		if ! $${script} ; then \
			echo "running $${script} failed"; \
			exit 1; \
		fi; \
		echo ; \
	done

.PHONE=all clean test
