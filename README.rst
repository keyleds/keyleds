==============
|logo| keyleds
==============

Userspace service for Logitech keyboards with per-key RGB LEDs.

Quick links:

* `installing`_
* `documentation`_
* `sample configuration`_
* `issue tracker`_

This project supports all Logitech USB keyboards, on most keyboard layouts.
The service is obviously only useful with those featuring per-key light control.

Features
--------

* Flexible per-application RGB settings with `key groups`_.
* Can react to window title changes, enabling switching profiles based on
  current webpage in browser or open file extension in editors.
* Features improved animation plugins, with key group support:

  - **Fixed** colors.
  - **Breathing** effect *(transparency-based)*.
  - **Wave** effect *(wavelength, speed and direction fully configurable,
    supports arbitrary color list with transparency)*.
  - **Stars** effect *(number, color list and light duration configurable)*.
  - **Keypress feedback** effect *(as all plugins, can be composited)*.

* Several plugins can be active at once, and composited with **alpha blending** to
  build complex effects.
* Systemd **session support**, detecting user switches so multiple users can
  use the service without them fighting over keyboard control.
* Supports **multi-keyboard** with different configuration per keyboard.

And a few goodies:

* Effects are implemented as **loadable modules**, using a clean API. Develop
  yours now.
* **Command-line tool** for your scripting needs and extended configuration
  (set game-mode keys, change report rate, see USB exchanges…).
* Optimized pure **C library**.
* **Python3 bindings** for the library.

----

Feedback, feature ideas, pull requests are welcome!

.. _installing: https://github.com/spectras/keyleds/wiki/Installing
.. _documentation: https://github.com/spectras/keyleds/wiki
.. _sample configuration: https://github.com/spectras/keyleds/blob/master/keyledsd/keyledsd.conf.sample
.. _issue tracker: https://github.com/spectras/keyleds/issues
.. _key groups: https://github.com/spectras/keyleds/wiki/Key-Group
.. |logo| image:: logo.svg
   :width: 64px
   :height: 80px
   :align: middle
   :alt:
