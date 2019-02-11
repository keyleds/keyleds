########################
Keyleds code conventions
########################

Please follow those guidelines if you intend to contribute code to the project.
Though small deviations might be okay, pull requests that completely ignore them
will be closed or flagged as needing a fix before being considered.

File paths
----------

All source files have a mandatory suffix, whhich is:

    * ``.c`` for C source code.
    * ``.cxx`` for C++ source code.
    * ``.h`` for header files (both C and C++).
    * ``.rst`` for ReStructured Text files.
    * ``.yaml`` for YAML data file distributed with project's binaries.

To encourage abstraction, headers are kept apart from their implementation,
having an ``include`` and ``src`` root, respectively.
The structure of those roots are strictly identical. For instance, the
implementation of a header named ``include/foo/bar.h`` must be in a source
file named ``src/foo/bar.c`` (with correct suffix).

Source files are named like this:

    * If it focuses on one class, it has that class' unqualified name as file name,
      with matching case. For instance, ``foo::Bar`` class is defined in
      ``Bar.h`` and implemented in ``Bar.cxx``.
    * Minor classes supporting a main class live in the main class's files.
    * Files that do not define a class use snake_case. For instance,
      ``some_misc_constants.h``.
    * Yes, this means C files always use snake_case.

File Format
-----------

All files must use:

    * UTF8 encoding, no byte order mark.
    * Unix-style line endings (``\n``).
    * Exactly 4 spaces per indent level.
    * Proper licencing boilerplate at the top of all source files.

Code Conventions
----------------

Code uses C++14, C99 and Python3.

Braces:

* Function body and type declaration's opening brace starts on a new line.
* All other block braces start at the end of the relevant line.
* Else keyword goes on same line as closing brace of the ``if`` block.
* Curly braces are mandatory on all control structures, even when they only
  control one statement, even if that statement is on the same line.

Spaces:

* Control structures parenthezises have one space on their outer sides.
* Binary operators have one space on both sides.
* Unary operators have no space.

Pointers, resources and ownership (C++):

* Always use automatic pointers and specifically ``std::unique_ptr``. Do not
  introduce a ``std::shared_ptr`` without an *excellent* reason.
* Raw pointers are non-owning. Reciprocal: raw owning pointers are forbidden.
* Use of new and delete operators is forbidden. One exception: create QObject
  instances with new is allowed if and only if its constructor is passed another
  valid QObject as its parent argument. Such objects are disposed of automatically
  by Qt, so the pointer returned by new is immediately non-owning.
* Prefer references where appropriate.
* Use RAII for all resource management, including external library objects. Either
  specialize std::default_delete or use the custom deleter form of unique_ptr.

Misc:

* Trivial, one-line accessors and simple forwarding methods can live within the class
  declaration. Others must have a separated implementation.
* Use exceptions where appropriate. Use them to handle exceptional situations,
  not expected bad outcomes (eg: user input validation failing is an expected
  outcome, not mandating an exception; a device timeout might throw an exception).
* Do not use RTTI. Project is compiled with ``-fno-rtti`` anyway.
* The directive ``using namespace`` is forbidden. Prefer qualifying types, and
  optionally add ``using`` directives for specific types, in a local scope.

Naming Conventions
------------------

They differ slightly for C and C++.

**Conventions for C++ code:**

    * Namespace names use snake_case, that is, underscore-separated words.
    * Type names normally use CamelCase. That is, a capital letter for each word
      including the first one. This holds for all types. Exceptions are allowed
      for STL-style types, such as ``iterator``, ``foo_list`` or ``bar_type``
      (as the STL is used extensively, those are quite common).
    * Variable names use lowerCamelCase. That is, a capital letter for each word,
      excluding the first one.
    * Non-public data members are prefixed with ``m_``. For instance ``m_foo``
      or ``m_fooBar``. As a reminder, only structures are allowed to have
      public data members, and can only have public data members.
    * Constants are named as any other variable.
    * Functions and methods use lowerCamelCase. That is, a capital letter for each word,
      excluding the first one.

        - Getter methods must have the name of the attribute they get, without
          the `m_` prefix.
        - Setter methods must start with ``set``.
        - For instance, data member ``m_foo`` would have getter ``foo`` and
          setter ``setFoo``.

    * Macros are all uppercase, with underscore_separated words. Their use is
      discouraged. Use constants, static functions, etc. instead.

**Conventions for C code:**

    * Structure names use snake_case. Opaque structures might by typedef'd to
      a CamelCase name. For instance, ``typedef struct foo_bar FooBar;``.
      The rationale is those are used in an object-oriented fashion, so they
      use C++ class naming scheme.
    * Variable names, constants, functions all use snake_case.
    * Macros are all uppercase, with underscore_separated words.

Comments
--------

Comments are mandatory. They should be relevant and to the point. Assume you will
stop developing tomorrow, not touch any code for two years and come back to this
project. That future you is your audience. Roughly, that means:

    * Explain what methods do, when it's not obvious. Explain arguments,
      assumptions, return types, parameter lifetime for references and pointers.
    * Comment all data members. They are the cornerstone of a well designed
      architecture.
    * Insert inline comments in function/methods wherever they might provide a
      readability gain.
    * Provide detailed comment for all method and classes that are part of a
      public API.

