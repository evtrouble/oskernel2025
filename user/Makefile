# 添加架构特定的宏定义和路径
ifeq ($(CONF_ARCH), riscv)
CFLAGS += -DARCH_RISCV=1
ARCH_DIR = riscv
else ifeq ($(CONF_ARCH), loongarch)
CFLAGS += -DARCH_LOONGARCH=1
ARCH_DIR = loongarch
endif

# 添加包含路径
INCLUDE_FLAGS = -I$(WORKPATH)/user/include -I$(WORKPATH)/user/$(ARCH_DIR)
CXXFLAGS += $(INCLUDE_FLAGS)
CFLAGS += $(INCLUDE_FLAGS)
ASFLAGS += $(INCLUDE_FLAGS) -x assembler-with-cpp

# 源文件
SRCS = $(wildcard *.c)
SRCS += $(wildcard $(ARCH_DIR)/*.c)
ASM_SRCS = $(wildcard $(ARCH_DIR)/*.S)

# 目标文件
OBJS = $(SRCS:%.c=$(BUILDPATH)/user/%.o)
ASM_OBJS = $(ASM_SRCS:%.S=$(BUILDPATH)/user/%.o)

all: initdir $(BUILDPATH)/user/user.a

$(BUILDPATH)/user/user.a: $(OBJS) $(ASM_OBJS)
	$(AR) rcs $@ $^

# 编译规则
$(BUILDPATH)/user/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDPATH)/user/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

initdir:
	@mkdir -p $(BUILDPATH)/user
	@mkdir -p $(BUILDPATH)/user/$(ARCH_DIR)

clean:
	rm -rf $(BUILDPATH)/user

.PHONY: all clean initdir
