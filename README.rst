=======
keyleds
=======

Userspace service for Logitech keyboards.

This project supports all Logitech USB keyboards, though the service is
obviously only useful with those featuring per-key light control.

* G410 Atlas Spectrum is fully tested.
* Other keyboards should work, though some plugins might need a layout
  description file. Please open an `issue`_ and I will make one for you!

Features
--------

The project is split into four components.

The **session service** animates the keyboard in response to various events:

* Dynamic detection of Logitech keyboard as they are plugged in.
* Flexible per-application light settings.

  - Can filter on window title for further refining.

* Multiple light settings can co-exist as layers with transparency support.
* Versatile plugin architecture to add new effects. Comes with:

  - Fixed lights (including key groups)
  - Breathing effect (transparency-based).
  - Wave effect (wavelength, speed and direction fully configurable,
    supports arbitrary color list with transparency).
  - Star effect (number, color list and light duration configurable).

* Support for per-keyboard settings.

The **command-line tool** is intended for power users to script their keyboard:

* Device enumeration.
* Full device information (type, firmware, capabilities, leds, layout, …)
* Individual key led control (set and get).
* Advanced Logitech keyboard settings (game-mode, report rate, …)

Lastly, for developers, the project includes:

* A **library** implementing Logitech keyboards protocol. Written in pure C,
  with no allocations on critical codepaths for optimal performance.
* **Python3 bindings** for the library.

Installing - prepackaged
------------------------

Effort is undergoing to provide official packages for various distributions.

It is now available from the `official PPA`_ for Ubuntu Xenial / Zesty:

.. code-block:: bash

    sudo add-apt-repository ppa:spectras/keyleds
    sudo apt-get update
    sudo apt-get install keyleds

After the install is complete, you have to re-plug your keyboard so
device permissions are applied.

If your distribution is not listed, you need to install manually.

Installing - manually
---------------------

Compiling
~~~~~~~~~

Building keyleds requires the following dependencies to be installed:

* ``cmake``
* ``Qt4 core`` development files
* ``X11`` development files, including ``XInput``.
* ``libc`` development files
* ``libudev`` development files
* ``libxml2`` development files
* ``libyaml`` development files
* ``Qt4`` dbus development files *(optional, required for DBus support)*
* ``python3`` development files *(optional, used for python bindings)*
* ``cython3`` *(optional, used for python bindings)*

The following one-liner should get you up and running on debian systems:

.. code-block:: bash

    sudo apt-get install build-essential cmake linux-libc-dev libqt4-dev libudev-dev libx11-dev libxi-dev libxml2-dev libyaml-dev

Then build the project with the following command:

.. code-block:: bash

    cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

You may then install it with a good old:

.. code-block:: bash

    sudo make install

Dealing with device permissions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default, the keyboard device is not usable by any non-root user.
This means you must either:

* Run this project as root. This means the cli tool, or your own program
  using the library.
* Configure your system to make the device accessible to other users.
  On udev-based systems, you can copy ``logitech.rules`` into
  ``/etc/udev/rules.d/90-logitech-plugdev.rules`` to automatically grant
  access to members of the ``plugdev`` group. Beware that this makes
  it possible for those users to spy on some keyboard presses.

Using the service
-----------------

If you used automatic installation, the service will start automatically when
you open an X session. You can enable this behavior with manual installation
with the following command:

.. code-block:: bash

    ln -s /usr/share/keyledsd/keyledsd.desktop $HOME/.config/autostart/

The service reads its configuration file from those paths, taking whichever comes first:

* `${HOME}/.config/keyledsd.conf`
* Any path from `${XDG_CONFIG_DIRS}`
* `/etc/keyledsd.conf`

If you used automatic installation, `/etc/keyledsd.conf` is provided. You can
either modify it, or copy it to your home folder to override the global one.

The sample `keyledsd.conf`_ shows examples of all plugins and a few common ways
to create plugin stacks for cool effects.


Using the command-line tool
---------------------------

The command-line tool and the service are compatible: You may use the command line
tool even when the service is in control of the keyboard. Note however that setting
key lights is useless then as the service will restore them right away.

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

If using the automatic install, install the development package first.
It should be called ``keyleds-dev``. Otherwise, manual mode installs
development files by default.

In your project, simply include `keyleds.h`_, and link with ``-lkeyleds``.
Most functions are self-explanatory. Have a look at
``keyledsctl/src/keyledsctl.c`` for examples.
Open tickets if you need help.

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

.. _issue: https://github.com/spectras/keyleds/issues
.. _official PPA: https://launchpad.net/~spectras/+archive/ubuntu/keyleds
.. _keyledsd.conf: https://github.com/spectras/keyleds/blob/master/keyledsd/keyledsd.conf.sample
.. _CSS color names: https://www.w3.org/wiki/CSS/Properties/color/keywords
.. _key names: https://github.com/spectras/keyleds/blob/master/libkeyleds/src/strings.c#L86
.. _keyleds.h: https://github.com/spectras/keyleds/blob/master/libkeyleds/include/keyleds.h
