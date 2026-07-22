.. _phosh-mobile-settings(1):

========================
phosh-mobile-settings
========================

-------------------------------------------
Mobile settings application for Phosh
-------------------------------------------

SYNOPSIS
--------
| **phosh-mobile-settings** [OPTIONS...] [PANEL]

DESCRIPTION
-----------

``phosh-mobile-settings`` is a settings application for the Phosh mobile shell
and related components. It provides a graphical interface to configure system
and application settings that would otherwise require command line usage.

OPTIONS
-------

``-h``, ``--help``
  Print help and exit

``-v``, ``--version``
  Show version information

``-d``, ``--debug-info``
  Print debugging information (e.g. for bug reports)

``-l``, ``--list``
  List the available panels and exit

*PANEL*
  Optional panel name to open. If given, the window opens with that panel
  shown. If omitted, the window opens with the last opened panel (saved in
  GSettings). Use ``--list`` to see available panel names.

CONFIGURATION
-------------

``phosh-mobile-settings`` stores its state and options in the
``mobi.phosh.MobileSettings`` GSettings schema (e.g. last opened panel,
optional conf-tweaks). Use ``gsettings(1)`` to inspect or override.

ENVIRONMENT VARIABLES
---------------------

``phosh-mobile-settings`` honors the following environment variables:

- ``MS_FORCE_OSK``: Assume a certain on-screen keyboard for development.
  Valid values are ``pos``, ``squeekboard`` and ``unknown``.

- ``MS_OSK_LAYOUTS``: Path to the OSK layouts JSON file. Overrides the
  default (e.g. when testing layouts or running from a build tree).

- ``MS_FORCE_DEVICE``: Assume a device name for plugin
  compatibility. Must match a supported device identifier (development).

- ``G_MESSAGES_DEBUG``, ``G_DEBUG`` and other environment variables supported
  by GLib. https://docs.gtk.org/glib/running.html

- ``GSETTINGS_SCHEMA_DIR``: Override search path for GSettings schemas
  (e.g. when running from a build tree).

- ``GSETTINGS_BACKEND``: GSettings backend; use ``memory`` to avoid writing
  to the system configuration (useful for development).

The ``--debug-info`` output includes a summary of relevant environment
variables (e.g. ``XDG_*``, ``WAYLAND_*``, ``PHOSH_DEBUG``) for bug reports.

SEE ALSO
--------

``phosh(1)``, ``phoc(1)``, ``gsettings(1)``
