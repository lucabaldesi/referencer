

CC = g++ -g -Wall #-pedantic
CFLAGS = `pkg-config --cflags poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6`
LIBS = `pkg-config --libs poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6` -lboost_regex

TARGET = pdfdivine

OBJECTS = main.o BibData.o

all: $(TARGET)
clean:
	rm -f $(TARGET)
#	rm -f $(OBJECTS)
	rm -f *.o

.C.o: ${patsubst %.C, %.h, $<}
	$(CC) $(CFLAGS) -o ${patsubst %.C, %.o, $<} -c $<

pdfdivine: $(OBJECTS)
	$(CC) $(LIBS) -o $(TARGET) $(OBJECTS)

tagwindow: TagWindow.o TagList.o  DocumentList.o 
	$(CC) $(LIBS) -o tagwindow TagWindow.o TagList.o DocumentList.o

