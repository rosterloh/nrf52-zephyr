BOARD ?= nrf52_pca10040
CONF_FILE = prj.conf
QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr

include $(ZEPHYR_BASE)/Makefile.inc
