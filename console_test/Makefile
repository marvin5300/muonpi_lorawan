OBJS	= obj/main.o obj/serial.o
SOURCE	= src/main.cpp src/serial.cpp
INCLUDE_DIR = include
HEADER	=
OUT	= console_test
CC	 = clang++
FLAGS	 = -g -c -Wall -I $(INCLUDE_DIR)
LFLAGS	 =

all: $(OBJS)
	$(CC) $(OBJS) -g -o $(OUT) $(LFLAGS)

obj/main.o: src/main.cpp include/serial.h include/main.h
	mkdir -p obj
	$(CC) $(FLAGS) src/main.cpp -o obj/main.o

obj/serial.o: src/serial.cpp include/serial.h
	mkdir -p obj
	$(CC) $(FLAGS) src/serial.cpp -o obj/serial.o

clean:
	rm -f $(OBJS) $(OUT)
