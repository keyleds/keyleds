#############
Release Notes
#############

*****************************
0.5.0 - current release
*****************************

Released on September 24th, 2017

Features:

- Added keypress feedback plugin.
- More generally, made keyboard events available to all plugins.

Bugfixes:

- Fixed compile errors on non-x86 platforms.

*****************************
0.4.3
*****************************

Released on September 19th, 2017

Bugfixes:

- Fixed errors when compiling with clang.
- Dropped Qt4 in favor of Qt5 for the event loop.
- HAL library no longer exposes internal symbols.

*****************************
0.4.2
*****************************

Released on September 15th, 2017

Features:

- Added layout descriptions for G810.

Bugfixes:

- Work around animation freeze when adjusting system time.

*****************************
0.4.1
*****************************

Released on August 29th, 2017

Bugfixes:

- Introduce a delay and multiple retry attempts to recover after an I/O
  error. Helps with keyboard diconnection when system comes back from sleep.
- Refactored the main animation loop to fix some race issues.
- Centralized logging and connected it to command line switches, so ``-v``
  and ``-q`` actually work.


*****************************
0.4
*****************************

Released on August 7th, 2017

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
