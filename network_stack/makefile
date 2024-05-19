TARGET=$(TARGET_PATH)/stack
TARGET_PATH=build
SRC_PATH=src
CXX= g++
SRC=$(wildcard $(SRC_PATH)/*.cpp)
OBJS=$(patsubst $(SRC_PATH)/%.cpp,$(TARGET_PATH)/%.o, $(SRC))
LIBS= -lpcap
CFLAGS=-g
$(TARGET_PATH)/%.o:$(SRC_PATH)/%.cpp
	$(CXX)  $(CFLAGS) -c $^ -o $@
all:depend $(TARGET)
depend:
	$(CXX) -MM $(SRC) > $(TARGET_PATH)/depend.d
-include $(TARGET_PATH)/depend.d
$(TARGET):$(OBJS)
	$(CXX) $^ $(LIBS) $(CFLAGS) -o $@ 
clean:
	rm $(TARGET_PATH)/depend.d $(OBJS) $(TARGET)
