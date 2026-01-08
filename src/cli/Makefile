SHELL=/bin/bash
SRC_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
DELIVER_PATH=$(SRC_DIR)/build
CMAKE_TMP_DIR=$(SRC_DIR)/cmake_tmp_dir

all:tools
	cp -a $(DELIVER_PATH)/* $(OUTBOUND_DELIVER_PATH)/

$(DELIVER_PATH):
	[ -d $(DELIVER_PATH) ] || mkdir $(DELIVER_PATH)
	[ -d $(DELIVER_PATH)/bin ] || mkdir $(DELIVER_PATH)/bin
	[ -d $(DELIVER_PATH)/lib ] || mkdir $(DELIVER_PATH)/lib
tools:$(DELIVER_PATH)
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
clean:
	rm -rf $(OUT_CODE_PATH)
	rm -rf $(DELIVER_PATH)
	rm -rf $(CMAKE_TMP_DIR)

.PHONY: all clean $(DELIVER_PATH) tools