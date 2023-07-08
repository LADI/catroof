.PHONY: all
all: catroofd uirun

.PHONY: doc
doc: README.html

#all: doc

catroofd: src/alsa.c
#	gcc $$(pkg-config --cflags --libs alsa) -o catroofd src/alsa.c
	./waf configure
	./waf

.PHONY: uirun
uirun:
	./src/catroof.lua

README.html: README.adoc README-docinfo.html README-docinfo-header.html
	asciidoc -b html5 -a data-uri -a icons --theme ladi -o README.html README.adoc
