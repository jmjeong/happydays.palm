## Makefile for HappyDays application

VERSION = 2.0
TARGET = happydays
APPNAME = "HappyDays"
APPID = "Jmje"

FILES = lun2sol.c sol2lun.c address.c datebook.c util.c \
		birthdate.c happydays.c memodb.c s2lconvert.c \
		todo.c notify.c
OBJS = obj/lun2sol.o obj/sol2lun.o obj/address.o obj/datebook.o obj/util.o \
		obj/birthdate.o obj/happydays.o obj/memodb.o obj/s2lconvert.o \
		obj/todo.o obj/notify.o \
		obj/happydays-sections.o

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

obj/%.o : %.c
	@echo "Compiling $<..." &&\
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

all: en
	ls -al prc/

all-lang: en ko de th dutch fr sp cz catalan br chi

en: obj/$(TARGET).prc
	mv obj/$(TARGET).prc prc

ko: obj/$(TARGET)-k8.prc obj/$(TARGET)-k11.prc
	mv obj/$(TARGET)-k8.prc obj/$(TARGET)-k11.prc prc
	zip $(TARGET)-$(VERSION)-ko.zip prc/$(TARGET)-k8.prc prc/$(TARGET)-k11.prc

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

obj/happydays-sections.o: obj/happydays-sections.s
	@echo "Compiling happydays-sections.s..." && \
	$(CC) -c obj/happydays-sections.s -o obj/happydays-sections.o

obj/happydays-sections.s obj/happydays-sections.ld : happydays.def
	@echo "Making multi-section info files..." && \
	cd obj && multigen ../happydays.def

obj/$(TARGET).prc: obj/$(TARGET) obj/bin.res
	@echo "Building program file ./obj/happydays.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET).prc happydays.def obj/*.bin obj/$(TARGET)

obj/$(TARGET)-k8.prc: obj/$(TARGET) obj/bin-k8.res
	@echo "Building program file ./obj/happydays-k8.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-k8.prc happydays.def obj/*.bin obj/$(TARGET)

obj/$(TARGET)-k11.prc: obj/$(TARGET) obj/bin-k11.res
	@echo "Building program file ./obj/happydays-k11.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-k11.prc happydays.def obj/*.bin obj/$(TARGET)

$(TARGET)-en.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin.res
	$(BUILDPRC) $(TARGET).prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

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


obj/bin.res: obj/$(TARGET)-en.rcp
	@echo "Compiling resource happydays.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L ENGLISH obj/$(TARGET)-en.rcp obj

obj/bin-k8.res: obj/$(TARGET)-ko.rcp
	@echo "Compiling resource happydays-ko.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L KOREAN -Fkt obj/$(TARGET)-ko.rcp obj

obj/bin-k11.res: obj/$(TARGET)-ko.rcp
	@echo "Compiling resource happydays-ko.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L KOREAN -FKm obj/$(TARGET)-ko.rcp obj

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

obj/$(TARGET)-en.rcp: $(TARGET).rcp translate/english.msg translate/hdr.msg
	@echo "Generating happydays-en.rcp file..." && \
	cat translate/hdr.msg translate/english.msg $(TARGET).rcp \
			> obj/$(TARGET)-en.rcp

obj/$(TARGET)-ko.rcp: $(TARGET).rcp translate/korean.msg translate/hdr.msg
	@echo "Generating happydays-ko.rcp file..." && \
	cat translate/hdr.msg translate/korean.msg $(TARGET).rcp \
			> obj/$(TARGET)-ko.rcp

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

obj/$(TARGET): $(OBJS) obj/happydays-sections.ld
	@echo "Linking..." && \
	$(CC) $(CFLAGS) $(OBJS) -o obj/$(TARGET) obj/happydays-sections.ld
	m68k-palmos-objdump --section-headers obj/happydays

send: $(TARGET).prc
	$(PILOTXFER) -i $(TARGET).prc

depend:
	$(CC) -MM $(FILES) > .depend

tags:
	etags $(FILES) *.h

clean:
	@echo "Deleting obj files..." &&\
	cd obj && \rm -f *

veryclean: clean
	-rm -f $(TARGET)*.prc pilot.ram pilot.scratch

html: 
	@echo "Generating html file" && \
	cd manual && wmk

zip: prc/$(TARGET).prc html
	-@echo "Making distribution file(happydays-$(VERSION).zip) ..." &&\
	\rm -rf dist && mkdir dist && mkdir dist/img && \
	cp prc/$(TARGET).prc dist && cp manual/*.html dist  && \
	cp manual/img/*.gif dist/img && cp manual/img/*.png dist/img && \
	cd dist && \
	zip -r ../happydays-$(VERSION).zip *  && \rm -rf dist

-include .depend
