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
```sh
# Create a new git branch REL (e.g. 11.6):
git checkout -b REL master

# Adjust the manifest and appdata
your-favorite-editor org.freefilesync.FreeFileSync.yml
your-favorite-editor data/org.freefilesync.FreeFileSync.appdata.xml

# Build and install. The installation part is necessary, because due to extra-data approach (see
# manifest), the actual FFS binary is downloaded and processed only during installation.
flatpak-builder builddir org.freefilesync.FreeFileSync.yml --force-clean --ccache --install --user

# Test the app. Your dev version should be installed as the 'master' branch, so if you have the
# stable version installed as well, you must distinguish them as shown below. Check your
# 'flatpak list --app' output to make sure.
flatpak run org.freefilesync.FreeFileSync//master

# Remove the dev version of the app
flatpak remove org.freefilesync.FreeFileSync//master

# Commit the changes
git add -u
git diff --cached
git commit
git push -u origin REL
# Submit the pull request now

# After the PR is approved, release it
git checkout master
git merge --ff-only REL
git push
git tag vREL
git push --tags
git branch -d REL
git push -d origin REL
```

See the progress and controls for new builds at [Flathub buildbot](https://flathub.org/builds/#/apps/org.freefilesync.FreeFileSync) ([beta branch](https://flathub.org/builds/#/apps/org.freefilesync.FreeFileSync~2Fbeta)).
