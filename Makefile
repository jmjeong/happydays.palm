## Makefile for HappyDays application

VERSION = 2.33
TARGET = happydays
APPNAME = "HappyDays"
APPID = "Jmje"

FILES = lun2sol.c sol2lun.c address.c datebook.c util.c \
		birthdate.c happydays.c memodb.c s2lconvert.c \
		todo.c notify.c datealarm.c
OBJS = obj/lun2sol.o obj/sol2lun.o obj/address.o obj/datebook.o obj/util.o \
		obj/birthdate.o obj/happydays.o obj/memodb.o obj/s2lconvert.o \
		obj/todo.o obj/notify.o obj/datealarm.o \
		obj/happydays-sections.o

CC = m68k-palmos-gcc

CFLAGS = -Wall -O2 -I/opt/palmdev/sdk-5r3/include/Clie -I/opt/palmdev/sdk-5r3/include/Clie/System -I/opt/palmdev/sdk-5r3/include/Clie/Libraries
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

all-lang: en it tur ko sp chi de br nor cz fr ru

PORTUGUESE_BR:
	$(CC) -c -DPORTUGUESE_BR $(CFLAGS) $(CPPFLAGS) \
			-o obj/happydays.o happydays.c

ITALIAN:
	$(CC) -c -DITALIAN $(CFLAGS) $(CPPFLAGS) \
			-o obj/happydays.o happydays.c

GERMAN:
	$(CC) -c -DGERMAN $(CFLAGS) $(CPPFLAGS) -o obj/happydays.o happydays.c

ENGLISH:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o obj/happydays.o happydays.c

en: ENGLISH obj/$(TARGET).prc
	mv obj/$(TARGET).prc prc

it: ITALIAN obj/$(TARGET)-it.prc 
	mv obj/$(TARGET)-it.prc prc
	zip $(TARGET)-$(VERSION)-it.zip prc/$(TARGET)-it.prc 

tur: ENGLISH obj/$(TARGET)-tur.prc 
	mv obj/$(TARGET)-tur.prc prc
	zip $(TARGET)-$(VERSION)-tur.zip prc/$(TARGET)-tur.prc manual/turkish-manual.zip

ko: ENGLISH obj/$(TARGET)-k8.prc obj/$(TARGET)-k11.prc
	mv obj/$(TARGET)-k8.prc obj/$(TARGET)-k11.prc prc
	zip $(TARGET)-$(VERSION)-ko.zip prc/$(TARGET)-k8.prc prc/$(TARGET)-k11.prc

sp: ENGLISH obj/$(TARGET)-sp.prc
	mv obj/$(TARGET)-sp.prc prc
	zip $(TARGET)-$(VERSION)-sp.zip prc/$(TARGET)-sp.prc

chi: ENGLISH obj/$(TARGET)-chi-big5.prc obj/$(TARGET)-chi-gb.prc
	mv obj/$(TARGET)-chi-big5.prc obj/$(TARGET)-chi-gb.prc prc
	zip $(TARGET)-$(VERSION)-ch.zip prc/$(TARGET)-chi-big5.prc \
			prc/$(TARGET)-chi-gb.prc

de: GERMAN obj/$(TARGET)-de.prc 
	mv obj/$(TARGET)-de.prc prc
	zip $(TARGET)-$(VERSION)-de.zip prc/$(TARGET)-de.prc

br: PORTUGUESE_BR obj/$(TARGET)-br.prc
	mv obj/$(TARGET)-br.prc prc
	zip $(TARGET)-$(VERSION)-br.zip prc/$(TARGET)-br.prc

nor: ENGLISH obj/$(TARGET)-nor.prc
	mv obj/$(TARGET)-nor.prc prc
	zip $(TARGET)-$(VERSION)-nor.zip prc/$(TARGET)-nor.prc

cz: ENGLISH obj/$(TARGET)-cz.prc
	mv obj/$(TARGET)-cz.prc prc
	zip $(TARGET)-$(VERSION)-cz.zip prc/$(TARGET)-cz.prc

fr: ENGLISH obj/$(TARGET)-fr.prc
	mv obj/$(TARGET)-fr.prc prc
	zip $(TARGET)-$(VERSION)-fr.zip prc/$(TARGET)-fr.prc

ru: ENGLISH obj/$(TARGET)-ru.prc
	mv obj/$(TARGET)-ru.prc prc
	zip $(TARGET)-$(VERSION)-ru.zip prc/$(TARGET)-ru.prc

catalan: ENGLISH obj/$(TARGET)-catalan.prc
	mv obj/$(TARGET)-catalan.prc prc
	zip $(TARGET)-$(VERSION)-catalan.zip prc/$(TARGET)-catalan.prc

th: ENGLISH obj/$(TARGET)-th.prc
	mv obj/$(TARGET)-th.prc prc
	zip $(TARGET)-$(VERSION)-th.zip prc/$(TARGET)-th.prc

danish: ENGLISH obj/$(TARGET)-danish.prc
	mv obj/$(TARGET)-danish.prc prc
	zip $(TARGET)-$(VERSION)-danish.zip prc/$(TARGET)-danish.prc


dutch: $(TARGET)-dutch.prc
	zip $(TARGET)-$(VERSION)-dutch.zip $(TARGET)-dutch.prc


.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

obj/happydays-sections.o: obj/happydays-sections.s
	@echo "Compiling happydays-sections.s..." && \
	$(CC) -c obj/happydays-sections.s -o obj/happydays-sections.o

obj/happydays-sections.s obj/happydays-sections.ld : happydays.def
	@echo "Making multi-section info files..." && \
	cd obj && /usr/bin/m68k-palmos-multigen ../happydays.def

obj/$(TARGET).prc: obj/$(TARGET) obj/bin.res
	@echo "Building program file ./obj/happydays.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET).prc happydays.def obj/*.bin obj/$(TARGET)

obj/$(TARGET)-it.prc: obj/$(TARGET) obj/bin-it.res
	@echo "Building program file ./obj/happydays-it.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-it.prc happydays.def obj/*.bin obj/$(TARGET)

obj/$(TARGET)-tur.prc: obj/$(TARGET) obj/bin-tur.res
	@echo "Building program file ./obj/happydays-tur.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-tur.prc happydays.def obj/*.bin obj/$(TARGET)

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

obj/$(TARGET)-nor.prc: obj/$(TARGET) obj/bin-nor.res
	@echo "Building program file ./obj/happydays-nor.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-nor.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-cz.prc: obj/$(TARGET) obj/bin-cz.res
	@echo "Building program file ./obj/happydays-cz.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-cz.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-fr.prc: obj/$(TARGET) obj/bin-fr.res
	@echo "Building program file ./obj/happydays-fr.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-fr.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-ru.prc: obj/$(TARGET) obj/bin-ru.res
	@echo "Building program file ./obj/happydays-ru.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-ru.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-catalan.prc: obj/$(TARGET) obj/bin-catalan.res
	@echo "Building program file ./obj/happydays-catalan.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-catalan.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-danish.prc: obj/$(TARGET) obj/bin-danish.res
	@echo "Building program file ./obj/happydays-danish.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-danish.prc happydays.def \
		obj/*.bin obj/$(TARGET)

obj/$(TARGET)-th.prc: obj/$(TARGET) obj/bin-th.res
	@echo "Building program file ./obj/happydays-th.prc..." && \
	$(BUILDPRC) -o obj/$(TARGET)-th.prc happydays.def \
		obj/*.bin obj/$(TARGET)

$(TARGET)-dutch.prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin-dutch.res
	$(BUILDPRC) $(TARGET)-dutch.prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc


obj/bin.res: obj/$(TARGET)-en.rcp
	@echo "Compiling resource happydays.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L ENGLISH obj/$(TARGET)-en.rcp obj

obj/bin-k8.res: obj/$(TARGET)-ko.rcp
	@echo "Compiling resource happydays-ko.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L KOREAN -Fkt obj/$(TARGET)-ko.rcp obj

obj/bin-it.res: obj/$(TARGET)-it.rcp
	@echo "Compiling resource happydays-it.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L ITALIAN obj/$(TARGET)-it.rcp obj

obj/bin-tur.res: obj/$(TARGET)-tur.rcp
	@echo "Compiling resource happydays-tur.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L TURKISH obj/$(TARGET)-tur.rcp obj

obj/bin-k11.res: obj/$(TARGET)-ko.rcp
	@echo "Compiling resource happydays-ko.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L KOREAN -FKm obj/$(TARGET)-ko.rcp obj

obj/bin-sp.res: obj/$(TARGET)-sp.rcp
	@echo "Compiling resource happydays-sp.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L SPANISH obj/$(TARGET)-sp.rcp obj

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

obj/bin-nor.res: obj/$(TARGET)-nor.rcp
	@echo "Compiling resource happydays-nor.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L NORWEGIAN obj/$(TARGET)-nor.rcp obj

obj/bin-cz.res: obj/$(TARGET)-cz.rcp
	@echo "Compiling resource happydays-cz.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L CZECH obj/$(TARGET)-cz.rcp obj


obj/bin-fr.res: obj/$(TARGET)-fr.rcp
	@echo "Compiling resource happydays-fr.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L FRENCH obj/$(TARGET)-fr.rcp obj

obj/bin-ru.res: obj/$(TARGET)-ru.rcp
	@echo "Compiling resource happydays-ru.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L RUSSIAN obj/$(TARGET)-ru.rcp obj

obj/bin-catalan.res: obj/$(TARGET)-catalan.rcp
	@echo "Compiling resource happydays-catalan.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L CATALAN  obj/$(TARGET)-catalan.rcp obj

obj/bin-danish.res: obj/$(TARGET)-danish.rcp
	@echo "Compiling resource happydays-danish.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L DANISH  obj/$(TARGET)-danish.rcp obj

obj/bin-th.res: obj/$(TARGET)-th.rcp
	@echo "Compiling resource happydays-th.rcp..." &&\
	(cd obj && rm -f *.bin ) && \
	$(PILRC) -q -L THAI  obj/$(TARGET)-th.rcp obj

bin-dutch.res: $(TARGET)-dutch.rcp
	rm -f *.bin
	$(PILRC) -L DUTCH $(TARGET)-dutch.rcp .


bin-br.res: $(TARGET)-br.rcp
	rm -f *.bin
	$(PILRC) -L BRAZILIAN $(TARGET)-br.rcp .

obj/$(TARGET)-en.rcp: $(TARGET).rcp translate/english.msg translate/hdr.msg
	@echo "Generating happydays-en.rcp file..." && \
	cat translate/hdr.msg translate/english.msg $(TARGET).rcp \
			> obj/$(TARGET)-en.rcp

obj/$(TARGET)-it.rcp: $(TARGET).rcp translate/italian.msg translate/hdr.msg
	@echo "Generating happydays-it.rcp file..." && \
	cat translate/hdr.msg translate/italian.msg $(TARGET).rcp \
			> obj/$(TARGET)-it.rcp

obj/$(TARGET)-tur.rcp: $(TARGET).rcp translate/turkish.msg translate/hdr.msg
	@echo "Generating happydays-tur.rcp file..." && \
	cat translate/hdr.msg translate/turkish.msg $(TARGET).rcp \
			> obj/$(TARGET)-tur.rcp

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

obj/$(TARGET)-nor.rcp: $(TARGET).rcp translate/norwegian.msg translate/hdr.msg
	@echo "Generating happydays-nor.rcp file..." && \
	cat translate/hdr.msg translate/norwegian.msg $(TARGET).rcp \
			> obj/$(TARGET)-nor.rcp

obj/$(TARGET)-cz.rcp: $(TARGET).rcp translate/czech.msg translate/hdr.msg
	@echo "Generating happydays-cz.rcp file..." && \
	cat translate/hdr.msg translate/czech.msg $(TARGET).rcp \
			> obj/$(TARGET)-cz.rcp

obj/$(TARGET)-fr.rcp: $(TARGET).rcp translate/french.msg translate/hdr.msg
	@echo "Generating happydays-fr.rcp file..." && \
	cat translate/hdr.msg translate/french.msg $(TARGET).rcp \
			> obj/$(TARGET)-fr.rcp

obj/$(TARGET)-ru.rcp: $(TARGET).rcp translate/russian.msg translate/hdr.msg
	@echo "Generating happydays-ru.rcp file..." && \
	cat translate/hdr.msg translate/russian.msg $(TARGET).rcp \
			> obj/$(TARGET)-ru.rcp

obj/$(TARGET)-catalan.rcp: $(TARGET).rcp translate/catalan.msg translate/hdr.msg
	@echo "Generating happydays-catalan.rcp file..." && \
	cat translate/hdr.msg translate/catalan.msg $(TARGET).rcp \
			> obj/$(TARGET)-catalan.rcp

obj/$(TARGET)-danish.rcp: $(TARGET).rcp translate/danish.msg translate/hdr.msg
	@echo "Generating happydays-danish.rcp file..." && \
	cat translate/hdr.msg translate/danish.msg $(TARGET).rcp \
			> obj/$(TARGET)-danish.rcp

obj/$(TARGET)-th.rcp: $(TARGET).rcp translate/thai.msg translate/hdr.msg
	@echo "Generating happydays-thai.rcp file..." && \
	cat translate/hdr.msg translate/thai.msg $(TARGET).rcp \
			> obj/$(TARGET)-th.rcp

$(TARGET)-dutch.rcp: $(TARGET).rcp dutch.msg hdr.msg
	cat hdr.msg dutch.msg $(TARGET).rcp > $(TARGET)-dutch.rcp


$(TARGET)-fr.rcp: $(TARGET).rcp french.msg hdr.msg
	cat hdr.msg french.msg $(TARGET).rcp > $(TARGET)-fr.rcp


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
	cd obj && \rm -rf *

veryclean: clean
	-rm -f $(TARGET)*.prc pilot.ram pilot.scratch

html: 
	@echo "Generating html file" && \
	cd manual && wmk

prc/$(TARGET).prc: obj/$(TARGET).prc
	mv obj/$(TARGET).prc prc

src:
	cvs co palm/happydays && cd palm && \
		zip -r ../happydays-src-$(VERSION).zip happydays && \
		cd .. && \rm -rf palm

zip: prc/$(TARGET).prc 
	-@echo "Making distribution file(happydays-$(VERSION).zip) ..." &&\
	\rm -rf dist && mkdir dist && \
	cp prc/$(TARGET).prc dist && cp manual/README.txt dist  && \
	cp manual/HappyDaysManual.pdf dist && cd dist && \
	zip -r ../happydays-$(VERSION).zip *  && \rm -rf dist

upload: zip 
	scp *.zip jmjeong.com:~/wikix/myfile/HappyDays

-include .depend
