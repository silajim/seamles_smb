
#ifndef SEAMLESS_SMB_LIB_EXPORT_H
#define SEAMLESS_SMB_LIB_EXPORT_H

#ifdef SEAMLESS_SMB_LIB_STATIC_DEFINE
#  define SEAMLESS_SMB_LIB_EXPORT
#  define SEAMLESS_SMB_LIB_NO_EXPORT
#else
#  ifndef SEAMLESS_SMB_LIB_EXPORT
#    ifdef seamless_smb_lib_EXPORTS
        /* We are building this library */
#      define SEAMLESS_SMB_LIB_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define SEAMLESS_SMB_LIB_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef SEAMLESS_SMB_LIB_NO_EXPORT
#    define SEAMLESS_SMB_LIB_NO_EXPORT 
#  endif
#endif

#ifndef SEAMLESS_SMB_LIB_DEPRECATED
#  define SEAMLESS_SMB_LIB_DEPRECATED __declspec(deprecated)
#endif

#ifndef SEAMLESS_SMB_LIB_DEPRECATED_EXPORT
#  define SEAMLESS_SMB_LIB_DEPRECATED_EXPORT SEAMLESS_SMB_LIB_EXPORT SEAMLESS_SMB_LIB_DEPRECATED
#endif

#ifndef SEAMLESS_SMB_LIB_DEPRECATED_NO_EXPORT
#  define SEAMLESS_SMB_LIB_DEPRECATED_NO_EXPORT SEAMLESS_SMB_LIB_NO_EXPORT SEAMLESS_SMB_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef SEAMLESS_SMB_LIB_NO_DEPRECATED
#    define SEAMLESS_SMB_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* SEAMLESS_SMB_LIB_EXPORT_H */
