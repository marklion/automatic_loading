SHELL=/bin/bash
SRC_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
DELIVER_PATH=$(SRC_DIR)/build
CMAKE_TMP_DIR=$(SRC_DIR)/cmake_tmp_dir

all:$(DELIVER_PATH) lib test
	cp -a $(DELIVER_PATH)/* $(OUTBOUND_DELIVER_PATH)/

$(DELIVER_PATH):
	[ -d $(DELIVER_PATH) ] || mkdir -p $(DELIVER_PATH)
lib:$(DELIVER_PATH)
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
test:$(DELIVER_PATH) lib
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
clean:
	rm -rf $(DELIVER_PATH)
	rm -rf $(CMAKE_TMP_DIR)

.PHONY:all $(DELIVER_PATH) clean lib