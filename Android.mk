# don't include LOCAL_PATH for submodules

include $(CLEAR_VARS)
LOCAL_MODULE    := texgz
LOCAL_CFLAGS    := -Wall
LOCAL_SRC_FILES := texgz/texgz_tex.c texgz/texgz_log.c

LOCAL_LDLIBS    := -Llibs/armeabi \
                   -llog -lz

include $(BUILD_SHARED_LIBRARY)
