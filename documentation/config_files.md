There are two separate types of global options for MoltenGamepad one might wish to configure:

1. Start-up options that cannot be changed later.
2. Dynamic global options normally accessible via the `set` command.

# Start-up Options: `moltengamepad.cfg`
An optional configuration file `moltengamepad.cfg` can be created in the config directory. This file is used to store information about how MoltenGamepad should behave. The contents are similar to the various command line arguments that can be passed to MoltenGamepad, and are unable to be changed after MoltenGamepad has started.

Comments can be included in the file via `#`.

A sample .cfg can be printed out using the `--print-cfg` option. This sample .cfg contains all available options to be set.

## Location

The XDG spec is followed, using `$XDG_CONFIG_HOME` and `$XDG_CONFIG_DIRS`. Only the first `moltengamepad.cfg` discovered is used.

By default this means the following locations ordered by preference:

1. `~/.config/moltengamepad/moltengamepad.cfg`
2. `/etc/xdg/moltengamepad/moltengamepad.cfg`

## Setting options

Most long-form command line arguments can be specified in this file instead. Compared to the commandline arguments, the following changes are required:

* Instead of hyphens, underscores are used
* True or false values are assigned instead of negation prefixes (i.e. `enumerate=false` instead of `no_enumerate`)

The `daemon`,`pidfile`, and `stay-alive` settings can not be specified in this file. They are exclusively commandline arguments.

The full list of available options can be seen by running

    moltengamepad --print-cfg

## Loading Profiles at Start-up

Profile files can be loaded at the start of MoltenGamepad by specifying them in the config file.

They are loaded after drivers are initialized but before any devices are added. As such, these profiles can only affect driver-level profiles.

    load profiles from "<filename>"

File paths are relative to the profiles directory.

## Command Line Arguments

When an option is expressed both in the config file and in the arguments, the value implied in the command line arguments takes precedence.

## Example

    #useful settings
    mimic_xpad = true
    make_fifo = true
    
    #load preferred default mappings
    load profiles from default_map

This of course requires a file `default_map` in the profiles directory

# Global Options: `options/*.cfg`

MoltenGamepad has a second notion of global options, those that can be changed while MoltenGamepad is running. They are separated into categories, and these categories can be displayed with the `print options` command.

These option categories are initialized with a matching `.cfg` file in the `/options/` subdirectory of the config directory. Similar to the above, `$XDG_CONFIG_DIRS` is respected.

## Example

For this example, we wish to set the option `auto_assign` in the `slots` category to `true`. Normally this would require the command `set slots auto_assign = true` to be entered after MoltenGamepad has started.

Instead, one can create the file `options/slots.cfg` with the following contents:

    auto_assign = true

Note how the file name matches the category of the options being set.
