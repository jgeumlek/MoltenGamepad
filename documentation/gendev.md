#MoltenGamepad Generic Device Driver Documentation

A generic driver can be specified by creating a `<filename>.cfg` file in a `gendevices` config directory.

Such a file has the following form, where `#` starts comments. All `<values>` MUST be placed in quotes when spaces/punctuation are present. (Limited escaping applies, `\"` for a literal quote, `\\` for a literal backslash)

    #specify devices details to match against. If any match, the device will be claimed by this driver.
    #see the section "Matching Devices" for more details
    [<Device Match>]
    [<Additional Device Match>]
    [<Additional Device Match>]
    
    name= <driver name> #set the name of this driver seen in MoltenGamepad
    devname= <device name prefix> #set the name assigned to identified devices
    
    #optional settings
    
    ##Should we grab the device exclusively, preventing events from being seen by others?
    #exclusive="false" 
    
    ##Should we block all permissions after opening, preventing others from even opening the device?
    ##Note: requires active user to be the device node owner.
    ##(This feature is still experimental)
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

Putting this together results in a match line that may resemble the following.

    [name="Microsoft X-Box 360 pad" vendor=045e product=028e]

The first field is assumed to be the name. Thus the following is valid as long as ambiguity is avoided.

    ["Microsoft X-Box 360 pad" vendor=045e product=028e]

When a match line specifies multiple fields, a device is considered a match only if ALL specified fields are matched.

#Finding Event Codes and Name Strings

The `evtest` utility (not included with MoltenGamepad) is incredibly useful for this. Run it to see all devices you currently have read access to. Select a device, and it will print out events as they happen. Interact with your input device, and make note of the events generated.

Near the top of the `evtest` output will be the vendor and product ids as well.


#Example 1
    
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

#Example 2

Here is a longer file, showing a configuration for a Dualshock 4 controller.


    ["Sony Computer Entertainment Wireless Controller"]
    
    name = "dualshock4"
    devname = "ds_"
    exclusive = "true"
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
    
    # ALIASING: a feature to allow referencing events through standardized names.
    #           These special aliases also allow this driver to inherit
    #           some default control mappings
    
    alias primary cross
    alias secondary circle
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
