An optional configuration file `moltengamepad.cfg` can be created in the config directory.

Comments can be included in the file via `#`.

#Location

The XDG spec is followed, using `$XDG_CONFIG_HOME` and `$XDG_CONFIG_DIRS`. Only the first `moltengamepad.cfg` discovered is used.

By default this means the following locations ordered by preference:

1. `~/.config/moltengamepad/moltengamepad.cfg`
2. `/etc/xdg/moltengamepad/moltengamepad.cfg`

#Setting options

Most long-form command line arguments can be specified in this file instead. Compared to the commandline arguments, the following changes are required:

* Instead of hyphens, underscores are used
* True or false values are assigned instead of negation prefixes (i.e. `enumerate=false` instead of `no_enumerate`)

The `daemon` and `pidfile` settings can not be specified in this file. They are exclusively commandline arguments.

#Loading Profiles at Start-up

Profile files can be loaded at the start of MoltenGamepad by specifying them in the config file.

They are loaded after drivers are initialized but before any devices are added. As such, these profiles can only affect driver-level profiles.

    load profiles from "<filename>"

File paths are relative to the profiles directory.

#Command Line Arguments

When an option is expressed both in the config file and in the arguments, the value implied in the command line arguments takes precedence.

#Example

    #useful settings
    mimic_xpad = true
    make_fifo = true
    
    #load preferred default mappings
    load profiles from default_map

This of course requires a file `default_map` in the profiles directory


