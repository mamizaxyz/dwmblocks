# Dwmblocks

Mamiza's Custom Build of Dwmblocks (Modular statusbar for dwm written in C).

# Modifying blocks

The statusbar is made from text output from command-line programs. Blocks are added and removed by editing the `blocks.def.h` file.

# Mamiza's build

This build of dwmblocks will default to my scripts in my dotfiles, so if you want it to read your own scripts
edit `blocks.def.h`.

# Signaling changes

You can signal updates to your specific block with `kill -(signum) $(pidof dwmblocks)`, signum is the block's update signal +34.

Note that all the blocks should have their own update signal.

# Clickable modules

This build will send your mouse input to your script as the `$BLOCK_BUTTON` variable.

For this to work you also need the appropriate patch installed on your dwm build.
