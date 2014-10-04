CXXFLAGS := -Wall -pedantic -ansi -g3 -Inetwork
CXXFLAGS += -O3

NET_OBJS := utils.o errors.o routing.o graph.o network.o
SRV_OBJS := main.o serial.o tolhnet.o dispatcher.o netshell.o applogic.o

OBJS := $(addprefix daemon/, $(SRV_OBJS) $(NET_OBJS))
DEPS := $(OBJS:.o=.d)

TARGETS := tolhnet

.PHONY: clean all

all : $(TARGETS)

tolhnet : $(addprefix daemon/, $(SRV_OBJS) $(NET_OBJS))
	$(CXX) -o $@ $^

daemon/%.o : daemon/%.cpp
	$(CXX) -MMD $(CXXFLAGS) -DNWK_DEBUG_DUMP -o $@ -c $<

daemon/%.o : network/%.cpp
	$(CXX) -MMD $(CXXFLAGS) -DNWK_DEBUG_DUMP -o $@ -c $<

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
