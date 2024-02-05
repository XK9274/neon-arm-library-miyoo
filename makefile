CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
AR = $(CROSS_COMPILE)ar
CFLAGS = -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7ve -I$(INC_DIR)
CXXFLAGS = -s -O3 -fPIC
LDFLAGS = -Wl,-rpath-link=/opt/miyoomini-toolchain/arm-linux-gnueabihf/libc/usr/lib -L./lib

SRC_DIR = ./src
OBJ_DIR = ./obj
INC_DIR = ./include
LIB_DIR = ./lib

STATIC_TARGET = $(LIB_DIR)/libneonarmmiyoo.a
SHARED_TARGET = $(LIB_DIR)/libneonarmmiyoo.so

C_SRC = $(wildcard $(SRC_DIR)/*.c)
ASM_SRC = $(wildcard $(SRC_DIR)/*.S)
OBJ = $(C_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) $(ASM_SRC:$(SRC_DIR)/%.S=$(OBJ_DIR)/%.o)

all: $(STATIC_TARGET) $(SHARED_TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S | $(OBJ_DIR)
	$(AS) -I$(INC_DIR) -c $< -o $@

$(STATIC_TARGET): $(OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $(OBJ)

$(SHARED_TARGET): $(OBJ) | $(LIB_DIR)
	$(CC) $(CFLAGS) $(CXXFLAGS) -shared -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ_DIR)/*.o $(LIB_DIR)/*.a $(LIB_DIR)/*.so
