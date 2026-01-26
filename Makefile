SHELL=/bin/bash
SRC_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
SRC_DIR:=$(SRC_DIR)/src
DELIVER_PATH=$(SRC_DIR)/../build
SUB_DIR=public config log cli modbus_io state_machine lidar xlrd live_camera hn_hht plate_gate scale
BUILD_MODE=build
OUTBOUND_DELIVER_PATH=$(DELIVER_PATH)
export BUILD_MODE
export OUTBOUND_DELIVER_PATH

.SILENT:
pack:all
	tar zcf ad_deliver.tar.gz -C $(DELIVER_PATH) lib bin dist conf
	cat $(SRC_DIR)/../deploy.sh ad_deliver.tar.gz > $(DELIVER_PATH)/install.sh
	chmod +x $(DELIVER_PATH)/install.sh
	rm ad_deliver.tar.gz

all:$(SUB_DIR)

$(SUB_DIR):$(DELIVER_PATH)
	[ -d $(DELIVER_PATH) ] || mkdir $(DELIVER_PATH)
	$(MAKE) -C $(SRC_DIR)/$@
	for component in $^;do [ -d $(SRC_DIR)/$$component/build ] && cp -a $(SRC_DIR)/$$component/build/* $(DELIVER_PATH)/ || echo no_assert; done

config:public
log:public config
modbus_io:log
state_machine:modbus_io log
lidar:state_machine
xlrd:state_machine
live_camera:log
hn_hht:log
plate_gate:state_machine hn_hht
cli:log modbus_io state_machine lidar xlrd live_camera hn_hht plate_gate scale
scale:state_machine

clean:
	rm -rf $(DELIVER_PATH)
	for sub_component in $(SUB_DIR); do make clean -C $(SRC_DIR)/$$sub_component;done

.PHONY:all $(SUB_DIR) $(DELIVER_PATH) clean pack