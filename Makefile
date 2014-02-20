BIN = bin
LIB = lib
BUILD = build
SRC = src
FLAGS = -std=c++0x -L$(LIB) -I$(SRC) -O2 -lpthread -lboost_thread \
	-lboost_system -Iinclude
# -DDRAWLINE_BEAUTY_OUTPUT
#-DVERBOSE
#-DDRAWLINE_DEBUG
#debug_FLAGS=-g
#-DDRAWLINE_BEAUTY_OUTPUT

all: $(BIN)/drawline $(BIN)/ne_driver lib

$(BIN)/drawline: $(BUILD)/drawline_driver.o $(BUILD)/drawline.o $(BUILD)/drawline_ds.o
	g++ $(BUILD)/drawline_driver.o \
		$(BUILD)/drawline.o \
		$(BUILD)/drawline_ds.o \
		-o $(BIN)/drawline \
		$(FLAGS) $(debug_FLAGS)

$(BUILD)/drawline_driver.o: $(SRC)/drawline_driver.cpp
	g++ $(SRC)/drawline_driver.cpp \
		-o $(BUILD)/drawline_driver.o \
		-c $(FLAGS) $(debug_FLAGS)

$(BUILD)/drawline.o: $(SRC)/drawline.cpp
	g++ $(SRC)/drawline.cpp \
		-o $(BUILD)/drawline.o \
		-c $(FLAGS) $(debug_FLAGS)

$(BUILD)/drawline_ds.o: $(SRC)/drawline_ds.cpp
	g++ $(SRC)/drawline_ds.cpp \
		-o $(BUILD)/drawline_ds.o \
		-c $(FLAGS) $(debug_FLAGS)

$(BIN)/ne_driver: $(SRC)/ne_driver.cpp $(SRC)/NameFilter.cpp
	g++ $(SRC)/ne_driver.cpp $(SRC)/NameFilter.cpp \
		-o $(BIN)/ne_driver \
		$(FLAGS) -lictlap_core_ne -llitecrf

clean:
	rm $(BIN)/drawline
	rm $(BIN)/ne_driver
	rm $(BUILD)/*.o

lib: $(BUILD)/drawline.o $(BUILD)/drawline_ds.o
	ar crv $(LIB)/libdrawline.a $(BUILD)/drawline.o \
		$(BUILD)/drawline_ds.o	

.PHONEY: clean lib


