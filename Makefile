## Makefile for HappyDays application

VERSION = 2.05
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
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<
#	@echo "Compiling $<..." &&\

all: en
	ls -al prc/

all-lang: en ko de th dutch fr sp cz catalan br chi

PORTUGUESE_BR:
	$(CC) -c -DPORTUGUESE_BR $(CFLAGS) $(CPPFLAGS) \
			-o obj/happydays.o happydays.c

GERMAN:
	$(CC) -c -DGERMAN $(CFLAGS) $(CPPFLAGS) -o obj/happydays.o happydays.c

ENGLISH:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o obj/happydays.o happydays.c

en: ENGLISH obj/$(TARGET).prc
	mv obj/$(TARGET).prc prc

ko: ENGLISH obj/$(TARGET)-k8.prc obj/$(TARGET)-k11.prc
	mv obj/$(TARGET)-k8.prc obj/$(TARGET)-k11.prc prc
	zip $(TARGET)-$(VERSION)-ko.zip prc/$(TARGET)-k8.prc prc/$(TARGET)-k11.prc

sp: ENGLISH obj/$(TARGET)-sp.prc
	zip $(TARGET)-$(VERSION)-sp.zip obj/$(TARGET)-sp.prc

chi: ENGLISH obj/$(TARGET)-chi-big5.prc obj/$(TARGET)-chi-gb.prc
	zip $(TARGET)-$(VERSION)-ch.zip obj/$(TARGET)-chi-big5.prc \
			obj/$(TARGET)-chi-gb.prc

de: GERMAN obj/$(TARGET)-de.prc 
	zip $(TARGET)-$(VERSION)-de.zip obj/$(TARGET)-de.prc

br: PORTUGUESE_BR obj/$(TARGET)-br.prc
	zip $(TARGET)-$(VERSION)-br.zip obj/$(TARGET)-br.prc

th: $(TARGET)-th.prc
	zip $(TARGET)-$(VERSION)-th.zip $(TARGET)-th.prc


dutch: $(TARGET)-dutch.prc
	zip $(TARGET)-$(VERSION)-dutch.zip $(TARGET)-dutch.prc

fr: $(TARGET)-fr.prc
	zip $(TARGET)-$(VERSION)-fr.zip $(TARGET)-fr.prc

cz: $(TARGET)-cz.prc
	zip $(TARGET)-$(VERSION)-cz.zip $(TARGET)-cz.prc

catalan: $(TARGET)-catalan.prc
	zip $(TARGET)-$(VERSION)-catalan.zip $(TARGET)-catalan.prc

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

obj/$(TARGET)-sp.prc: obj/$(TARGET) obj/bin-sp.res
	@echo "Building program file ./obj/happydays-sp.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-sp.prc happydays.def obj/*.bin obj/$(TARGET)

obj/$(TARGET)-chi-big5.prc: obj/$(TARGET) obj/bin-chi-big5.res
	@echo "Building program file ./obj/happydays-chi-big5.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-chi-big5.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-chi-gb.prc: obj/$(TARGET) obj/bin-chi-gb.res
	@echo "Building program file ./obj/happydays-chi-gb.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-chi-gb.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-de.prc: obj/$(TARGET) obj/bin-de.res
	@echo "Building program file ./obj/happydays-de.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-de.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-br.prc: obj/$(TARGET) obj/bin-br.res
	@echo "Building program file ./obj/happydays-br.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-br.prc happydays.def \
		obj/*.bin obj/$(TARGET)

$(TARGET)-th.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-th.res
	$(BUILDPRC) $(TARGET)-th.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-dutch.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-dutch.res
	$(BUILDPRC) $(TARGET)-dutch.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-fr.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-fr.res
	$(BUILDPRC) $(TARGET)-fr.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-cz.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-cz.res
	$(BUILDPRC) $(TARGET)-cz.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

$(TARGET)-catalan.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-catalan.res
	$(BUILDPRC) $(TARGET)-catalan.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

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

obj/bin-sp.res: obj/$(TARGET)-sp.rcp
	@echo "Compiling resource happydays-sp.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L SPANISH -FKm obj/$(TARGET)-sp.rcp obj

obj/bin-chi-big5.res: obj/$(TARGET)-chi-big5.rcp
	@echo "Compiling resource happydays-chi-big5.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L CHINESE -F5 obj/$(TARGET)-chi-big5.rcp obj

obj/bin-chi-gb.res: obj/$(TARGET)-chi-gb.rcp
	@echo "Compiling resource happydays-chi-gb.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L CHINESE -F5 obj/$(TARGET)-chi-gb.rcp obj

obj/bin-de.res: obj/$(TARGET)-de.rcp
	@echo "Compiling resource happydays-de.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L GERMAN obj/$(TARGET)-de.rcp obj

obj/bin-br.res: obj/$(TARGET)-br.rcp
	@echo "Compiling resource happydays-br.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L PORTUGUESE-BR obj/$(TARGET)-br.rcp obj


bin-th.res: $(TARGET)-th.rcp
	rm -f *.bin
	$(PILRC) -L THAI $(TARGET)-th.rcp .

bin-dutch.res: $(TARGET)-dutch.rcp
	rm -f *.bin
	$(PILRC) -L DUTCH $(TARGET)-dutch.rcp .

bin-fr.res: $(TARGET)-fr.rcp
	rm -f *.bin
	$(PILRC) -L FRENCH $(TARGET)-fr.rcp .

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

obj/$(TARGET)-sp.rcp: $(TARGET).rcp translate/spanish.msg translate/hdr.msg
	@echo "Generating happydays-sp.rcp file..." && \
	cat translate/hdr.msg translate/spanish.msg $(TARGET).rcp \
			> obj/$(TARGET)-sp.rcp

obj/$(TARGET)-chi-big5.rcp: $(TARGET).rcp translate/chinese.msg translate/hdr.msg
	@echo "Generating happydays-chi-big5.rcp file..." && \
	cat translate/hdr.msg translate/chinese.msg $(TARGET).rcp \
			> obj/$(TARGET)-chi-big5.rcp

obj/$(TARGET)-chi-gb.rcp: $(TARGET).rcp translate/schinese.msg translate/hdr.msg
	@echo "Generating happydays-chi-gb.rcp file..." && \
	cat translate/hdr.msg translate/schinese.msg $(TARGET).rcp \
			> obj/$(TARGET)-chi-gb.rcp

obj/$(TARGET)-de.rcp: $(TARGET).rcp translate/german.msg translate/hdr.msg
	@echo "Generating happydays-de.rcp file..." && \
	cat translate/hdr.msg translate/german.msg $(TARGET).rcp \
			> obj/$(TARGET)-de.rcp

obj/$(TARGET)-br.rcp: $(TARGET).rcp translate/brazilian.msg translate/hdr.msg
	@echo "Generating happydays-br.rcp file..." && \
	cat translate/hdr.msg translate/brazilian.msg $(TARGET).rcp \
			> obj/$(TARGET)-br.rcp

$(TARGET)-de.rcp: $(TARGET).rcp german.msg hdr.msg
	cat hdr.msg german.msg $(TARGET).rcp > $(TARGET)-de.rcp

$(TARGET)-th.rcp: $(TARGET).rcp thai.msg hdr.msg
	cat hdr.msg thai.msg $(TARGET).rcp > $(TARGET)-th.rcp

$(TARGET)-dutch.rcp: $(TARGET).rcp dutch.msg hdr.msg
	cat hdr.msg dutch.msg $(TARGET).rcp > $(TARGET)-dutch.rcp

$(TARGET)-fr.rcp: $(TARGET).rcp french.msg hdr.msg
	cat hdr.msg french.msg $(TARGET).rcp > $(TARGET)-fr.rcp

$(TARGET)-cz.rcp: $(TARGET).rcp czech.msg hdr.msg
	cat hdr.msg czech.msg $(TARGET).rcp > $(TARGET)-cz.rcp

$(TARGET)-catalan.rcp: $(TARGET).rcp catalan.msg hdr.msg
	cat hdr.msg catalan.msg $(TARGET).rcp > $(TARGET)-catalan.rcp

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

prc/$(TARGET).prc: obj/$(TARGET).prc
	mv obj/$(TARGET).prc prc

zip: prc/$(TARGET).prc html
	-@echo "Making distribution file(happydays-$(VERSION).zip) ..." &&\
	\rm -rf dist && mkdir dist && mkdir dist/img && \
	cp prc/$(TARGET).prc dist && cp manual/*.html dist  && \
	cp manual/img/*.gif dist/img && cp manual/img/*.png dist/img && \
	cd dist && \
	zip -r ../happydays-$(VERSION).zip *  && \rm -rf dist

-include .depend
