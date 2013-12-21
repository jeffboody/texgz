# don't include LOCAL_PATH for submodules

include $(CLEAR_VARS)
LOCAL_MODULE    := texgz
LOCAL_CFLAGS    := -Wall
LOCAL_SRC_FILES := texgz/texgz_tex.c texgz/texgz_log.c
LOCAL_LDLIBS    := -Llibs/armeabi \
                   -llog -lz

ifeq ($(TEXGZ_USE_JPEG),TRUE)
	LOCAL_SRC_FILES := $(LOCAL_SRC_FILES) texgz/texgz_mgm.c texgz/texgz_jpeg.c
endif

ifeq ($(TEXGZ_USE_JPEG),TRUE)
	LOCAL_SHARED_LIBRARIES := libmyjpeg
endif

include $(BUILD_SHARED_LIBRARY)
