=================
LUA engine plugin
=================

This plugin allows writing effects in LUA.

Architecture notes
------------------

As all plugins, the front-end architecture exposed to keyleds through the
plugin mechanism consists of:

    * The main entry point, auto-created with ``KEYLEDSD_EXPORT_PLUGIN macro``.
      It handles the lifetime of…
    * :class:`LuaPlugin` faux-singleton. It loads scripts and manages instances
      of…
    * :class:`LuaEffect`. This is the main class that the service interfaces
      with. It is designed as a Mediator, implementing the Effect interface
      to communicate with keyleds service and the Controller interface to
      control the behavior of LUA environment.

      As such, :class:`LuaEffect` acts as the unique communication point between
      the lua environment and keyleds service.

The back-end side of the plugin implements integration with the LUA engine,
designed to be compatible with both LuaJIT and mainstream LUA (5.2+). It
consists of:

    * LUA Wrapper objects, implemented as table of static methods.
    * Helper libraries, namely ``lua_types`` that uses templates and sfinae to
      ease the manipulation of wrapper objects and ``lua_common`` that groups
      common idioms.
    * The :class:`Environment` wrapper, acting as the single entry point into
      the LUA back-end.

To that effect, the following rules must be followed when working on the LUA
engine plugin:

    * LUA engine files all begin with ``lua_`` prefix.
    * From keyleds, they are only allowed to #include other ``lua_``-prefixed
      files and the single keyleds header of the object they wrap.
    * No other unit is allowed to #include a ``lua_``-prefixed file.
    * The ``Environment`` unit is the only exception: it is not prefixed,
      so it is allowed to #include it from the outside, yet it follows the
      same rules as LUA engine files, and is allowed for inclusion from engine
      files.

Following those rules guarantee complete decoupling of the LUA environment,
except for fine-grained, easily-located coupling of each wrapper to its wrapped
object.
