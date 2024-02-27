Due to various compatibility issues, some objects are replaced with those from different Pd/ELSE versions.

### adsr~.c

Modify the `[adsr~]` object from ELSE 1.0 beta44 to have the same API as ELSE 1.0 RC-12:

- Taking integer (0~127) velocity values instead of float (0~1) ones.
- Default flag being `-log` instead of `-lin`.

Modifications have been made to the source file. This directory stores the original reference.

### sigmund~.c

Use the `[sigmund~]` from Pd 0.54-1.

The source file in the submodule remains version 0.52-0, which needs to be overwritten manually.