LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := crashlog
LOCAL_SRC_FILES := \
	crashlogjni.cpp \
	../../../sig_segv.c \
	../../../backtrace/backtrace.c \
	../../../backtrace/backtrace-arm.c \
	../../../backtrace/symbol_table.c \
	../../../backtrace/backtrace-helper.c \
	../../../backtrace/map_info.c \
	../../../backtrace/cp-demangle.c


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../../../ \
	$(LOCAL_PATH)/../../../backtrace \


LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)

