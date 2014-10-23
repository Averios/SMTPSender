CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=SMTPSend.c cencode.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=SMTPSend

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o SMTPSend
