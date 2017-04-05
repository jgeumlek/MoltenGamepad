# Rumble and the Uinput Bug

Rumble processing in MoltenGamepad can be enabled with the `--rumble` flag.

Current kernels have a bug in their uinput module which makes it unsafe to destroy virtual devices with uploaded force-feedback effects.

First, a quick explanation of how a game sends force-feedback events in the Linux kernel:

* First, upload a description of the desired effect, such as strength or duration.
* Later, send commands to play a previously uploaded effect
* Finally, erase the uploaded effect when it is no longer needed.

The uinput module has to forward these steps to MoltenGamepad for processing. When a device has non-erased uploaded effects, the destroy routine of uinput attempts gets caught in a spin lock. This spin lock causes problems in the linux kernel, and appears to require a reboot.

MoltenGamepad attempts to work around this bug by simply refusing to exit and refusing to destroy its uinput devices if it sees some effects have not been erased. Though this works for most cases, the small chance of a race condition or of MoltenGamepad being killed ungracefully can still lead to this issue!

As such, it is not active by default. Now that you know the risks, you can decide for yourself. (i.e. make sure you don't forcefully kill MoltenGamepad when it talks about remaining force feedback effects!)
