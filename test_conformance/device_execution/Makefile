ifdef BUILD_WITH_ATF
ATF = -framework ATF
USE_ATF = -DUSE_ATF
endif

SRCS = main.c \
	device_info.cpp \
	device_queue.cpp \
	enqueue_block.cpp \
	enqueue_flags.cpp \
	enqueue_multi_queue.cpp \
	enqueue_ndrange.cpp \
	enqueue_wg_size.cpp \
	execute_block.cpp \
	host_multi_queue.cpp \
	host_queue_order.cpp \
	nested_blocks.cpp \
	utils.cpp \
	../../test_common/harness/errorHelpers.c \
	../../test_common/harness/testHarness.c \
	../../test_common/harness/kernelHelpers.c \
	../../test_common/harness/typeWrappers.cpp \
	../../test_common/harness/mt19937.c \
		  
DEFINES = DONT_TEST_GARBAGE_POINTERS

SOURCES = $(abspath $(SRCS))
LIBPATH += -L/System/Library/Frameworks/OpenCL.framework/Libraries
LIBPATH += -L.
HEADERS = 
TARGET = test_device_execution
INCLUDE = 
COMPILERFLAGS = -c -Wall -g -Wshorten-64-to-32
CC = c++
CFLAGS = $(COMPILERFLAGS) ${RC_CFLAGS} ${USE_ATF} $(DEFINES:%=-D%) $(INCLUDE)
CXXFLAGS = $(COMPILERFLAGS) ${RC_CFLAGS} ${USE_ATF} $(DEFINES:%=-D%) $(INCLUDE)
LIBRARIES = -framework OpenCL -framework OpenGL -framework GLUT -framework AppKit ${ATF}

OBJECTS := ${SOURCES:.c=.o}
OBJECTS := ${OBJECTS:.cpp=.o}

TARGETOBJECT =
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(RC_CFLAGS) $(OBJECTS) -o $@ $(LIBPATH) $(LIBRARIES)

clean:
	rm -f $(TARGET) $(OBJECTS)

.DEFAULT:
	@echo The target \"$@\" does not exist in Makefile.
