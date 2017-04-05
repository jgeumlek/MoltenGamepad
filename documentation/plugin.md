# Loading Plugins

MoltenGamepad will not load any plugins unless `--load-plugins` is specified.

A plugin will be loaded if its `.so` file is placed in a `plugins/` subdirectory of a config dir.

# Building Plugins

Plugins add drivers to MG to support more interesting hardware and features that the generic drivers cannot. They can optionally be statically built straight into the MG executable, or they can be loaded at runtime as `.so` files in a `plugins/` subdirectory of a config directory.

The first few lines of the MG Makefile control which plugins will be built statically and which will be built externally. The external plugins are placed in `built_plugins/`, but still need to be moved to an appropriate location to be loaded.

These plugins are not sandboxed and can run arbitrary code! Pay close attention to the plugins you load. This is a strong reason for why running MG as root is not recommended. If you wish to disable plugin loading entirely, you may compile MG with `NO_PLUGIN_LOADING` defined.


# Making a Plugin

A driver can be added via filling out a struct of callbacks defined in `plugin/plugin.h`

See the example plugin to get a decent starting point for a plugin.

This API is subject to revision.

A struct ending in `_methods` is a collection of function pointers your driver may call and use.

A struct ending in `_plugin` is a collection of values/function pointers the driver is expected to provide.

To avoid documentation going stale, check the comments within `plugin/plugin.h` for descriptions of the code requirements.

For now, the hack-ish build system imposes these constraints:

* The plugin  must be located in a folder `source/plugin/<plugin name>`.
* A Makefile should have a target `plug.a` that is a static archive of all `.o` files for the plugin.
* If the plugin requires linking with additional libraries, create a file `source/plugin/<plugin name>/ldlibs` that has only a single line: the appropriate linker arguments with a trailing space.   Example: `-lscraw `. Do not put a newline, and do not forget the trailing whitespace.
* To be built externally, the plugin Makefile should support a "plug.so" target.


The name of the plugin can then be added to the variable `MG_BUILT_INS` in the main Makefile of this project to request that it be built and included.
