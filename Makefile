BIN_DIR=bin

#TODO: autogenerate this makefile from some kind of list, like:


all:  \
	$(BIN_DIR)/tneokernel_cortex_m4_arm-none-eabi-gcc.a \
	$(BIN_DIR)/tneokernel_cortex_m4f_arm-none-eabi-gcc.a \
	$(BIN_DIR)/tneokernel_pic32mx_xc32.a

$(BIN_DIR)/tneokernel_cortex_m4_arm-none-eabi-gcc.a :
	make -f Makefile-single TN_ARCH=cortex_m4 TN_COMPILER=arm-none-eabi-gcc

$(BIN_DIR)/tneokernel_cortex_m4f_arm-none-eabi-gcc.a :
	make -f Makefile-single TN_ARCH=cortex_m4f TN_COMPILER=arm-none-eabi-gcc

$(BIN_DIR)/tneokernel_pic32mx_xc32.a :
	make -f Makefile-single TN_ARCH=pic32mx TN_COMPILER=xc32



