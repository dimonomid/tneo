# This makefile accepts two params: TN_ARCH and TN_COMPILER. Both are mandatory.
#
#  TN_ARCH: the following values are valid:
# 
#     cortex_m0
#     cortex_m0plus
#     cortex_m1
#     cortex_m3
#     cortex_m4
#     cortex_m4f
#
#     pic32mx
#
#     pic24_dspic_noeds
#     pic24_dspic_eds     
#
#  TN_COMPILER: depends on TN_ARCH.
#     For cortex-m series, the following values are valid:
#
#        arm-none-eabi-gcc
#        clang
#
#     For pic32mx, just one value is valid:
#
#        xc32
#
#     For pic24/dspic, just one value is valid:
#
#        xc16
#
#
#
#  Example invocation:
#
#     $ make TN_ARCH=cortex_m3 TN_COMPILER=arm-none-eabi-gcc
#





#---------------------------------------------------------------------------
# Cortex-M series
#---------------------------------------------------------------------------

ifeq ($(TN_ARCH), $(filter $(TN_ARCH), cortex_m0 cortex_m0plus cortex_m1 cortex_m3 cortex_m4 cortex_m4f))
   TN_ARCH_DIR = cortex_m

   ifeq ($(TN_COMPILER), $(filter $(TN_COMPILER), arm-none-eabi-gcc clang))

      ifeq ($(TN_ARCH), cortex_m0)
         CORTEX_M_FLAGS = -mcpu=cortex-m0 -mfloat-abi=soft
      endif
      ifeq ($(TN_ARCH), cortex_m0plus)
         CORTEX_M_FLAGS = -mcpu=cortex-m0plus -mfloat-abi=soft
      endif
      ifeq ($(TN_ARCH), cortex_m1)
         CORTEX_M_FLAGS = -mcpu=cortex-m1 -mfloat-abi=soft
      endif
      ifeq ($(TN_ARCH), cortex_m3)
         CORTEX_M_FLAGS = -mcpu=cortex-m3 -mfloat-abi=soft
      endif
      ifeq ($(TN_ARCH), cortex_m4)
         CORTEX_M_FLAGS = -mcpu=cortex-m4 -mfloat-abi=soft
      endif
      ifeq ($(TN_ARCH), cortex_m4f)
         CORTEX_M_FLAGS = -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
      endif

      ifeq ($(TN_COMPILER), arm-none-eabi-gcc)
         CC = arm-none-eabi-gcc
         AR = arm-none-eabi-ar
         CFLAGS = $(CORTEX_M_FLAGS) -mthumb -Os -fsigned-char -ffunction-sections -fdata-sections -g3 -Wall
         ASFLAGS = $(CFLAGS) -x assembler-with-cpp
         TN_COMPILER_VERSION := $(shell $(CC) --version | grep '^gcc' | sed 's/^.* //g')

         BINARY_CMD = $(AR) -r $(BINARY) $(OBJS)
      endif

      ifeq ($(TN_COMPILER), clang)
         CC = clang
         AR = ar  #TODO: probably use clang archiver?
         CFLAGS = $(CORTEX_M_FLAGS) -target arm-none-eabi -mthumb -Os -fsigned-char -ffunction-sections -fdata-sections -g3 -Wall
         ASFLAGS = $(CFLAGS) -x assembler-with-cpp
         TN_COMPILER_VERSION := 

         BINARY_CMD = $(AR) -r $(BINARY) $(OBJS)
      endif
   endif

endif




#---------------------------------------------------------------------------
# PIC32 series
#---------------------------------------------------------------------------

ifeq ($(TN_ARCH), $(filter $(TN_ARCH), pic32mx))
   TN_ARCH_DIR = pic32

   ifeq ($(TN_COMPILER), $(filter $(TN_COMPILER), xc32))

      ifeq ($(TN_ARCH), pic32mx)
         PIC32MX_FLAGS = -mprocessor=32MX440F512H
      endif

      ifeq ($(TN_COMPILER), xc32)
         CC = xc32-gcc
         AR = xc32-ar
         CFLAGS = $(PIC32MX_FLAGS) -g -x c -Os -ffunction-sections -fdata-sections -Wall
         ASFLAGS = $(PIC32MX_FLAGS)
         TN_COMPILER_VERSION := $(shell $(CC) --version | grep '^xc32-gcc' | sed -r 's/^.*(v[0-9.]+).*/\\1/g')

         BINARY_CMD = $(AR) -r $(BINARY) $(OBJS)
      endif

   endif
endif



#---------------------------------------------------------------------------
# PIC24/dsPIC series
#---------------------------------------------------------------------------

ifeq ($(TN_ARCH), $(filter $(TN_ARCH), pic24_dspic_noeds pic24_dspic_eds))
   TN_ARCH_DIR = pic24_dspic

   ifeq ($(TN_COMPILER), $(filter $(TN_COMPILER), xc16))

      ifeq ($(TN_ARCH), pic24_dspic_noeds)
         PIC24_DSPIC_FLAGS = -mcpu=24FJ256GB106
      endif
      ifeq ($(TN_ARCH), pic24_dspic_eds)
         PIC24_DSPIC_FLAGS = -mcpu=24FJ256GB206
      endif

      ifeq ($(TN_COMPILER), xc16)
         CC = xc16-gcc
         AR = xc16-ar
         CFLAGS = $(PIC24_DSPIC_FLAGS) -Os -ffunction-sections -fdata-sections -mlarge-code -mlarge-data -mconst-in-code -msmart-io=1 -msfr-warn=off -Wall -omf=elf
         ASFLAGS = $(CFLAGS)
         TN_COMPILER_VERSION := $(shell $(CC) --version | grep '^xc16-gcc' | sed -r 's/^.*(v[0-9.]+).*/\\1/g')

         BINARY_CMD = $(AR) -r $(BINARY) $(OBJS)
      endif

   endif
endif

ERR_MSG_STD = See comments in the Makefile-single for usage notes




# check if TN_ARCH and TN_COMPILER values were recognized

ifeq ($(TN_ARCH),)
   $(error TN_ARCH is undefined. $(ERR_MSG_STD))
endif

ifeq ($(TN_COMPILER),)
   $(error TN_COMPILER is undefined. $(ERR_MSG_STD))
endif

ifndef TN_ARCH_DIR
   $(error TN_ARCH has invalid value. $(ERR_MSG_STD))
endif

ifndef BINARY_CMD
   $(error TN_COMPILER has invalid value. $(ERR_MSG_STD))
endif





SOURCE_DIR     = src
BIN_DIR        = bin
ARCH_BIN_DIR   = $(BIN_DIR)/$(TN_ARCH)/$(TN_COMPILER)
OBJ_DIR        = $(ARCH_BIN_DIR)/obj

CPPFLAGS = -I${SOURCE_DIR} -I${SOURCE_DIR}/core -I${SOURCE_DIR}/core/internal -I${SOURCE_DIR}/arch

# get just all headers
HEADERS  := $(shell find ${SOURCE_DIR}/ -name "*.h")

# get all core sources plus sources for needed platform
SOURCES  := $(wildcard $(SOURCE_DIR)/core/*.c $(SOURCE_DIR)/arch/$(TN_ARCH_DIR)/*.c $(SOURCE_DIR)/arch/$(TN_ARCH_DIR)/*.S)

# generate list of all object files from source files
OBJS     := $(patsubst %.c,$(OBJ_DIR)/%.o,$(patsubst %.S,$(OBJ_DIR)/%.o,$(notdir $(SOURCES))))

# generate binary library file
BINARY = $(ARCH_BIN_DIR)/tneokernel_$(TN_ARCH)_$(TN_COMPILER).a

# command that creates necessary directories, must be used in rules below
MKDIR_P_CMD = @mkdir -p $(@D)




all: $(BINARY)

#-- for simplicity, every object file just depends on any header file
$(OBJS): $(HEADERS)

$(BINARY): $(OBJS)
	$(MKDIR_P_CMD)
	$(BINARY_CMD)

$(OBJ_DIR)/%.o : $(SOURCE_DIR)/core/%.c
	$(MKDIR_P_CMD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o : $(SOURCE_DIR)/arch/$(TN_ARCH_DIR)/%.c
	$(MKDIR_P_CMD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o : $(SOURCE_DIR)/arch/$(TN_ARCH_DIR)/%.S
	$(MKDIR_P_CMD)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<


