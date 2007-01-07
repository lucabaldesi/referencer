

CC = g++ -g -Wall #-pedantic
CFLAGS = `pkg-config --cflags poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6 libglademm-2.4`
LIBS = `pkg-config --libs poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6 libglademm-2.4` -lboost_regex

TARGET = tagwindow
OBJECTS = TagWindow.o TagList.o  DocumentList.o Document.o BibData.o DocumentProperties.o

.C.o: ${patsubst %.C, %.h, $<}
	$(CC) $(CFLAGS) -o ${patsubst %.C, %.o, $<} -c $<

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LIBS) -o $(TARGET) $(OBJECTS)

clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)
