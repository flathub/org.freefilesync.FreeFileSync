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

Since there is no version control for FreeFileSync, the exploded tarball for
each version is pushed into the `src` branch. Patches are kept in
`patchNN-description` branches and rebased on top of new releases.
(The reason for this is to make patch rebasing more comfortable and
maintainable. A new release is still built from the tarball, not from the `src`
branch.)

The workflow for building a new release `REL` is:
```
git branch vREL master
# download new tarball into tarball/
git checkout src
rm src -rf
unzip -d src tarball/<tarball>.zip
git add -A
git commit

# for each patchNN branch, do:
git checkout patchNN
git rebase src
# end for

git checkout vREL

# for each patchNN branch, do:
git diff src patchNN > ./patchNN.patch
# end for

# adjust *appdata.xml and *FreeFileSync.yml
flatpak-builder builddir org.freefilesync.FreeFileSync.yml --force-clean
# test the app

git add -A
git commit
git push -u origin vREL
# submit a PR

# after merging PR to master
git branch -d vREL
git push -d origin vREL
git checkout master
git tag vREL
git push --tags
# push all modified branches (patchNN, src)
```
