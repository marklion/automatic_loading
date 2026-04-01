SHELL=/bin/bash
SRC_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
DELIVER_PATH=$(SRC_DIR)/build
CMAKE_TMP_DIR=$(SRC_DIR)/cmake_tmp_dir
SM_OUT_CODE_PATH=$(SRC_DIR)/sm_gen_code
SM_IDL_FILE=$(SRC_DIR)/../state_machine/state_machine_idl.thrift

all:$(DELIVER_PATH) $(SM_OUT_CODE_PATH) lib test
	cp -a $(DELIVER_PATH)/* $(OUTBOUND_DELIVER_PATH)/

$(DELIVER_PATH):
	[ -d $(DELIVER_PATH) ] || mkdir -p $(DELIVER_PATH)

$(SM_OUT_CODE_PATH):$(SM_OUT_CODE_PATH)/cpp
	[ -d $@/build ] || mkdir $@/build

$(SM_OUT_CODE_PATH)/cpp:$(SM_IDL_FILE) $(DELIVER_PATH)
	[ -d $@ ] || mkdir -p $@
	thrift -out $@ --gen cpp:no_skeleton $(SM_IDL_FILE)

lib:$(DELIVER_PATH) $(SM_OUT_CODE_PATH)
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
test:$(DELIVER_PATH) lib
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
clean:
	rm -rf $(DELIVER_PATH)
	rm -rf $(CMAKE_TMP_DIR)

.PHONY:all $(DELIVER_PATH) clean lib
