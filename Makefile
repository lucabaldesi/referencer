

CC = g++ -g -Wall #-pedantic
CFLAGS = `pkg-config --cflags poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6 libglademm-2.4 gconfmm-2.6` -DDATADIR="\"$(DATADIR)\""
LIBS = libbibutils/*.o `pkg-config --libs poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6 libglademm-2.4 gconfmm-2.6` -lboost_regex
#LIBS = libbibutils/libbibutils.a libbibutils/libbibprogs.a `pkg-config --libs poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6 libglademm-2.4 gconfmm-2.6` -lboost_regex

TARGET = referencer
OBJECTS = TagWindow.o TagList.o  DocumentList.o Document.o BibData.o DocumentProperties.o Preferences.o Utility.o BibUtils.o

PREFIX=/usr/local
DATADIR=$(PREFIX)/share/$(TARGET)
BINDIR=$(PREFIX)/bin

.C.o: ${patsubst %.C, %.h, $<}
	$(CC) $(CFLAGS) -o ${patsubst %.C, %.o, $<} -c $<

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LIBS) -o $(TARGET) $(OBJECTS)

install:
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(DATADIR)
	install -s $(TARGET) $(DESTDIR)$(BINDIR)
	install -m 0644 *.glade $(DESTDIR)$(DATADIR)



clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)
