## Makefile for HappyDays application

TARGET = happydays
APPNAME = "HappyDays"
APPID = "Jmje"

FILES = happydays.c lun2sol.c sol2lun.c address.c datebook.c util.c \
		birthdate.c memodb.c
OBJS = happydays.o lun2sol.o sol2lun.o address.o datebook.o util.o \
		birthdate.o memodb.o

CC = m68k-palmos-gcc

CFLAGS = -Wall -O2
#CFLAGS = -Wall -g -O2

PILRC = ../pilrc.new/pilrc
OBJRES = m68k-palmos-obj-res
NM = m68k-palmos-nm
BUILDPRC = build-prc
PILOTXFER = pilot-xfer

all: alwaysres $(TARGET).prc $(TARGET)-kt.prc $(TARGET)-km.prc

korean: alwaysres $(TARGET)-k.prc

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

$(TARGET).prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin.res
	$(BUILDPRC) $(TARGET).prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-kt.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-kt.res
	$(BUILDPRC) $(TARGET)-kt.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-km.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-km.res
	$(BUILDPRC) $(TARGET)-km.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc
code0000.$(TARGET).grc: $(TARGET)
	$(OBJRES) $(TARGET)

code0001.$(TARGET).grc: code0000.$(TARGET).grc

data0000.$(TARGET).grc: code0000.$(TARGET).grc

pref0000.$(TARGET).grc: code0000.$(TARGET).grc

rloc0000.$(TARGET).grc: code0000.$(TARGET).grc

bin.res: $(TARGET).rcp $(TARGET).pbitm
	$(PILRC) -L ENGLISH $(TARGET).rcp .
	touch bin.res

bin-kt.res: $(TARGET).rcp $(TARGET).pbitm
	$(PILRC) -L KOREAN -Fkt $(TARGET).rcp .
	touch bin.res

bin-km.res: $(TARGET).rcp $(TARGET).pbitm
	$(PILRC) -L KOREAN -Fkm $(TARGET).rcp .
	touch bin.res

alwaysres:
	rm -f bin.res

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)
	! $(NM) -u $(TARGET) | grep .

send: $(TARGET).prc
	$(PILOTXFER) -i $(TARGET).prc

depend:
	$(CC) -MM $(FILES) > .depend

tags:
	etags $(FILES) *.h

clean:
	-rm -f *.[oa] $(TARGET) *.bin bin.res *.grc Makefile.bak

veryclean: clean
	-rm -f $(TARGET).prc pilot.ram pilot.scratch

save:
	zip $@ *.c Makefile *.rcp README COPYING *.h *.pbitm pilrc.diff

-include .depend
