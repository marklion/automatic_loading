SHELL=/bin/bash
SRC_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
DELIVER_PATH=$(SRC_DIR)/build
OUT_CODE_PATH=$(SRC_DIR)/gen_code
IDL_FILE=$(SRC_DIR)/public_idl.thrift
CMAKE_TMP_DIR=$(SRC_DIR)/cmake_tmp_dir

all:$(OUT_CODE_PATH) $(DELIVER_PATH) lib daemon web
	cp -a $(DELIVER_PATH)/* $(OUTBOUND_DELIVER_PATH)/

$(OUT_CODE_PATH):$(OUT_CODE_PATH)/cpp
	[ -d $@/build ] || mkdir $@/build

$(OUT_CODE_PATH)/cpp:$(IDL_FILE) $(DELIVER_PATH)
	[ -d $@ ] || mkdir -p $@
	thrift -out $@ --gen cpp:no_skeleton $(IDL_FILE)

$(DELIVER_PATH):
	[ -d $(DELIVER_PATH) ] || mkdir -p $(DELIVER_PATH)
lib:$(DELIVER_PATH)
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
daemon:$(DELIVER_PATH) lib
	[ -d $(CMAKE_TMP_DIR)/$@ ] || mkdir -p $(CMAKE_TMP_DIR)/$@
	pushd $(CMAKE_TMP_DIR)/$@;[ -f Makefile ] || cmake -D PRJ_INTERNAL_BUILD=$(OUTBOUND_DELIVER_PATH) -D CUR_BUILD_PATH=$(DELIVER_PATH) $(SRC_DIR)/$@; $(MAKE)
web:$(DELIVER_PATH)
	mkdir -p $(DELIVER_PATH)/dist
	if [ -n "$$(find $(SRC_DIR)/web/cli_server -type f \( -path "*/dist/*" -prune -o \( -name '*.js' -o -name '*.json' -o -name '*.vue' -o -name 'webpack.config.js' \) \) -newer $(SRC_DIR)/web/cli_server/.build_timestamp 2>/dev/null | head -1)" ] || [ ! -f "$(SRC_DIR)/web/cli_server/.build_timestamp" ]; then \
		pushd $(SRC_DIR)/web/cli_server && npm install && npm run build && touch .build_timestamp; \
	else \
		echo "npm dependencies and build are up to date"; \
	fi
	cp -a $(SRC_DIR)/web/cli_server/dist/* $(DELIVER_PATH)/dist/

clean:
	rm -rf $(OUT_CODE_PATH)
	rm -rf $(DELIVER_PATH)
	rm -rf $(CMAKE_TMP_DIR)

.PHONY:all $(DELIVER_PATH) $(OUT_CODE_PATH) clean lib daemon cli