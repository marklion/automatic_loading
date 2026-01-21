SHELL=/bin/bash
SRC_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
DELIVER_PATH=$(SRC_DIR)/build

all:$(DELIVER_PATH) video_cast
	cp -a $(DELIVER_PATH)/* $(OUTBOUND_DELIVER_PATH)/

$(DELIVER_PATH):
	[ -d $(DELIVER_PATH) ] || mkdir -p $(DELIVER_PATH)
video_cast:$(DELIVER_PATH)
	mkdir -p $(DELIVER_PATH)/dist
	if [ -n "$$(find $(SRC_DIR)/$@/ -type f \( -path "*/dist/*" -prune -o \( -name '*.js' -o -name '*.json' -o -name '*.vue' -o -name 'webpack.config.js' \) \) -newer $(SRC_DIR)/$@/.build_timestamp 2>/dev/null | head -1)" ] || [ ! -f "$(SRC_DIR)/$@/.build_timestamp" ]; then \
		pushd $(SRC_DIR)/$@ && npm install && npm run build:linux && touch .build_timestamp; \
	else \
		echo "npm dependencies and build are up to date"; \
	fi
	cp $(SRC_DIR)/$@/dist/*.AppImage $(DELIVER_PATH)/dist/$@.AppImage
clean:
	rm -rf $(DELIVER_PATH)

.PHONY:all $(DELIVER_PATH) clean video_cast