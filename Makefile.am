SUBDIRS =	\
	data	\
	libcaphe\
	po	\
	src

EXTRA_DIST = 		\
	autogen.sh	\
	HACKING.md	\
	README.md	\
	THANKS.md

distclean-local:
	find . -name '*~' -print0 | \
		xargs --no-run-if-empty -0 rm -vf

indent:
	./scripts/code/indent.sh all

icons:
	for size in 16 22 24 32 48; do \
	  inkscape \
	    --export-area-drawing \
	    --export-width $${size} \
	    --export-png data/icons/hicolor/$${size}x$${size}/apps/goodvibes.png \
	    data/icons/src/goodvibes-small.svg; \
	done
	for size in 256 512; do \
	  inkscape \
	    --export-area-drawing \
	    --export-width $${size} \
	    --export-png data/icons/hicolor/$${size}x$${size}/apps/goodvibes.png \
	    data/icons/src/goodvibes-large.svg; \
	done
