

CC = g++ -g
CFLAGS = `pkg-config --cflags poppler gtkmm-2.4`
LIBS = `pkg-config --libs poppler gtkmm-2.4` -lboost_regex

TARGET = pdfdivine

OBJECTS = main.o BibData.o

all: $(TARGET)
clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)

.C.o:
	$(CC) $(CFLAGS) -o ${patsubst %.C, %.o, $<} -c $<

pdfdivine: $(OBJECTS)
	$(CC) $(LIBS) -o $(TARGET) $(OBJECTS)

tagwindow: TagWindow.o TagWindow.h TagList.o TagList.h 
	$(CC) $(LIBS) -o tagwindow TagWindow.o TagList.o

