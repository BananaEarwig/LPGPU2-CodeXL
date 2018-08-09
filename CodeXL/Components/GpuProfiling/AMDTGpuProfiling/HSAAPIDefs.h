//=====================================================================
// Copyright (c) 2012 Advanced Micro Devices, Inc. All rights reserved.
//
/// \author GPU Developer Tools
/// \file $File: //devtools/main/CodeXL/Components/GpuProfiling/AMDTGpuProfiling/HSAAPIDefs.h $
/// \version $Revision: #5 $
/// \brief  This file contains definitions for CL API Functions
//
//=====================================================================
// $Id: //devtools/main/CodeXL/Components/GpuProfiling/AMDTGpuProfiling/HSAAPIDefs.h#5 $
// Last checkin:   $DateTime: 2013/01/09 15:36:14 $
// Last edited by: $Author: chesik $
// Change list:    $Change: 461835 $
//=====================================================================
#ifndef _HSAAPI_DEFS_H_
#define _HSAAPI_DEFS_H_

#include <qtIgnoreCompilerWarnings.h>
#include <AMDTBaseTools/Include/gtIgnoreCompilerWarnings.h>

#include <QString>
#include <QMap>
#include <QList>

#include <TSingleton.h>

#if defined(signals)
    #pragma push_macro("signals")
    #undef signals
    #define NEED_TO_POP_SIGNALS_MACRO
#endif
#include "../HSAFdnCommon/HSAFunctionDefs.h"
#if defined (NEED_TO_POP_SIGNALS_MACRO)
    #pragma pop_macro("signals")
#endif


/// Group IDs for HSA APIs
enum HSAAPIGroup
{
    HSAAPIGroup_Unknown = 0,
    HSAAPIGroup_Agent,
    HSAAPIGroup_CodeObject,
    HSAAPIGroup_Executable,
    HSAAPIGroup_ExtensionsGeneral,
    HSAAPIGroup_ExtensionsFinalizer,
    HSAAPIGroup_ExtensionsImage,
    HSAAPIGroup_ExtensionsSampler,
    HSAAPIGroup_InitShutDown,
    HSAAPIGroup_ISA,
    HSAAPIGroup_Memory,
    HSAAPIGroup_QueryInfo,
    HSAAPIGroup_Queue,
    HSAAPIGroup_Signal
};

/// a type representing a set of HSAAPIGroup values
typedef unsigned int HSAAPIGroups;

/// Singleton class with helpers for dealing with CL function types
class HSAAPIDefs : public TSingleton<HSAAPIDefs>
{
    friend class TSingleton<HSAAPIDefs>;
public:
    /// Gets the OpenCL API function name for the specified HSA_API_Type
    /// \param type the function type whose function name is requested
    /// \return the OpenCL API function name for the specified HSA_API_Type
    const QString& GetHSAPIString(HSA_API_Type type);

    /// Gets the groups for the specified function type
    /// \param type the function type whose groups are requested
    /// \return the groups for the specified function type
    HSAAPIGroups GetHSAAPIGroup(HSA_API_Type type);

    /// Gets the HSA_API_Type value for the given function name
    /// \param name the function name whose HSA_API_Type value is requested
    /// \return the HSA_API_Type value for the given function name
    HSA_API_Type ToHSAAPIType(const QString& name);

    /// Gets the group name from the given HSAAPIGroup value
    /// \param group the HSAAPIGroup whose name is requested
    /// \return the group name from the given HSAAPIGroup value
    const QString GroupToString(HSAAPIGroup group);

    /// Is this an HSA function string:
    /// \param apiFunctionName the name of the function
    /// \return true iff the string represents an HSA function
    bool IsHSAAPI(const QString& apiFunctionName) const { return m_funcNameToHSATypeMap.contains(apiFunctionName); };

    /// Append the list of all HSA functions to the list:
    /// \param list[out] the list to append the HSA functions for
    void AppendAllHSAFunctionsToList(QStringList& list) { list << m_hsaAPIStringsList; };

private:
    /// Initializes the static instance of the HSAAPIDefs class
    HSAAPIDefs();

    /// Add the requested function to the maps:
    void AddFunctionToMap(HSA_API_Type hsaType, const QString& funcName);

    /// Builds the API type to function name map:
    void BuildAPIFunctionNamesMap();

    /// Build the API type to group map:
    void BuildAPIGroupsMap();

    /// List of HSA function names:
    QStringList                      m_hsaAPIStringsList;

    /// Map - HSA API type -> function name:
    QMap<HSA_API_Type, QString>      m_hsaTypeTofuncNameMap;

    /// Map - function name -> hsa api type :
    QMap<QString, HSA_API_Type>      m_funcNameToHSATypeMap;

    /// Map of HSA_API_Type to group id:
    QMap<HSA_API_Type, HSAAPIGroups>  m_HSAAPIGroupMap;

};

#endif // _HSAAPI_DEFS_H_

