

CC = g++ -g -Wall #-pedantic
CFLAGS = `pkg-config --cflags poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6`
LIBS = `pkg-config --libs poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6` -lboost_regex

TARGET = tagwindow
OBJECTS = TagWindow.o TagList.o  DocumentList.o BibData.o

.C.o: ${patsubst %.C, %.h, $<}
	$(CC) $(CFLAGS) -o ${patsubst %.C, %.o, $<} -c $<

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LIBS) -o $(TARGET) $(OBJECTS)

clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)

pdfdivine: main.o BibData.o
	$(CC) $(LIBS) main.o BibData.o -o pdfdivine
