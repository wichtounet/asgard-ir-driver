default: release

.PHONY: default release debug all clean

include make-utils/flags.mk
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

clean: base_clean

include make-utils/cpp-utils-finalize.mk
