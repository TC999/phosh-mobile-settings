# Configurable Tweaks support

## Introduction

This feature implements support for the YAML definition format implemented by
the now deprecated [postmarketOS Tweaks application][1].

Since the original documentation is incomplete, this serves as an up-to-date
reference that also includes notable changes from the original. While this
implementation largely strives to be compatible with the original, some features
are no longer as useful due to Linux phones gaining broader adoption and regular
settings applications implementing them. As such, they have not been implemented

If you see any justification for re-adding these features, patches are welcome!

 [1]: https://gitlab.postmarketos.org/postmarketOS/postmarketos-tweaks

## Backends

### gsettings

```yaml
backend: gsettings
gtype: boolean
key: org.postmarketos.Tweaks.coolsetting
```

Uses GSettings to read and write the settings. The key is the full path to the
setting. It's possible to set the key to a list of settings, in which case the
first one found will be used.

The gtype option is used to define which type the gsetting is in case it's
different than the type of the widget. This is mainly useful when things are
remapped.

### gtk3settings

```yaml
type: boolean
backend: gtk3settings
key: gtk-application-prefer-dark-theme
default: "0"
map:
  true: "1"
  false: "0"
```

This is a backend for modifying the `~/.config/gtk-3.0/settings.ini` file. The
key is the name of the setting inside the `[Settings]` section.

### symlink

```yaml
backend: symlink
key: ~/.local/var/example.data
source_ext: false
```

This creates a symlink where the source is the input data from the user and
the target is the `key`.

If `source_ext` is true, the extension of the source file will be appended to
the key before using that as the target path for the symlink.

Setting the value to `None` will remove the symlink.

This backend is normally used with the file widget

### sysfs

```yaml
type: choice
map:
  80% (4.10V): 4100000
  85% (4.15V): 4150000
  90% (4.20V): 4200000
  100% (4.35V): 4350000
backend: sysfs
key: /sys/class/power_supply/axp20x-battery/voltage_max
stype: int
```

With this, whenever a value is selected, it will be written to a staged
configuration file in the current user's cache directory. After which, the user
will be prompted to save the setting as root, which if accepted will cause the
file to be moved to `/etc/sysfs.d/phosh-mobile-settings-tweaks.conf`, where it
will be read by the update-sysfs program if it is run. That program then in
turn actually writes the value to the path inside `/sys`.

### xresources

```yaml
backend: xresources
key: dwm.background
type: color
default: #BBBBBB
```

Reads and writes to ~/.Xresources. If a given key is available in that file, it
will be read on program startup, otherwise the default value from the setting
definition is used.

## Differences from postmarketOS Tweaks

### Distributions ship their own settings definitions

Settings definitions are expected to be shipped by distributions instead of
including them in the application itself like postmarketOS Tweaks did. This
makes it easier to only include certain settings definitions on e.g. a user
interface-basis. Distributions are also able to configure the exact path they
want files to be searched for through the `tweaks-data-dir` build option.

### Parser handles setting gtype to type if relevant

In postmarketOS Twekas, if gtype was omitted, the GSettings backend would fall
back to the type property. This implementation handles that in the parser
instead, so backends can rely on type being set properly.

### Parser sets GSettings as backend if none is specified

While postmarketOS Tweaks sets GSettings as the value of the `backend` setting
definition property if none was specified when parsing, this implementation
instead sets it as the default value for the backend property in the parser. As
such, setting definitions always have the backend property set in the code even
if they don't in the conf-tweaks files.

### Parser sets `map` property based on datasource

Backends shouldn't normally have to access the datasource property as the
parser handles setting the `map` property to the data provided by the
datasource.

### Hardwareinfo, environment, osksdl, css, and soundtheme backends are omitted

The hardwareinfo backend was never documented in postmarketOS Tweaks, and is
not as useful as it once was with both GNOME's and Plasma's settings
applications supporting mostly all of the properties it supported. Additionally,
it carried its own list of CPU and System on a Chip names, vendors, et cetera
which quickly became outdated and would have needed constant updating (which
didn't happen). This is better handled by external libraries such as KDE's Solid
framework.

Similarly, the osksdl backend was also never documented. However, its reason for
omission is different and rather due to the fact that [osk-sdl was deprecated in
favour of unl0kr][2]. As such, there is little point in supporting configuration
for it, and if anything misleading.

The environment backend too was undocumented, however unlike the others there
do not seem to be any instances of it actually being used. As such, it has been
omitted. However, unlike the hardwareinfo and osksdl backend which have
fundamental flaws that resulted in their exclusion, the environment backed is
only excluded due to being undocumented and apparently unused.

While the css backend didn't have any fundamental problems, all of its uses had
been replaced by better solutions. It could be brought back if a new use-case
is found.

Finally, the soundtheme backend is only excluded because all of the functions
it served in the original have been replaced by native settings in
Phosh Mobile Settings. If there is interest in including it, e.g. if you have a
personal use-case, feel free to make an issue.

To clarify, the parser still recognises these omitted backend types, but any
setting using it gets marked as invalid by the page builder and consequently is
hidden.
