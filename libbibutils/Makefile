#CC = gcc -Wall 
RANLIB = ranlib

BIB_OBJS = bbl.o fields.o lists.o name.o title.o reftypes.o

SIMPLE_OBJS   = is_ws.o strsearch.o charsets.o
NEWSTR_OBJS   = newstr.o newstr_conv.o entities.o latex.o utf8.o
CONTAIN_OBJS  = fields.o lists.o xml.o xml_encoding.o
INPUT_OBJS    = bibtexin.o bibtypes.o bibtexout.o \
		copacin.o copactypes.o \
		endin.o endtypes.o endout.o \
		isiin.o isitypes.o \
		medin.o \
		modsin.o \
		risin.o ristypes.o risout.o \
		reftypes.o
OUTPUT_OBJS   = modsout.o
BIBUTILS_OBJS = $(SIMPLE_OBJS) $(NEWSTR_OBJS) $(CONTAIN_OBJS) \
	$(INPUT_OBJS) $(OUTPUT_OBJS) name.o title.o bibl.o serialno.o bibutils.o

BIBPROGS_OBJS = bibprogs.o compiledate.o

all:  libbibutils.a libbibprogs.a

libbibutils.a: $(BIBUTILS_OBJS)
	ar -cr libbibutils.a $(BIBUTILS_OBJS)
	$(RANLIB) libbibutils.a

libbibprogs.a: $(BIBPROGS_OBJS)
	ar -cr libbibprogs.a $(BIBPROGS_OBJS)
	$(RANLIB) libbibprogs.a

compiledate.o : compiledate.c
	$(CC) -DVERSION="\"$(VERSION)\"" -DDATE="\"$(DATE)\"" -c -o compiledate.o compiledate.c

clean: 
	/bin/rm -f *.o core

realclean:
	/bin/rm -f *.o *.a core

test:
