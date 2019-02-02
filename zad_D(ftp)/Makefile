TARGET = main
NAME = ftp_serv
LIB = ftp_utils
FLAGS = -Wall -lpthread
COMPILER = gcc

default: all

all: $(TARGET).c
	$(COMPILER) -o $(NAME) $(FLAGS) $(TARGET).c $(LIB).c

debug: $(TARGET).c
	$(COMPILER) -o $(NAME) -g $(FLAGS) $(TARGET).c $(LIB).c

clean: ftp_serv
	rm ftp_serv
