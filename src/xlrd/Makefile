SHELL=/bin/bash
SRC_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
DELIVER_PATH=$(SRC_DIR)/build
OUT_CODE_PATH=$(SRC_DIR)/gen_code
IDL_FILE=$(SRC_DIR)/xlrd_idl.thrift
CMAKE_TMP_DIR=$(SRC_DIR)/cmake_tmp_dir

all:$(OUT_CODE_PATH) lib daemon
	cp -a $(DELIVER_PATH)/* $(OUTBOUND_DELIVER_PATH)/

$(OUT_CODE_PATH):$(OUT_CODE_PATH)/cpp
	[ -d $@/build ] || mkdir $@/build

$(DELIVER_PATH):
	[ -d $(DELIVER_PATH) ] || mkdir $(DELIVER_PATH)
	[ -d $(DELIVER_PATH)/bin ] || mkdir $(DELIVER_PATH)/bin
	[ -d $(DELIVER_PATH)/lib ] || mkdir $(DELIVER_PATH)/lib

$(OUT_CODE_PATH)/cpp:$(IDL_FILE) $(DELIVER_PATH)
	[ -d $@ ] || mkdir -p $@
	thrift -out $@ --gen cpp:no_skeleton $(IDL_FILE)
lib:$(DELIVER_PATH)
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
daemon:$(DELIVER_PATH) lib
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
clean:
	rm -rf $(OUT_CODE_PATH)
	rm -rf $(DELIVER_PATH)
	rm -rf $(CMAKE_TMP_DIR)

.PHONY: all clean $(OUT_CODE_PATH) $(DELIVER_PATH) lib daemon