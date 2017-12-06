# FreeFileSync on Flathub

[FreeFileSync](https://www.freefilesync.org/) is a graphical folder comparison
and synchronization software. See its homepage at
[freefilesync.org](https://www.freefilesync.org).

[![screenshot](https://www.freefilesync.org/images/screenshots/openSUSE.png)](https://www.freefilesync.org/images/screenshots/openSUSE.png)

This repo contains a [Flatpak] manifest for building a
Flatpak package for FreeFileSync. You can download the the final Flatpak
package from [Flathub].

[Flatpak]: http://flatpak.org
[Flathub]: https://flathub.org

## Installation

To install FreeFileSync through Flathub, visit
[flathub.org/apps](https://flathub.org/apps.html) and click on FreeFileSync.
It should open in a supported software center (like GNOME Software) with the
option to install the app.

Or you can use the command line:
```
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.freefilesync.FreeFileSync
```

## Usage

This package contains two tools named `FreeFileSync` and `RealTimeSync`. Simply
search for either of them in your desktop app launcher.

Or you can use the command line:
```
flatpak run org.freefilesync.FreeFileSync
```
or
```
flatpak run --command=RealTimeSync org.freefilesync.FreeFileSync
```
