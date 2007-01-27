

CC = g++ -g -Wall #-pedantic
CFLAGS = `pkg-config --cflags poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6 libglademm-2.4 gconfmm-2.6` -DDATADIR="\"$(DATADIR)\""
LIBS = `pkg-config --libs poppler gtkmm-2.4 libgnomeuimm-2.6 gnome-vfsmm-2.6 libglademm-2.4 gconfmm-2.6` -lboost_regex -Llibbibutils -lbibutils

TARGET = referencer
OBJECTS = TagWindow.o TagList.o  DocumentList.o Document.o BibData.o DocumentProperties.o Preferences.o Utility.o BibUtils.o

PREFIX=/usr/local
DATADIR=$(PREFIX)/share/$(TARGET)
BINDIR=$(PREFIX)/bin

all: $(TARGET)

.C.o: ${patsubst %.C, %.h, $<}
	$(CC) $(CFLAGS) -o ${patsubst %.C, %.o, $<} -c $<

libbibutils/libbibutils.a:
	$(MAKE) -C libbibutils libbibutils.a
	# Remove the .o files in favour of the .a file
	$(MAKE) -C libbibutils clean

$(TARGET): $(OBJECTS) libbibutils/libbibutils.a
	$(CC) -o $(TARGET) $(OBJECTS) $(LIBS) 

install: all
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(DATADIR)
	install -s $(TARGET) $(DESTDIR)$(BINDIR)
	install -m 0644 *.glade $(DESTDIR)$(DATADIR)
	install -m 0644 thumbnail_frame.png $(DESTDIR)$(DATADIR)

uninstall:
	rm -f  $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -rf $(DESTDIR)$(DATADIR)

clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)
	$(MAKE) -C libbibutils realclean
