CXX=g++
LD=g++

default: release

.PHONY: default release debug all clean

include make-utils/flags-pi.mk
include make-utils/cpp-utils.mk

pi.conf:
	echo "user=pi" > pi.conf
	echo "pi=192.168.20.161" >> pi.conf
	echo "password=raspberry" >> pi.conf
	echo "dir=/home/pi/asgard/asgard-ir-driver/" >> pi.conf

conf: pi.conf

include pi.conf


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

remote_clean:
	sshpass -p ${password} ssh ${user}@${pi} "cd ${dir} && make clean"

remote_make:
	sshpass -p ${password} scp Makefile ${user}@${pi}:${dir}/
	sshpass -p ${password} scp src/*.cpp ${user}@${pi}:${dir}/src/
	sshpass -p ${password} ssh ${user}@${pi} "cd ${dir} && make"

remote_run:
	sshpass -p ${password} ssh -t ${user}@${pi} "cd ${dir} && make run"

clean: base_clean

include make-utils/cpp-utils-finalize.mk
