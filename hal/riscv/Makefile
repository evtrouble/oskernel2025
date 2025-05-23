support_platform = qemu k210
sbi_support = rustsbi-qemu rustsbi-k210

# 定义SBI路径
SBI_PATH = $(WORKPATH)/hal/riscv/SBI

# SBI 路径定义（使用统一路径）
SBI_PATH = $(WORKPATH)/hal/riscv/SBI

# >>>> 
# *******************************************************

# *******************************************************
# >>>> make function
# >>>> 

# card src function
# -> $1: directory
# -> $2: file type 
card_src = $(foreach d,$(wildcard $1*),$(call card_src,$d/,$2)$(filter $2,$d))

# card all sub-directory, but ignore 'include'
# -> $1: directory to card all sub-direcotry, whose suffix should be '/', such as 'kernel/'
#        null means current directory
card_dir = $(foreach d, $(filter-out include/, $(wildcard $1*/)), $(call card_dir, $1$d) $d)

# >>>>
# ********************************************************

# ********************************************************
# >>>> compile flags
# >>>>

INCLUDE_FLAGS = -I$(WORKPATH)/thirdparty/EASTL/include \
				-I$(WORKPATH)/thirdparty/EASTL/test/packages/EABase/include/Common \
               -I$(WORKPATH)/kernel/include \
			   -I$(WORKPATH)/hsai/include \
			   -DRUSTSBI_ENABLED \
				-mcmodel=medany

export CFLAGS += $(INCLUDE_FLAGS)
export CXXFLAGS += $(INCLUDE_FLAGS)

# >>>>
# ********************************************************

# check platform
plat_name = \
	$(if $(filter $(CONF_PLATFORM),$(support_platform)), \
	$(CONF_PLATFORM), \
	$(error platform '${CONF_PLATFORM}' is not supported)\
	)
plat_name := $(strip $(plat_name))

# architecture name
arch_name = $(CONF_ARCH)

# target name
target_name = hal_$(arch_name)_$(plat_name)

# card source
c_src = $(wildcard *.c)
cc_src = $(wildcard *.cc)
s_src = $(wildcard *.S)

srcs = $(c_src) $(cc_src) $(s_src)

# card obj file name
c_obj = $(patsubst %.c,$(BUILDPATH)/hal/$(arch_name)/%.o,$(c_src))
cc_obj = $(patsubst %.cc,$(BUILDPATH)/hal/$(arch_name)/%.o,$(cc_src))
s_obj = $(patsubst %.S,$(BUILDPATH)/hal/$(arch_name)/%.o,$(s_src))

objs = $(c_obj) $(cc_obj) $(s_obj)

# the source files' dependencies, created by gcc
depends = \
	$(patsubst %.cc, $(BUILDPATH)/hal/$(arch_name)/%.d, $(cc_src)) \
	$(patsubst %.c, $(BUILDPATH)/hal/$(arch_name)/%.d, $(c_src)) \
	$(patsubst %.S, $(BUILDPATH)/hal/$(arch_name)/%.d, $(s_src))


.PHONY: all clean test initdir add_libcpu compile_platform build_sbi rustsbi-clean

all: initdir build_sbi compile_platform add_libcpu

# 添加SBI构建目标
build_sbi:
ifeq ($(CONF_PLATFORM),k210)
	@if which cargo >/dev/null; then \
        echo "Building RustSBI for K210 from source..."; \
        cd $(SBI_PATH)/rustsbi-k210 && \
        cargo build --release && \
        cp target/riscv64gc-unknown-none-elf/release/rustsbi-k210 $(BUILDPATH)/; \
    else \
        echo "Using pre-built RustSBI for K210..."; \
        cp $(SBI_PATH)/sbi-k210 $(BUILDPATH)/rustsbi-k210; \
    fi
	@$(OBJDUMP) -S $(BUILDPATH)/rustsbi-k210 > $(BUILDPATH)/rustsbi-k210.asm
else
	@if which cargo >/dev/null; then \
        echo "Building RustSBI for QEMU from source..."; \
        cd $(SBI_PATH)/rustsbi-qemu && \
        cargo build --release && \
        cp target/riscv64gc-unknown-none-elf/release/rustsbi-qemu $(BUILDPATH)/; \
    else \
        echo "Using pre-built RustSBI for QEMU..."; \
        cp $(SBI_PATH)/sbi-qemu $(BUILDPATH)/rustsbi-qemu; \
    fi
	@$(OBJDUMP) -S $(BUILDPATH)/rustsbi-qemu > $(BUILDPATH)/rustsbi-qemu.asm
endif

initdir:
	mkdir -p $(BUILDPATH)/hal/$(arch_name)

compile_platform:
	$(MAKE) all -C $(plat_name)

add_libcpu: $(BUILDPATH)/$(target_name).a

$(BUILDPATH)/$(target_name).a: $(objs)
	$(AR) rc $(BUILDPATH)/$(target_name).a $(objs)

$(cc_obj): $(BUILDPATH)/hal/$(arch_name)/%.o: %.cc 
	$(CXX) -c $(CXXFLAGS) -MF $(BUILDPATH)/hal/$(arch_name)/$*.d -o $@ $<

$(c_obj): $(BUILDPATH)/hal/$(arch_name)/%.o: %.c 
	$(CC) -c $(CFLAGS) -MF $(BUILDPATH)/hal/$(arch_name)/$*.d -o $@ $<

$(s_obj): $(BUILDPATH)/hal/$(arch_name)/%.o: %.S 
	$(AS) -c $(ASFLAGS) -MF $(BUILDPATH)/hal/$(arch_name)/$*.d -o $@ $<

-include $(depends)

test:
	@echo $(plat_name)
	@echo $(target_name)
#	@echo $(call test_wild, $(test_dir), %.$(test_cc_ext))
#	@echo $(call test_wild, $(test_dir), %.$(test_c_ext))
#	@echo $(call test_wild, $(test_dir), %.$(test_S_ext))
#	@echo "--------> depd  : $(depends)"
	@echo "--------> srcs  : $(c_src)"
	@echo "--------> srcs  : $(cc_src)"
# @echo "--------> srcs  : $(s_src)"
# @echo "--------> srcs  : $(srcs)"
#	@echo "--------> objs  : $(objs)"
#	@echo "--------> build : $(BUILDPATH)"

clean: 
	cd "$(BUILDPATH)/hal/$(arch_name)"; rm -f *.o *.d
	$(MAKE) clean -C $(plat_name)

rustsbi-clean:
ifeq ($(CONF_PLATFORM),k210)
	@cd $(SBI_PATH)/rustsbi-k210 && cargo clean
	@rm -f $(BUILDPATH)/rustsbi-k210
else
	@cd $(SBI_PATH)/rustsbi-qemu && cargo clean
	@rm -f $(BUILDPATH)/rustsbi-qemu
endif
