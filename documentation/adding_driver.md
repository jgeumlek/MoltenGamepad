
A driver can be added via filling out a struct of callbacks defined in `plugin/plugin.h`

See the steam controller driver for a simple driver.

This API is subject to revision.

A struct ending in `_methods` is a collection of function pointers your driver may call and use.

A struct ending in `_plugin` is a collection of values/function pointers the driver is expected to provide.

To avoid documentation going stale, check the comments within `plugin/plugin.h` for descriptions of the code requirements.

For now, the hack-ish build system imposes these constraints:

* The plugin  must be located in a folder `source/plugin/<plugin name>`.
* A Makefile should have a target `plug.a` that is a static archive of all `.o` files for the plugin.
* If the plugin requires linking with additional libraries, create a file `source/plugin/<plugin name>/ldlibs` that has only a single line: the appropriate linker arguments with a trailing space.   Example: `-lscraw `. Do not put a newline, and do not forget the trailing whitespace.


The name of the plugin can then be added to the variable `MG_BUILT_INS` in the main Makefile of this project to request that it be built and included.

In the future, these plugins will be optionally built as dynamically linked libraries.


