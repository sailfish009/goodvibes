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
	./scripts/images.sh icons
