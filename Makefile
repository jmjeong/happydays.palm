## Makefile for HappyDays application

VERSION = 2.0
TARGET = happydays
APPNAME = "HappyDays"
APPID = "Jmje"

FILES = lun2sol.c sol2lun.c address.c datebook.c util.c \
		birthdate.c happydays.c memodb.c s2lconvert.c todo.c \
		notify.c
OBJS = lun2sol.o sol2lun.o address.o datebook.o util.o \
		birthdate.o happydays.o memodb.o s2lconvert.o todo.o \
		notify.o \
		happydays-sections.o

CC = m68k-palmos-gcc

CFLAGS = -Wall -O2 
#CFLAGS = -Wall -g -O2

PILRC = pilrc
OBJRES = m68k-palmos-obj-res
NM = m68k-palmos-nm
BUILDPRC = build-prc
PILOTXFER = pilot-xfer
ZIP = zip
LANG = ENGLISH

all: en
	ls -al happydays*.prc

all-lang: en ko de th dutch fr sp cz catalan br chi

en: $(TARGET).prc

ko: $(TARGET)-kt.prc $(TARGET)-km.prc
	zip $(TARGET)-$(VERSION)-ko.zip $(TARGET)-kt.prc $(TARGET)-km.prc

de: $(TARGET)-de.prc 
	zip $(TARGET)-$(VERSION)-de.zip $(TARGET)-de.prc

th: $(TARGET)-th.prc
	zip $(TARGET)-$(VERSION)-th.zip $(TARGET)-th.prc

chi: $(TARGET)-chi-big5.prc $(TARGET)-chi-gb.prc
	zip $(TARGET)-$(VERSION)-ch.zip $(TARGET)-chi-big5.prc $(TARGET)-chi-gb.prc

dutch: $(TARGET)-dutch.prc
	zip $(TARGET)-$(VERSION)-dutch.zip $(TARGET)-dutch.prc

fr: $(TARGET)-fr.prc
	zip $(TARGET)-$(VERSION)-fr.zip $(TARGET)-fr.prc

sp: $(TARGET)-sp.prc
	zip $(TARGET)-$(VERSION)-sp.zip $(TARGET)-sp.prc

cz: $(TARGET)-cz.prc
	zip $(TARGET)-$(VERSION)-cz.zip $(TARGET)-cz.prc

catalan: $(TARGET)-catalan.prc
	zip $(TARGET)-$(VERSION)-catalan.zip $(TARGET)-catalan.prc

br: $(TARGET)-br.prc
	zip $(TARGET)-$(VERSION)-br.zip $(TARGET)-br.prc

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

happydays-sections.o: happydays-sections.s
	$(CC) -c happydays-sections.s

happydays-sections.s happydays-sections.ld : happydays.def
	multigen happydays.def

$(TARGET).prc: $(TARGET) bin.res
	$(BUILDPRC) -o $(TARGET).prc happydays.def *.bin $(TARGET)

$(TARGET)-en.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin.res
	$(BUILDPRC) $(TARGET).prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-kt.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-kt.res
	$(BUILDPRC) $(TARGET)-kt.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-km.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-km.res
	$(BUILDPRC) $(TARGET)-km.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-de.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-de.res
	$(BUILDPRC) $(TARGET)-de.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-th.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-th.res
	$(BUILDPRC) $(TARGET)-th.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-chi-big5.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-chi-big5.res
	$(BUILDPRC) $(TARGET)-chi-big5.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-chi-gb.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-chi-gb.res
	$(BUILDPRC) $(TARGET)-chi-gb.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-dutch.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-dutch.res
	$(BUILDPRC) $(TARGET)-dutch.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-fr.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-fr.res
	$(BUILDPRC) $(TARGET)-fr.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-sp.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-sp.res
	$(BUILDPRC) $(TARGET)-sp.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-cz.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-cz.res
	$(BUILDPRC) $(TARGET)-cz.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-catalan.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-catalan.res
	$(BUILDPRC) $(TARGET)-catalan.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-br.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-br.res
	$(BUILDPRC) $(TARGET)-br.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

code0000.$(TARGET).grc: $(TARGET)
	$(OBJRES) $(TARGET)

code0001.$(TARGET).grc: code0000.$(TARGET).grc

data0000.$(TARGET).grc: code0000.$(TARGET).grc

pref0000.$(TARGET).grc: code0000.$(TARGET).grc

rloc0000.$(TARGET).grc: code0000.$(TARGET).grc

bin.res: $(TARGET)-en.rcp
	rm -f *.bin
	$(PILRC) -q -L ENGLISH $(TARGET)-en.rcp .

bin-kt.res: $(TARGET)-ko.rcp
	rm -f *.bin
	$(PILRC) -L KOREAN -Fkt $(TARGET)-ko.rcp .

bin-km.res: $(TARGET)-ko.rcp
	rm -f *.bin
	$(PILRC) -L KOREAN -Fkm $(TARGET)-ko.rcp .

bin-de.res: $(TARGET)-de.rcp
	rm -f *.bin
	$(PILRC) -L GERMAN $(TARGET)-de.rcp .

bin-th.res: $(TARGET)-th.rcp
	rm -f *.bin
	$(PILRC) -L THAI $(TARGET)-th.rcp .

bin-chi-big5.res: $(TARGET)-chi-big5.rcp
	rm -f *.bin
	$(PILRC) -L CHINESE -F5 $(TARGET)-chi-big5.rcp .

bin-chi-gb.res: $(TARGET)-chi-gb.rcp
	rm -f *.bin
	$(PILRC) -L CHINESE -F5 $(TARGET)-chi-gb.rcp .

bin-dutch.res: $(TARGET)-dutch.rcp
	rm -f *.bin
	$(PILRC) -L DUTCH $(TARGET)-dutch.rcp .

bin-fr.res: $(TARGET)-fr.rcp
	rm -f *.bin
	$(PILRC) -L FRENCH $(TARGET)-fr.rcp .

bin-sp.res: $(TARGET)-sp.rcp
	rm -f *.bin
	$(PILRC) -L SPANISH $(TARGET)-sp.rcp .

bin-cz.res: $(TARGET)-cz.rcp
	rm -f *.bin
	$(PILRC) -L CZECH $(TARGET)-cz.rcp .

bin-catalan.res: $(TARGET)-catalan.rcp
	rm -f *.bin
	$(PILRC) -L CATALAN $(TARGET)-catalan.rcp .

bin-br.res: $(TARGET)-br.rcp
	rm -f *.bin
	$(PILRC) -L BRAZILIAN $(TARGET)-br.rcp .

$(TARGET)-en.rcp: $(TARGET).rcp translate/english.msg translate/hdr.msg
	cat translate/hdr.msg translate/english.msg $(TARGET).rcp > $(TARGET)-en.rcp

$(TARGET)-ko.rcp: $(TARGET).rcp korean.msg hdr.msg
	cat hdr.msg korean.msg $(TARGET).rcp > $(TARGET)-ko.rcp

$(TARGET)-de.rcp: $(TARGET).rcp german.msg hdr.msg
	cat hdr.msg german.msg $(TARGET).rcp > $(TARGET)-de.rcp

$(TARGET)-th.rcp: $(TARGET).rcp thai.msg hdr.msg
	cat hdr.msg thai.msg $(TARGET).rcp > $(TARGET)-th.rcp

$(TARGET)-chi-big5.rcp: $(TARGET).rcp chinese.msg hdr.msg
	cat hdr.msg chinese.msg $(TARGET).rcp > $(TARGET)-chi-big5.rcp

$(TARGET)-chi-gb.rcp: $(TARGET).rcp schinese.msg hdr.msg
	cat hdr.msg schinese.msg $(TARGET).rcp > $(TARGET)-chi-gb.rcp

$(TARGET)-dutch.rcp: $(TARGET).rcp dutch.msg hdr.msg
	cat hdr.msg dutch.msg $(TARGET).rcp > $(TARGET)-dutch.rcp

$(TARGET)-fr.rcp: $(TARGET).rcp french.msg hdr.msg
	cat hdr.msg french.msg $(TARGET).rcp > $(TARGET)-fr.rcp

$(TARGET)-sp.rcp: $(TARGET).rcp spanish.msg hdr.msg
	cat hdr.msg spanish.msg $(TARGET).rcp > $(TARGET)-sp.rcp

$(TARGET)-cz.rcp: $(TARGET).rcp czech.msg hdr.msg
	cat hdr.msg czech.msg $(TARGET).rcp > $(TARGET)-cz.rcp

$(TARGET)-catalan.rcp: $(TARGET).rcp catalan.msg hdr.msg
	cat hdr.msg catalan.msg $(TARGET).rcp > $(TARGET)-catalan.rcp

$(TARGET)-br.rcp: $(TARGET).rcp brazilian.msg hdr.msg
	cat hdr.msg brazilian.msg $(TARGET).rcp > $(TARGET)-br.rcp

$(TARGET): $(OBJS) happydays-sections.ld
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS) happydays-sections.ld
	m68k-palmos-objdump --section-headers happydays
#	! $(NM) -u $(TARGET) | grep .

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
	zip $@ *.c Makefile *.rcp README COPYING *.h *.pbitm 

-include .depend
