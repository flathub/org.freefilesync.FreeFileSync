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

## Maintaining this repository

[Please become a co-maintainer](https://github.com/flathub/org.freefilesync.FreeFileSync/issues/11)

The workflow for building a new release `REL` is:
```
git checkout -b REL master
# adjust *FreeFileSync.yml and data/*appdata.xml
flatpak-builder builddir org.freefilesync.FreeFileSync.yml --force-clean --ccache
# test the app
flatpak-builder --run builddir org.freefilesync.FreeFileSync.yml FreeFileSync

git add -A
git commit
git push -u origin REL
# submit a PR

# after the PR is approved
git checkout master
git merge --ff-only REL
git push
git tag vREL
git push --tags
git branch -d REL
git push -d origin REL
```

See the progress and controls for new builds at
[Flathub buildbot](https://flathub.org/builds/#/apps/org.freefilesync.FreeFileSync).
