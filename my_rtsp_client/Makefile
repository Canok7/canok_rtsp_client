#there was a bug, if you change any *.h , please make clean firtly

CROSS_COMPILE =
CC = $(CROSS_COMPILE)gcc
CXX= $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip


CC_FLAG= -g
#CC_FLAG +=-DEN_MEMWATCH

INCLUDE_PATH += -I.


LINK_PATH += -L. 
LFLAGS += -lpthread 


RELEASE_DIR := Release
TARGET := test_rtsp_client
RELEASE_TARGET := $(RELEASE_DIR)/$(TARGET)

#SRCS    :=  $(wildcard *.c)
SRCS := $(wildcard *.cpp)
OBJS :=  $(SRCS:%.cpp=%.o)
.PHONY : clean
%/.:
	mkdir -p $*

release:$(RELEASE_DIR)/. $(RELEASE_TARGET)
	cp ${RELEASE_TARGET} ./
$(RELEASE_TARGET): $(addprefix $(RELEASE_DIR)/,$(OBJS))
	$(CXX) -o $@ $^  $(LINK_PATH) $(LFLAGS)
	echo "release"

	
$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INCLUDE_PATH) -c $(CC_FLAG) $^ -o $@

clean:
	rm -f $(TARGET)
	rm -rf $(RELEASE_DIR)
