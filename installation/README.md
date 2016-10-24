There are several different ways to configure your system for MoltenGamepad. Choose the one that is right for you. These files are provided as a starting point.

# As Root (NOT RECOMMENDED)

If you run MoltenGamepad as root, you will not need to deal with any additional set up. MoltenGamepad should work out of the box. However, this set up is not secure and is not recommended. We strongly suggest you choose one of the other methods, and as such we do not provide any files to facilitate this set up.

# As a System User (`systemuser`)

This set up creates a new "gamepad" user account that is given only the permissions required to manage your game pad devices. The original game pad devices will be hidden from all other users, so the only controllers your software will ever see are those created by MoltenGamepad.

Note that this means your controllers will be invisible even when MoltenGamepad is not running!

This set up is the only one that can avoid race conditions with software that natively supports controller hotplugging. With this set up, the software never gets any chance to see the original controller device.

# As Your User (`singleuser`)

This set up is specifically for systems with only a single primary user account. In this case, all controller devices become owned by your user account, and this gives you sufficient permission to hide them.

Compared to the "As a System User" method, this set up gracefully falls back to the normal behavior when MoltenGamepad is not running. Only once MoltenGamepad starts are the controllers hidden, and they become visible again once MoltenGamepad exits.

There is the small possibility of failing to hide a recently inserted controller from software that natively supports hotplugging.
