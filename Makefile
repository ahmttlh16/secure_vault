# --------------------------------------------------------------------------
# Makefile - CLI Not ve Parola Yoneticisi
# Desteklenen ortamlar: Windows (MinGW/GCC) & Linux (GCC)
# Derleme: make (veya mingw32-make)
# Temizlik: make clean
# --------------------------------------------------------------------------

CC       = gcc
CFLAGS   = -Wall -Wextra -Werror -std=c11 -Iinclude

# Kaynak ve obje dizinleri
SRC_DIR  = src
OBJ_DIR  = obj
SRCS     = $(wildcard $(SRC_DIR)/*.c)
OBJS     = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Platform tespiti ve ayarlari
ifeq ($(OS),Windows_NT)
    TARGET      = vault_manager.exe
    OPENSSL_DIR ?= C:/Program Files/OpenSSL-Win64
    CFLAGS     += -I"$(OPENSSL_DIR)/include"
    LDFLAGS     = -L"$(OPENSSL_DIR)/lib/VC/x64/MD" -lcrypto -lssl -lws2_32 -lcrypt32
    MKDIR_CMD   = if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
    RM_CMD      = if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR) & if exist $(TARGET) del /f /q $(TARGET)
else
    TARGET      = vault_manager
    LDFLAGS     = -lssl -lcrypto
    MKDIR_CMD   = mkdir -p $(OBJ_DIR)
    RM_CMD      = rm -rf $(OBJ_DIR) $(TARGET)
endif

# Debug modunda derleme (opsiyonel: make DEBUG=1)
ifdef DEBUG
CFLAGS  += -g -fsanitize=address,undefined -DDEBUG
LDFLAGS += -fsanitize=address,undefined
endif

# -- Ana hedefler ----------------------------------------------------------

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo.
	@echo   [OK] Derleme basarili: ./$(TARGET)
	@echo.

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@$(MKDIR_CMD)

clean:
	-@$(RM_CMD)
	@echo   [OK] Temizlendi.
