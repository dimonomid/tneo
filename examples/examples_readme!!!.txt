
Before trying to build examples, please read the following page carefully:
https://dfrank.bitbucket.io/tneokernel_api/latest/html/building.html

In short, you need to copy configuration file in the tneo directory to
build it.  Each example has `tn_cfg_appl.h` file, and you should either create
a symbolic link to this file from `tneo/src/tn_cfg.h` or just copy this
file as `tneo/src/tn_cfg.h`.

