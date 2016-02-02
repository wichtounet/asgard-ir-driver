user=pi
pi=192.168.20.161
password=raspberry
dir=/home/${user}/asgard/asgard-server/

default: release

.PHONY: default release debug all clean

include make-utils/flags-pi.mk
include make-utils/cpp-utils.mk

CXX_FLAGS += -pedantic
LD_FLAGS  += -llirc_client

$(eval $(call auto_folder_compile,src))
$(eval $(call auto_add_executable,ir_driver))

release: release_ir_driver
release_debug: release_debug_ir_driver
debug: debug_ir_driver

all: release release_debug debug

run: release
	./release/bin/ir_driver

remote_make:
	sshpass -p ${password} scp Makefile ${user}@${pi}:${dir}/
	sshpass -p ${password} scp src/*.cpp ${user}@${pi}:${dir}/
	sshpass -p ${password} ssh ${user}@${pi} "cd ${dir} && make"

remote_run:
	sshpass -p ${password} ssh ${user}@${pi} "cd ${dir} && make run"

clean: base_clean

include make-utils/cpp-utils-finalize.mk
