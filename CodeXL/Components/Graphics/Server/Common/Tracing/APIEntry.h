//==============================================================================
/// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file APIEntry.h
/// \brief The APIEntry structure is used to track traced call invocations.
//==============================================================================

#ifndef APIENTRY_H
#define APIENTRY_H

#include <AMDTBaseTools/Include/gtASCIIString.h>
#include "../CommonTypes.h"
#include <string>

enum FuncId : int;

    //-----------------------------------------------------------------------------
    /// List of parameter types used for the API trace parameters.
    /// Data is stored as binary and reconstituted before sending the data back to the client.
    //-----------------------------------------------------------------------------
    enum PARAMETER_TYPE
{
    PARAMETER_POINTER,
    PARAMETER_POINTER_SPECIAL,
    PARAMETER_INT,
    PARAMETER_UNSIGNED_INT,
    PARAMETER_UNSIGNED_CHAR,
    PARAMETER_FLOAT,
    PARAMETER_BOOL,
    PARAMETER_UINT64,
    PARAMETER_SIZE_T,

    // Data types requiring special treatment.
    PARAMETER_GUID,
    PARAMETER_REFIID,
    PARAMETER_DXGI_FORMAT,
    PARAMETER_PRIMITIVE_TOPOLOGY,
    PARAMETER_QUERY_TYPE,
    PARAMETER_PREDICATION_OP,
    PARAMETER_COMMAND_LIST,
    PARAMETER_FEATURE,
    PARAMETER_DESCRIPTOR_HEAP,
    PARAMETER_HEAP_TYPE,
    PARAMETER_RESOURCE_STATES,

    PARAMETER_STRING,
    PARAMETER_WIDE_STRING,

    PARAMETER_ARRAY,
};

//-----------------------------------------------------------------------------
/// Enum containing flags on how to display a return value. Default is decimal
/// Other formats may be needed later such as custom bitfields.
//-----------------------------------------------------------------------------
enum ReturnDisplayType
{
    RETURN_VALUE_DECIMAL,
    RETURN_VALUE_HEX,
};

//-----------------------------------------------------------------------------
/// The type and pointer for a parameter that needs to be stringified.
//-----------------------------------------------------------------------------
struct ParameterEntry
{
    int            mType; ///< Data type
    const void*    mData; ///< Data pointer
};

//-----------------------------------------------------------------------------
/// Amount of memory needed for each parameter.
//-----------------------------------------------------------------------------
static const int BYTES_PER_PARAMETER = 128;

//--------------------------------------------------------------------------
/// The APIEntry structure is used to track all calls that are traced at runtime.
//--------------------------------------------------------------------------
class APIEntry
{
public:
    //--------------------------------------------------------------------------
    /// Constructor used to initialize members of new CallData instances.
    /// \param inThreadId The thread Id that the call was invoked from.
    /// \param inFuncId The function ID of this API call
    /// \param inArguments A string containing the arguments of the invoked call.
    //--------------------------------------------------------------------------
    APIEntry(UINT inThreadId, FuncId inFuncId, const std::string& inArguments);

    //--------------------------------------------------------------------------
    /// Constructor used to initialize members of new CallData instances.
    /// \param inThreadId The thread Id that the call was invoked from.
    /// \param inFuncId The function ID of this API call
    //--------------------------------------------------------------------------
    APIEntry(UINT inThreadId, FuncId inFuncId);

    //--------------------------------------------------------------------------
    /// Virtual destructor since this is subclassed elsewhere.
    //--------------------------------------------------------------------------
    virtual ~APIEntry();

    //--------------------------------------------------------------------------
    /// Use API-specific methods to determine the API name for this entry.
    //--------------------------------------------------------------------------
    virtual const char* GetAPIName() const = 0;

    //--------------------------------------------------------------------------
    /// Append this APIEntry's information to the end of the API Trace response.
    /// \param out The API Trace response stream to append to.
    /// \param inStartTime The start time for the API call.
    /// \param inEndTime The end time for the API call.
    //--------------------------------------------------------------------------
    virtual void AppendAPITraceLine(gtASCIIString& out, double inStartTime, double inEndTime) const = 0;

    //--------------------------------------------------------------------------
    /// Check if this logged APIEntry is a Draw call.
    /// \returns True if the API is a draw call. False if it's not.
    //--------------------------------------------------------------------------
    virtual bool IsDrawCall() const = 0;

    //-----------------------------------------------------------------------------
    /// Convert a numeric return value into a human-readable string.
    /// \param inReturnValue A numeric return value. Could be an HRESULT, or a number.
    /// \param inDisplayType A flag indicating how the return value should be displayed
    /// \param ioReturnValue A string used to write the return value to
    //-----------------------------------------------------------------------------
    void PrintReturnValue(const INT64 inReturnValue, const ReturnDisplayType inDisplayType, gtASCIIString& ioReturnValue) const;

    //--------------------------------------------------------------------------
    /// The ID of the thread that the call was invoked/logged in.
    //--------------------------------------------------------------------------
    UINT mThreadId;

    //--------------------------------------------------------------------------
    /// A string with the call parameters. Each API call is printed with its own format string.
    //--------------------------------------------------------------------------
    gtASCIIString mParameters;

    //--------------------------------------------------------------------------
    /// An instance of the FuncId enum. Will be converted to a string before seen in the client.
    //--------------------------------------------------------------------------
    FuncId mFunctionId;
};

#endif // APIENTRY_H