# System Files

A system-wide installation of MoltenGamepad can offer several benefits. Base devices will *always* be invisible to regular users without having to change permissions, and MoltenGamepad's virtual devices will always be visible. Systemd can make sure a directory is available for MG's fifo, and start it automatically. Run `./install.sh` from this directory to deploy this setup.

You will also need to place the compiled MoltenGamepad binary as `/usr/local/bin/moltengamepad`.
