

CC = g++
CFLAGS = `pkg-config --cflags poppler gtkmm-2.4`
LIBS = `pkg-config --libs poppler gtkmm-2.4`

TARGET = pdfdivine

OBJECTS = main.o BibData.o

all: $(TARGET)
clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)

main.o: main.C BibData.h
	$(CC) $(CFLAGS) -c main.C

BibData.o: BibData.C BibData.h
	$(CC) $(CFLAGS) -c BibData.C

pdfdivine: $(OBJECTS)
	$(CC) $(LIBS) -o $(TARGET) $(OBJECTS)
