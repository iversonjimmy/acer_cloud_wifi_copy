//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_DYNAMIC_H__
#define __VPL_DYNAMIC_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for dynamic linking loader operations.
/// Please see @ref VPLDynamic.
//============================================================================

#include "vpl_types.h"
#include "vpl__dynamic.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLDynamic VPL Dynamic Linking Loader API
///
/// This API provides an interface to the dynamic linking loader, which allows
/// loading, using, and unloading shared libraries.
/// It is similar to the POSIX.1-2001 dlopen(), dlsym(), and dlclose() functions,
/// but this API does not currently support RTLD_LAZY, RTLD_DEFAULT, RTLD_NEXT,
/// or passing a filename NULL to dlopen (please let us know if you have a need
/// for any of them).
///@{

/// Opaque handle to a loaded dynamic library.
typedef VPLDL__LibHandle VPLDL_LibHandle;

///
/// Loads the specified dynamic library.  All undefined symbols in the library are
/// resolved, and if the library has dependencies on other shared libraries, it
/// will attempt to recursively open those as well.  This function blocks until the
/// operation is complete.  Lazy loading is not currently supported.
///
/// @param[in] filename Null-terminated string specifying the name or path of
///     the file to load.
/// @param[in] global If #VPL_TRUE, symbols found in this library will be
///     visible to subsequently loaded libraries.  Otherwise, the symbols will
///     not be available when resolving references in later libraries.
/// @param[out] handleOut Upon success, the location is set to the handle for
///     the specified dynamic library.
/// @return #VPL_OK Success; @a handleOut will be set to the new handle.
/// @return #VPL_ERR_INVALID @a filename was NULL or @a handleOut was NULL.
/// @return #VPL_ERR_FAIL The operation failed.
int VPLDL_Open(const char* filename,
               VPL_BOOL global,
               VPLDL_LibHandle* handleOut);

///
/// Retrieves the address where @a symbol is loaded in memory (for the specified
/// library).
///
/// @param[in] handle Handle to the dynamic library to search.
/// @param[in] symbol Null-terminated string specifying the symbol to find.
/// @param[out] addrOut Location to store the address of the symbol upon success.
/// @return #VPL_OK Success; @a addrOut will be set to the address of the symbol.
/// @return #VPL_ERR_INVALID @a symbol was NULL or @a addrOut was NULL.
/// @return #VPL_ERR_BADF The handle was invalid or inactive.
/// @return #VPL_ERR_FAIL The symbol was not found.
int VPLDL_Sym(VPLDL_LibHandle handle,
              const char* symbol,
              void** addrOut);
//% Removing from API until a developer asks for it:
//%
//%/// Special values for use with #VPLDL_SymSearch().
//%typedef enum VPLDL_SearchType
//%{
//%    /// Find the first occurrence of the symbol using the default search order.
//%    VPLDL_SEARCH_LIB_DEFAULT,
//%    
//%    /// Find the next occurrence of the symbol after the current library.
//%    VPLDL_SEARCH_LIB_NEXT
//%    
//%} VPLDL_SearchType_t;
//%
//%/// @param[in] searchType Specify #VPLDL_SEARCH_LIB_DEFAULT to find the first
//%///     occurrence of @a symbol using the default search order, or 
//%///     #VPLDL_SEARCH_LIB_NEXT to find the next occurrence after the current library.
//%/// @param[in] symbol Null-terminated string specifying the symbol to find.
//%/// @param[out] addrOut Location to store the address of the symbol upon success.
//%/// @return #VPL_OK Success; @a addrOut will be set to the address of the symbol.
//%/// @return #VPL_ERR_INVALID @a symbol was NULL or @a addrOut was NULL.
//%int VPLDL_SymSearch(VPLDL_SearchType_t searchType,
//%                    const char* symbol,
//%                    void** addrOut);

///
/// Closes the handle to the dynamic library, also unloading the library if there
/// are no other references to it.  If there are other open handles for the
/// library or other loaded libraries are using symbols from this library, it 
/// will not be unloaded until those are also closed.
///
/// @param[in] handle Handle to the dynamic library to close.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_BADF The handle was invalid or inactive.
/// @return #VPL_ERR_FAIL The operation failed.
int VPLDL_Close(VPLDL_LibHandle handle);

///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
