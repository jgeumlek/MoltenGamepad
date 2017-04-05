#MoltenGamepad Generic Device Driver Documentation

##Intro

MoltenGamepad can translate button and axis events from any evdev device, such those in `/dev/input/event#`. To do this, MoltenGamepad needs some basic information on how to handle these devices. Creating a Generic Driver is an easy way to get enable basic functionality of MoltenGamepad for a device.

A configuration file has four main parts/purposes:

1. Specify what criteria a device must meet for this generic driver to apply.
2. Specify information about this generic driver, including options
3. Specify raw events read from the device and appropriate names to be exposed for these events.
4. Specify any event name aliases the device needs.

Default event mappings are NOT handled by these configs. That is to be handled by loading the desired profile at start up.

The format for these config files is designed to be simple yet flexible. There is a lot of flexibility required for generic drivers, so the full spec can be daunting. Look at the example files to get a good idea of how this all works.

## Generic Driver Config File

A generic driver can be specified by creating a `<filename>.cfg` file in a `gendevices` config directory.

Such a file has the following form, where `#` starts comments. All `<values>` MUST be placed in quotes when spaces/punctuation are present. (Limited escaping applies, `\"` for a literal quote, `\\` for a literal backslash)

This is the full spec. See the examples for a simpler view.

    # 1. Device Matching
    #specify devices details to match against. If any match, the device will be claimed by this driver.
    #see the section "Matching Devices" for more details
    [<Device Match>]
    [<Additional Device Match>]
    [<Additional Device Match>]
    
    # 2. Driver Info
    name= <driver name> #set the name of this driver seen in MoltenGamepad
    devname= <device name prefix> #set the name assigned to identified devices
    
    #    Driver Settings (if omitted, the default values shown are used)
    
    ##Should we grab the device exclusively, preventing events from being seen by others?
    ##This hides incoming events, but it DOES NOT hide the device node. It merely makes it appear silent.
    #exclusive="false" 
    
    ##Should we block all permissions after opening, preventing others from even opening the device?
    ##This is generally effective at making software ignore the original device entirely.
    ##Note: requires active user to be the device node owner.
    #change_permissions="false" 
    
    ##Should we coalesce all identified devices into one virtual input source?
    ##(Helpful for annoying devices that create a dead duplicate node,
    ## or if you know you want to combine all devices of this type)
    #flatten="false"

    ##Should we try to forward rumble (force-feedback) events?
    ##This setting has no effect unless MoltenGamepad was run with rumble enabled.
    ##You can not enable both rumble and flatten.
    #rumble="false"
    
    
    ##How many input sources should we generate for this device?
    ##Useful for device nodes that may represent two or more controllers together.
    #split = 1
    
    ##Specify the device types. This is used for allocating output slots.
    ##Can be an any identifier string, but the following are special:
    ## "gamepad" - a normal gamepad device. (default device type)
    ## "keyboard" - forces the keyboard output slot to be used.
    ## Anything else leads to MG attempting to allocate no more than one of that
    ## type in each slot.
    #device_type = "gamepad"
    #
    ##If split is greater than one, each split can be given a different device type.
    #1.device_type = "gamepad"
    #2.device_type = "gamepad"
    #...

    ##Should we forcefully listen to the root "gamepad" profile to get our event mappings?
    ##By default, we listen non-forcefully: only when the driver has "gamepad" type devices.
    ##Reminder: The default device type is "gamepad"!
    #gamepad_subscription = false
    
    # 3. Specify Events
    
    #begin event identification, repeat for each event
    <event code name> = <name>,<description>
    
    #<event code name> is the evdev event name, such as btn_left or key_esc
    # NOTE: If you need to specify an event by number, use the following notation:
    #       key(306)          [the same as btn_c]
    #       abs(1)            [the same as abs_y]
    #<name> is the name of the event seen in MoltenGamepad
    #<description> is the description of the event seen in MoltenGamepad
    
    #If split is greater than one, prefix the <event code name> with the desired subdevice number
    #followed by a dot ( . ), 
    1.<event code name> = <name>,<description>
    
    # 4. Specify aliases
    
    #alias external_name local_name
    
    #local_name must be one the declared event <name>s
    #Aliases are for convenience, and for handling being subscribed to a different profile.
    

If multiple `[<Device Match>]` declarations are in a row, they are presumed to be alternative devices that should be grouped under the same driver. If they are not in a row, it is assumed that the user is beginning a new generic driver specification. (Yes, multiple generic drivers can be specified in one file.)

#Matching Devices

The most basic way to match a device is via it's reported name string. Putting it in quotes is recommended.

    ["Sony Computer Entertainment Wireless Controller"]

Other traits can be specified in match declaration using `<field>=` notation. Available fields to match against are 

* `name` : the reported name string
* `vendor` : the hexadecimal vendor id
* `product` : the hexadecimal product id
* `uniq` : a (potentially missing) uniquely identifying string for the device
* `driver` : the name of the linux driver for this event device
* `events` : can be one of `superset`, `subset`, or `exact`. Superset matches if the device contains all the events of this generic driver. Subset matches if the device has no reported events not listed in this driver, but it must have at least one event in common with this driver. Exact requires both of the conditions of superset and subset to hold. That is, the device has exactly the events of this driver; no more, no less.
* `order` : A positive integer indicating the order to consider matches. When a device has multiple matches, the one with the lowest `order` is taken. If not specified, a match has an `order` of `1`, which is the lowest allowed value. Ties are broken based on whichever match is read first.

Putting this together results in a match line that may resemble the following.

    [name="Microsoft X-Box 360 pad" vendor=045e product=028e]

The first field is assumed to be the name. Thus the following is valid as long as ambiguity is avoided.

    ["Microsoft X-Box 360 pad" vendor=045e product=028e]

When a match line specifies multiple fields, a device is considered a match only if ALL specified fields are matched.

Remember that multiple match lines can be used, in which a device matches the driver if ANY individual match line is matched.

The `order` property of matches is for cases where a device might match two separate generic drivers.

## Finding Event Codes and Name Strings

The `evtest` utility (not included with MoltenGamepad) is incredibly useful for this. Run it to see all devices you currently have read access to. Select a device, and it will print out events as they happen. Interact with your input device, and make note of the events generated.

Near the top of the `evtest` output will be the vendor and product ids as well.


## Example 1
    
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

## Example 2

Here is a longer file, showing a configuration for a Dualshock 4 controller.


    ["Sony Computer Entertainment Wireless Controller"]
    
    name = "dualshock4"
    devname = "ds_"
    exclusive = "true"
    #since the original device is also a gamepad by most standards, we need change_permissions in
    #order to hide the original DS4 devices.
    #Reminder: change_permissions requires a udev rule to make the current user the owner of the device
    change_permissions = "true"
    flatten = "false"
    rumble = "true"
    
    
    btn_tl2 = "share", "Share Button"
    btn_tr2 = "options", "Options Button"
    
    btn_thumbl = "touchpad_press", "Touchpad click action"
    btn_select = "l3", "Left stick click"
    btn_start = "r3", "Right stick click"

    abs_hat0x = "leftright", "D-pad left/right axis"
    abs_hat0y = "updown", "D-pad up/down axis"
    
    btn_east = "cross", "The cross (X) button"
    btn_c = "circle", "The circle button"
    btn_north = "triangle", "The triangle button"
    btn_south = "square", "The square button"
    
    btn_west = "l1", "The left top trigger"
    btn_z = "r1", "The right top trigger"
    
    btn_tl = "l2", "The left lower trigger"
    abs_rx = "l2_axis", "The left lower trigger analog values"
    
    btn_tr = "r2", "The right lower trigger"
    abs_ry = "r2_axis", "The right lower trigger analog values"
    
    abs_x = "left_x", "The left stick x axis"
    abs_y = "left_y", "The left stick y axis"
    
    abs_z = "right_x", "The right stick x axis"
    abs_rz = "right_y", "The right stick y axis"
    
    #The default device type is "gamepad", and
    #this leads to the driver subscribing to the gamepad profile.
    #But the gamepad profile uses some different event names.
    #So these aliases let us inherit the appropriate event mappings.
    
    alias first cross
    alias second circle
    alias third square
    alias fourth triangle
    
    alias thumbl l3
    alias thumbr r3
    
    alias start options
    alias select share
    
    alias tr r1
    alias tr2_axis_btn r2
    alias tl l1
    alias tl2_axis_btn l2
    
    alias tr2_axis r2_axis
    alias tl2_axis l2_axis

## More Examples

See the [MG-Files repo](https://github.com/jgeumlek/MG-Files) for more contributed generic driver cfgs.
