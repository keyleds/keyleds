=======
keyleds
=======

This implements a user-level driver for
Logitech keyboards. It is a personal development project built by
capturing Logitech's software communications to the keyboard. Plus
some experiments throwing bytes at it.

* Tested on a G410 Atlas Spectrum keyboard.
* Other Logitech keyboard should work as well. If you test other
  keyboards, please let me know what works so I can make a list.

Why another project? Because the ones I found work from replaying
captured USB frames, which is clumsy and prone to failing at
every firmware update. On the other hand, this project contains
an actual implementation of Logitech protocol, which should make
it much more robust and future-proof.

Features
--------

The project is split into four components. Two for end users:

* A background service. Watches keyboards and reacts to external events by
  animating keys, like Logitech's software does on Windows.
* A small command-line tool to test devices, query and set leds in scripts.

And two for developers:
* A static library implementing Logitech protocol.
* Python3 bindings for that library.

Service features
~~~~~~~~~~~~~~~~

The service is intended to be run in an X session as the user. It supports
the following features:

* Dynamic detection of keyboards as they are plugged in.
* Reacts to X session events: active window switching, window title updates.
* Plugin architecture for key effects (static plugins only for now).
* Configurable filtering of: plugins can be enabled/disabled based
  on keyboard serial, window state, ...
* Exposes some of its configuration through DBus.

Command-line tool and library features
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following features are supported:

* Device enumeration and basic information (keyboard type, firmware version, …).
* Inspection of device capabilities and led configuration.
* Individual key led control (set and get).
* Automatic translation between led identifiers and keycodes.
* Game-mode configuration
* Report rate configuration
* Library is written is pure C, with special care taken to avoid memory
  allocations on critical code paths.

Compiling
---------

Building keyleds requires the following dependencies to be installed:

* cmake
* Qt4 core development files
* X11 development files
* libc development files
* libudev development files
* libxml2 development files
* libyaml development files
* Qt4 dbus development files *(optional, required for DBus support)*
* python3 development files *(optional, used for python bindings)*
* cython3 *(optional, used for python bindings)*

The following one-liner should get you up and running on debian systems:

.. code-block:: bash

    sudo apt-get install build-essential cmake linux-libc-dev libqt4-dev libudev-dev libx11-dev libxml2-dev libyaml-dev python3-dev cython

Build the project with the following command:

.. code-block:: bash

    cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

You may then install it with a good old:

.. code-block:: bash

    sudo make install

Dealing with device permissions
-------------------------------

By default, the keyboard device is not usable by any non-root user.
This means you must either:

* Run this project as root. This means the cli tool, or your own program
  using the library.
* Configure your system to make the device accessible to other users.
  On udev-based systems, you can copy ``logitech.rules`` into
  ``/etc/udev/rules.d/90-logitech-plugdev.rules`` to automatically grant
  access to members of the ``plugdev`` group. Beware that this makes
  it possible for those users to spy on some keyboard presses.

Using the background service
----------------------------

Either start it manually or drop the provided desktop file in your
``$HOME/.config/autostart`` directory so it is started automatically on login.

It requires a configuration file. You may use `keyledsd.conf`_ as a starting
point. Copy it as ``$HOME/.config/keyledsd.conf`` if using the desktop file.


Using the cli tool
------------------

* Listing connected, supported devices:

  .. code-block:: console

        $ keyledsctl list
        /dev/hidraw1 046d:c330 [111111111111]

  The number in square brackets is the USB serial number of the device.

* Querying device information:

  .. code-block:: console

        $ keyledsctl info
        Name:           Gaming Keyboard G410
        Type:           keyboard
        Model:          c33000000000
        Serial:         xxxxxxxx
        Firmware[c330]: application U1  v1.002 r20 [active]
        Firmware[aabc]: bootloader BOT v14.000 r7
        Known features: feature version gamemode name reportrate leds led-effects
        Report rates:   [1ms] 2ms 4ms 8ms
        LED block[01]:  105 keys, max_rgb(255, 255, 255)
        LED block[40]:    2 keys, max_rgb(255, 255, 255)

* Setting LED state. The subcommand takes a list of directives in the form
  ``key=color`` and applies them in order. Keys can be either their name,
  their number or the special keyword ``all``:

  .. code-block:: console

        $ keyledsctl set-leds all=yellow enter=green f1=ff00cd f2=ff00cd

  This would set all keys to be yellow, except ``enter``, ``F1`` and ``F2``,
  which would be green and pink respectively. Recognized colors include all
  `CSS color names`_, and rgb values in hexa notation (ala web color). A list of
  recognized keys can be obtained using ``get-leds`` or looking at
  `key names`_ in the source.

  For non-standard keys, a key block can be set with ``-b``. It applies for
  all subsequent directives. For instance, this sets LED-enable key to red,
  Gamemode-enable key to blue and key “1” to yellow.

  .. code-block:: console

        $ keyledsctl set-leds -b modes 1=red 2=blue -b keys 1=yellow

  Known key blocks are: *keys*, *media*, *gkeys*, *logo* and *modes*.

  Lastly, while special key blocks just use the key number, keys in the
  ``keys`` block are looked up in a keycode translation table. This means
  “1” is recognized as key “1” (actual numeric value 0x02). To force
  a numeric code, either prepend it with a 0 (``01=yellow``) or use
  hexadecimal (``x1=yellow``).

* Getting LED state:

  .. code-block:: console

        $ keyledsctl get-leds
        A=#00dcff
        B=#00dcff
        ...
        RALT=#00dcff
        RMETA=#00dcff

  Each key is output on a single line. The format is the same that is used by
  ``set-leds``, making it possible to save/restore LED status this way:

  .. code-block:: console

        # Saving a block of leds, such as keys, modes, gkeys...
        $ keyledsctl get-leds -b keys > savedkeys.txt
        # Restoring the block of leds
        $ xargs <savedkeys.txt keyledsctl set-leds -b keys

* Setting blocked keys when game mode is enabled:

  .. code-block:: console

        $ keyledsctl gamemode lmeta rmeta compose

  To clear the list, simply invoke the command with no key.

* Dealing with multiple devices. Either device path or USB serial ca be used:

  .. code-block:: console

        $ keyledsctl list
        /dev/hidraw1 046d:c330 [111111111111]
        /dev/hidraw5 046d:c330 [222222222222]
        /dev/hidraw7 046d:c330 [333333333333]
        $ keyledsctl set-leds -d /dev/hidraw1 all=red
        $ keyledsctl set-leds -d 222222222222 all=green
        $ export KEYLEDS_DEVICE=333333333333
        $ keyledsctl set-leds all=blue

  This sequence sets the three attached keyboards to turn all red, all green
  and all blue respectively. Note that if both and environment variable and
  a command-line option are specified, the command-line option takes precedence.

* Lastly, one may insert option ``-dd`` before any subcommand to enable
  debug output, including USB exchanges.

Using the API
-------------

Simply include `keyleds.h`_, and link with ``libkeyleds.a``. Most functions
are self-explanatory. Have a look at ``src/keyledsctl.c`` for examples.
Open tickets if you need help. Code documentation should come with time.

Some functions allocate structures but don't have a matching ``*_free_*``
function. When you find some, please open an issue and use stdlib's ``free``
in the meantime.

Using python bindings
---------------------

Python3 bindings are experimental and still incomplete. Pull requests welcome.
To use them, simply build the project and copy ``pykeyleds.so`` into your
python project.

Here is a sample of what works:

.. code-block:: pycon

    >>> import pykeyleds
    >>> dev = pykeyleds.Device('/dev/hidraw1', 1)
    >>> dev.name
    'Gaming Keyboard G410'
    >>> dev.type
    'keyboard'
    >>> dev.protocol
    4

    >>> dev.version
    DeviceVersion(model=c33000000000, serial=35344708, transport=8, protocols=(
        DeviceProtocol(0, product=0xc330, version='U1 v101.2.14', active=True),
        DeviceProtocol(1, product=0xaabc, version='BOTv114.0.7', active=False)
    ))

    >>> dev.features
    (1, 3, 17698, 5, 7680, 17728, 7856, 32864, 193, 6145, 6146, 32896, 32880, 6177)

    >>> dev.leds
    {'modes': KeyBlock('modes', 0x40, nb_keys=2, color=Color(255, 255, 255)),
     'keys': KeyBlock('keys', 0x01, nb_keys=105, color=Color(255, 255, 255))}

    >>> dev.leds['keys'].get_all()
    (KeyColor(KEY_A, id=4, Color(0, 205, 255),
     KeyColor(KEY_B, id=5, Color(0, 205, 255),
     ...
     KeyColor(KEY_RALT, id=230, Color(0, 205, 255),
     KeyColor(KEY_RMETA, id=231, Color(0, 205, 255))

    >>> dev.leds['keys'].set_all_keys(pykeyleds.Color(63, 191, 127))
    >>> dev.commit_leds()

All properties are read once at first access and cached. On the other hand,
methods in the form ``get_*`` query the device at every invocation.

.. _CSS color names: https://www.w3.org/wiki/CSS/Properties/color/keywords
.. _key names: https://github.com/spectras/keyleds/blob/master/src/keyleds/strings.c#L86
.. _keyleds.h: https://github.com/spectras/keyleds/blob/master/include/keyleds.h
