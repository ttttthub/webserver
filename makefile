BUILD_DIR =./build
OBJS= $(BUILD_DIR)/main.o $(BUILD_DIR)/task.o $(BUILD_DIR)/bloomfilter.o  \
	$(BUILD_DIR)/log.o $(BUILD_DIR)/utc_timer.o 

main:$(OBJS)
	g++  -std=c++11 -o main $(OBJS) -lpthread

$(BUILD_DIR)/main.o: main.cc thread_pool.h  epollServer.h log.h bloomfilter.h
	g++ -c $<  -o $@

$(BUILD_DIR)/log.o: log.cc log.h
	g++ -c $< -o $@
 
$(BUILD_DIR)/utc_timer.o: utc_timer.cc utc_timer.h
	g++ -c $< -o $@

$(BUILD_DIR)/task.o: task.cc task.h log.h bloomfilter.h
	g++ -c $<  -o  $@

$(BUILD_DIR)/bloomfilter.o: bloomfilter.cc bloomfilter.h log.h
	g++ -c $< -o  $@

.PHONY :clean

clean:
	cd $(BUILD_DIR) && rm -f ./*
