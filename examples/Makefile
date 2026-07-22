LIBS = pms-1.0-uninstalled gtk4 libadwaita-1

pms-prefs-example: pms-prefs-example.c
	gcc -Wall -O2 -DLIBPMS_USE_UNSTABLE_API `pkgconf --cflags $(LIBS)` $< `pkgconf --libs $(LIBS)` -o $@

clean:
	rm -f pms-prefs-example
