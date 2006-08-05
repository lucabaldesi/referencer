

CC = g++
CFLAGS = `pkg-config --cflags poppler-glib gtk+-2.0`
LIBS = `pkg-config --libs poppler-glib gtk+-2.0`

TARGET = pdfdivine

OBJECTS = main.o

all: $(TARGET)
clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)

main.o: main.C
	$(CC) $(CFLAGS) -c main.C

pdfdivine: $(OBJECTS)
	$(CC) $(LIBS) -o $(TARGET) $(OBJECTS)
