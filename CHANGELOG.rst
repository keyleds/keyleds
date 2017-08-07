#############
Release Notes
#############

*****************************
0.4 - current release
*****************************

Released on August 7, 2017

Features:

- Added support for systemd user-acces permissions. This means the service
  will pause and resume animations when current session changes.
- Added stars effect plugin
- Added available plugins and device layout information to DBus interface.

Bugfixes:

- Fixed: hangs when system clock goes back in time.
- Fixed: I/O errors after the service was paused for some time and other
  tools communicated with it in the meantime.

----

Changelog added for version 0.3.3
