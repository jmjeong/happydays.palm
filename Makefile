## Makefile for HappyDays application

TARGET = happydays
APPNAME = "HappyDays"
APPID = "Jmje"

FILES = happydays.c lun2sol.c sol2lun.c address.c datebook.c util.c \
		birthdate.c memodb.c s2lconvert.c todo.c
OBJS = happydays.o lun2sol.o sol2lun.o address.o datebook.o util.o \
		birthdate.o memodb.o s2lconvert.o todo.o

CC = m68k-palmos-gcc

CFLAGS = -Wall -O2 -D$(LANG)
#CFLAGS = -Wall -g -O2

PILRC = pilrc
OBJRES = m68k-palmos-obj-res
NM = m68k-palmos-nm
BUILDPRC = build-prc
PILOTXFER = pilot-xfer
LANG = ENGLISH

all: en

all-lang: en ko de th

en: alwaysmake $(TARGET).prc

ko: alwaysmake $(TARGET)-kt.prc $(TARGET)-km.prc

de: alwaysmake $(TARGET)-de.prc 

th: alwaysmake $(TARGET)-th.prc

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

$(TARGET)-de.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-de.res
	$(BUILDPRC) $(TARGET)-de.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-th.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-th.res
	$(BUILDPRC) $(TARGET)-th.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

code0000.$(TARGET)-de.grc: $(TARGET)-de
	$(OBJRES) $(TARGET)

code0000.$(TARGET)-th.grc: $(TARGET)-th
	$(OBJRES) $(TARGET)

code0000.$(TARGET)-en.grc: $(TARGET)-en
	$(OBJRES) $(TARGET)

code0000.$(TARGET).grc: $(TARGET)
	$(OBJRES) $(TARGET)

code0001.$(TARGET).grc: code0000.$(TARGET).grc

data0000.$(TARGET).grc: code0000.$(TARGET).grc

pref0000.$(TARGET).grc: code0000.$(TARGET).grc

rloc0000.$(TARGET).grc: code0000.$(TARGET).grc

bin.res: $(TARGET)-en.rcp 
	$(PILRC) -L ENGLISH $(TARGET)-en.rcp .

bin-kt.res: $(TARGET)-ko.rcp 
	$(PILRC) -L KOREAN -Fkt $(TARGET)-ko.rcp .

bin-km.res: $(TARGET)-ko.rcp
	$(PILRC) -L KOREAN -Fkm $(TARGET)-ko.rcp .

bin-de.res: $(TARGET)-de.rcp 
	$(PILRC) -L GERMAN $(TARGET)-de.rcp .

bin-th.res: $(TARGET)-th.rcp 
	$(PILRC) -L THAI $(TARGET)-th.rcp .

$(TARGET)-en.rcp: $(TARGET).rcp english.msg hdr.msg
	cat hdr.msg english.msg $(TARGET).rcp > $(TARGET)-en.rcp

$(TARGET)-ko.rcp: $(TARGET).rcp korean.msg hdr.msg
	cat hdr.msg korean.msg $(TARGET).rcp > $(TARGET)-ko.rcp
	
$(TARGET)-de.rcp: $(TARGET).rcp german.msg hdr.msg
	cat hdr.msg german.msg $(TARGET).rcp > $(TARGET)-de.rcp

$(TARGET)-th.rcp: $(TARGET).rcp thai.msg hdr.msg
	cat hdr.msg thai.msg $(TARGET).rcp > $(TARGET)-th.rcp



alwaysmake:
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
	-rm -f $(TARGET)*.prc pilot.ram pilot.scratch

save:
	zip $@ *.c Makefile *.rcp README COPYING *.h *.pbitm pilrc.diff

-include .depend
