OBJS= wifi.o template.o main.o

EXECUTABLE=inetd 
INCLUDE_DIR=../include

CC=gcc
	  
CFLAGS=-I$(INCLUDE_DIR) -fomit-frame-pointer -Wall -Wstrict-prototypes -pipe -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -g  
LDFLAGS=-fomit-frame-pointer -Wall -Wstrict-prototypes -pipe -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -L../lib -lpthread -lc -lm -lz -lhibus -lefence

#COMPILE=$(CC) $(CFLAGS) -c

all:$(EXECUTABLE)

$(EXECUTABLE): $(OBJS) 
	$(CC) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS) 



COMPILE=$(CC) $(CFLAGS) -c


%o:%c 
	$(COMPILE) -o $@ $< 
	
clean:
	-rm -f $(OBJS) $(EXECUTABLE) *.gdb *.elf* 
