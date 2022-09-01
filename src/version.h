#ifndef __VERSION_H_
#define __VERSION_H_

#define _STR(x)                 #x
#define STR(x)                  _STR(x)
#define VTX_VERSION_MAJOR       0 // increment when a major release is made (big new feature, etc)
#define VTX_VERSION_MINOR       1 // increment when a minor release is made (small new feature, change etc)
#define VTX_VERSION_PATCH_LEVEL 0 // increment when a bug is fixed
#define VTX_VERSION_STRING      STR(VTX_VERSION_MAJOR) "." STR(VTX_VERSION_MINOR) "." STR(VTX_VERSION_PATCH_LEVEL)

#endif /* __VERSION_H_ */