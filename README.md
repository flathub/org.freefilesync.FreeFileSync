# FreeFileSync on Flathub

[FreeFileSync] is a graphical folder comparison and synchronization software. See its homepage at [freefilesync.org][FreeFileSync].

This repo contains a [Flatpak] manifest for building a Flatpak package for FreeFileSync. You can download the the final Flatpak package from [FreeFileSync on Flathub].

**Note: This is not the upstream repo for FreeFileSync. See [freefilesync.org][FreeFileSync] instead.**

[![screenshot](https://www.freefilesync.org/images/screenshots/openSUSE.png)](https://www.freefilesync.org/images/screenshots/openSUSE.png)

[FreeFileSync]: https://www.freefilesync.org
[Flatpak]: https://flatpak.org
[FreeFileSync on Flathub]: https://flathub.org/apps/details/org.freefilesync.FreeFileSync

## Installation

To install FreeFileSync through Flathub, first make sure Flathub is [set up](https://flatpak.org/setup/) on your system, and then visit [FreeFileSync on Flathub] page and click *Install*.

Or you can use the command line:
```
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.freefilesync.FreeFileSync
```

## Usage

This package contains two tools named `FreeFileSync` and `RealTimeSync`. Simply search for either of them in your desktop app launcher.

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

See the progress and controls for new builds at [Flathub buildbot](https://flathub.org/builds/#/apps/org.freefilesync.FreeFileSync).
