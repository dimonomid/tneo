
Before trying to build examples, please read the following page carefully:
http://dfrank.bitbucket.org/tneokernel_api/latest/html/building.html

In short, you need to copy configuration file in the tneokernel directory to
build it.  Each example has `tn_cfg_appl.h` file, and you should either create
a symbolic link to this file from `tneokernel/src/tn_cfg.h` or just copy this
file as `tneokernel/src/tn_cfg.h`.

