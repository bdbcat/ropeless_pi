#define API_VERSION       "1.16"

#define PLUGIN_VERSION_MAJOR 2
#define PLUGIN_VERSION_MINOR 1
#define PLUGIN_VERSION_PATCH 0
#define PLUGIN_VERSION_TWEAK 0

#define OPENGL_FOUND
#ifdef OPENGL_FOUND
#define ocpnUSE_GL 1
#endif

#ifdef __MINGW32__
// wxWidgets is build using unicode, make sure we also use it.
#ifndef wxUSE_UNICODE
#define wxUSE_UNICODE 1
#endif

#ifndef UNICODE
#define UNICODE 1
#endif

#ifndef _UNICODE
#define _UNICODE 1
#endif

#endif  // __MINGW32__


#ifdef _MSC_VER 
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#define MAKING_PLUGIN 1

#ifndef DECL_IMP
// Walk around strange (buggy?) DECL_IMP definition in ocpn_plugin.h
#define DECL_IMP
#endif
