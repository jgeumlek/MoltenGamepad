#MoltenGamepad Generic Device Driver Documentation

A generic driver can be specified by creating a `<filename>.cfg` file in the `gendevices` config directory.

Such a file has the following form, where `#` starts comments. All `<values>` MUST be placed in quotes when spaces/punctuation are present. (Limited escaping applies, `\"` for a literal quote, `\\` for a literal backslash)

    #specify device name strings to match against. If any match, the device will be claimed by this driver.
    [<Device Name String>]
    [<Additional Device Name>]
    [<Additional Device Name>]
    
    name= <driver name> #set the name of this driver seen in MoltenGamepad
    devname= <device name prefix> #set the name assigned to identified devices
    
    #optional settings
    
    ##Should we grab the device exclusively, preventing events from being seen by others?
    #exclusive="false" 
    
    ##Should we coalesce all identified devices into one virtual input source?
    ##(Helpful for annoying devices that create a dead duplicate node,
    ## or if you know you want to combine all devices of this type)
    #flatten="false"
    
    ##How many input sources should we generate for this device?
    #split = 1
    
    #begin event identification, repeat for each event
    <event code name> = <name>,<description>
    
    #<event code name> is the evdev event name, such as btn_left or key_esc
    # NOTE: numeric codes not allowed! Must be event names.
    #<name> is the name of the event seen in MoltenGamepad
    #<description> is the description of the event seen in MoltenGamepad
    
    #If split is greater than one, prefix the <event code name> with the desired subdevice number
    #followed by a dot ( . ), 
    1.<event code name> = <name>,<description>
    

If multiple `[<device name string>]` declarations are in a row, they are presumed to be alternative names that should be grouped under the same driver. If they are not in a row, it is assumed that the user is beginning a new generic driver specification. (Yes, multiple generic drivers can be specified in one file.)

#Finding Event Codes and Name Strings

The `evtest` utility (not included with MoltenGamepad) is incredibly useful for this. Run it to see all devices you currently have read access to. Select a device, and it will print out events as they happen. Interact with your input device, and make note of the events generated.


#Example
    
Here is a short example using the split functionality:

    ["AlpsPS/2 ALPS GlidePoint"]
    
    name = "touchpad"
    devname = "pad"
    exclusive = "true"
    flatten = "true"
    split = 2

    1.btn_left = "hit","Player 1 action."
    2.btn_right = "hit","Player 2 action."
    
This creates a driver named `touchpad`. When an `AlpsPS/2 ALPS GlidePoint` device is found, two input sources are made, `pad1` and `pad2`. Each will have an event named `hit`, mappable directly (such as `pad1.hit = ...`) or via the driver profile to affect both (`touchpad.hit = ...`).

Splitting is useful for arcade panels and some controller hubs, which can appear as a single event device.
