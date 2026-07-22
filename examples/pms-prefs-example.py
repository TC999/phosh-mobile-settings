#!/usr/bin/python3

import gi

gi.require_version("Pms", "1.0")
gi.require_version("Adw", "1")
gi.require_version("Gtk", "4.0")
from gi.repository import Pms
from gi.repository import Adw
from gi.repository import Gtk


def on_activate(app):
    prefs = Pms.OskLayoutPrefs(visible=True)
    prefs.load_osk_layouts()

    box = Gtk.Box(margin_top=10, margin_end=10, margin_bottom=10, margin_start=10)
    box.append(prefs)

    win = Gtk.ApplicationWindow(application=app)
    win.set_child(box)
    win.present()


if __name__ == "__main__":
    app = Adw.Application(application_id="mobi.phosh.MobileSettings.LibpmsExample")
    app.connect("activate", on_activate)
    app.run(None)
