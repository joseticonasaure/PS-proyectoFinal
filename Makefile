# Variables del compilador y banderas
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -Iinclude -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE
LDFLAGS = -pthread

# Directorios de la estructura del proyecto
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Nombre del ejecutable final
TARGET = $(BIN_DIR)/admin_sys

# Recopilacion de archivos fuente y objetos
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

ifeq ($(OS),Windows_NT)
MKDIR = mkdir
RMDIR = rmdir /s /q
else
MKDIR = mkdir -p
RMDIR = rm -rf
endif

# Reglas principales
.PHONY: all clean directories

all: directories $(TARGET)

# Creacion dinamica de directorios de compilacion
directories:
	-@$(MKDIR) $(OBJ_DIR)
	-@$(MKDIR) $(BIN_DIR)

# Enlazado final del ejecutable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compilacion modular de objetos
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpieza de archivos generados
clean:
	-@$(RMDIR) $(OBJ_DIR)
	-@$(RMDIR) $(BIN_DIR)