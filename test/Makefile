PWD := $(shell pwd)/..
LIB_TARGET := $(PWD)/libjson.a
EXE_TARGET := $(PWD)/json.elf
BUILD_DIR := $(PWD)/build

# toolchain
#TOOLCHAIN_PATH := /home/gn/Workspace/Project/iotextend_lib/toolchain/x86_64-arago-linux/usr/bin
TOOLCHAIN_PATH := /usr/bin
#PREFIX := aarch64-linux-gnu-
CC := $(TOOLCHAIN_PATH)/$(PREFIX)gcc
AR := $(TOOLCHAIN_PATH)/$(PREFIX)ar

# sources
LIB_SRCS := $(shell find $(PWD)/lib -name '*.c')
LIB_OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(LIB_SRCS:.c=.o)))

EXE_SRCS := $(shell find $(PWD)/test -name '*.c')
EXE_OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(EXE_SRCS:.c=.o)))

vpath %.c $(sort $(dir $(LIB_SRCS)))
vpath %.c $(sort $(dir $(EXE_SRCS)))

CFLAGS := -O2
CFLAGS += -I$(PWD)/lib\
		  -I$(PWD)/test

LFLAGS := -lcheck -lm -lpthread -lrt -lsubunit

all: clean $(BUILD_DIR) $(LIB_TARGET) $(EXE_TARGET)

clean:
	-rm -r $(PWD)/build
	-rm $(LIB_TARGET)
	-rm $(EXE_TARGET)

$(BUILD_DIR):
	@echo $(LIB_SRCS)
	@mkdir $@

$(BUILD_DIR)/%.o: %.c  
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(LIB_TARGET): $(LIB_OBJS)
	$(AR) crv $@ $(LIB_OBJS)

$(EXE_TARGET): $(EXE_OBJS)
	$(CC) $(EXE_OBJS) $(LFLAGS) -L$(PWD) -ljson -o $@
	chmod +x $(EXE_TARGET)

.PHONY: all clean
