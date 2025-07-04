LOCAL_PATH := $(call my-dir)

ROOT_DIR   := $(LOCAL_PATH)/../../..
CORE_DIR   := $(ROOT_DIR)/platforms/libretro
SOURCE_DIR := $(ROOT_DIR)/src
INCLUDES   := -I$(CORE_DIR) -I$(SOURCE_DIR) -I$(DEPS_DIR)/miniz -I$(DEPS_DIR)/libchdr/include -I$(DEPS_DIR)/zstd -I$(DEPS_DIR)/lzma/include

include $(CORE_DIR)/Makefile.common

COREFLAGS := -DHAVE_STDINT_H -DHAVE_INTTYPES_H -D__LIBRETRO__ -DGG_DISABLE_DISASSEMBLER -DZ7_ST $(INCLUDES)

GIT_VERSION ?= " $(shell git describe --abbrev=7 --dirty --always --tags || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
   COREFLAGS += -DEMULATOR_BUILD=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_CXXFLAGS  := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
LOCAL_LDLIBS 	:= -llog
include $(BUILD_SHARED_LIBRARY)
