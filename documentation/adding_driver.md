
A driver can be added via filling out a struct of callbacks defined in `plugin/plugin.h`

See the steam controller driver for a simple driver.

This API is subject to revision.

A struct ending in `_methods` is a collection of function pointers your driver may call and use.

A struct ending in `_plugin` is a collection of values/function pointers the driver is expected to provide.

To avoid documentation going stale, check the comments within `plugin/plugin.h` for descriptions of the code requirements.

For now, the hack-ish build system imposes these constraints:

* All code must be located in a folder `source/plugin/<plugin name>` with no subdirectories.
* A file named `Makefile` should at minimum contain a line
    SRCS:=$(SRCS) <the list of files to be compiled for this plugin, relative the root of this repo>
* That same file can optionally include a line like
    LDLIBS:=$(LDLIBS) <the list of libraries to be linked for this plugin>

The name of the plugin can then be added to the variable `MG_BUILT_INS` in the main Makefile of this project to request.

In the future, these plugins will be optionally built as dynamically linked libraries.


