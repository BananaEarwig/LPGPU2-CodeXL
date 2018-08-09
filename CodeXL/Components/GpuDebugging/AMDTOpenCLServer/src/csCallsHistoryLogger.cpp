//==================================================================================
// Copyright (c) 2016 , Advanced Micro Devices, Inc.  All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file csCallsHistoryLogger.cpp
///
//==================================================================================

//------------------------------ csCallsHistoryLogger.cpp ------------------------------

// Infra:
#include <AMDTBaseTools/Include/gtAssert.h>
#include <AMDTOSWrappers/Include/osApplication.h>
#include <AMDTAPIClasses/Include/apFileType.h>
#include <AMDTServerUtilities/Include/suStringConstants.h>
#include <AMDTServerUtilities/Include/suGlobalVariables.h>

// Local:
#include <src/csStringConstants.h>
#include <src/csOpenCLMonitor.h>
#include <src/csCallsHistoryLogger.h>


// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::csCallsHistoryLogger
// Description: Constructor
// Author:      Yaki Tebeka
// Date:        4/11/2009
// ---------------------------------------------------------------------------
csCallsHistoryLogger::csCallsHistoryLogger(int contextId, apMonitoredFunctionId contextCreationFunc, const gtVector<gtString>* pContextAttribs)
    : suCallsHistoryLogger(apContextID(AP_OPENCL_CONTEXT, contextId), contextCreationFunc, suMaxLoggedOpenCLCalls(), CS_STR_ComputeContextCallsHistoryLoggerMessagesLabelFormat, true)
{
    if (nullptr != pContextAttribs)
    {
        parseContextAttribs(*pContextAttribs);
    }
}


// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::~csCallsHistoryLogger
// Description: Destructor
// Author:      Yaki Tebeka
// Date:        4/11/2009
// ---------------------------------------------------------------------------
csCallsHistoryLogger::~csCallsHistoryLogger()
{
}

// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::onFrameTerminatorCall
// Description: Called when a monitored function, defined as a frame terminator
//              is called. Clears the calls history log.
// Author:      Uri Shomroni
// Date:        18/2/2010
// ---------------------------------------------------------------------------
void csCallsHistoryLogger::onFrameTerminatorCall()
{
    // Clear the log:
    clearLog();
}

// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::calculateHTMLLogFilePath
// Description: Calculates and outputs the HTML log file path.
// Author:      Yaki Tebeka
// Date:        10/11/2009
// Implementation notes:
//   The log file path is:
//   <_logFilesDirectoryPath>\<Application name><OpenCLCallsLog>.txt
// ---------------------------------------------------------------------------
void csCallsHistoryLogger::calculateHTMLLogFilePath(osFilePath& textLogFilePath) const
{
    // Build the log file name:
    gtString logFileName;
    logFileName.appendFormattedString(CS_STR_callsLogFilePath, _contextId._contextId);

    // Set the log file path:
    textLogFilePath = suCurrentSessionLogFilesDirectory();
    textLogFilePath.setFileName(logFileName);

    gtString fileExtension;
    apFileTypeToFileExtensionString(AP_HTML_FILE, fileExtension);
    textLogFilePath.setFileExtension(fileExtension);
}


// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::getHTMLLogFileHeader
// Description:
//   Outputs a string that will be used as the generated HTML log file's header
// Author:      Yaki Tebeka
// Date:        10/11/2009
// ---------------------------------------------------------------------------
void csCallsHistoryLogger::getHTMLLogFileHeader(gtString& htmlLogFileHeader) const
{
    htmlLogFileHeader.makeEmpty();

    // Get the debugged application name:
    gtString applicationName;
    osGetCurrentApplicationName(applicationName);

    // Print the HTML header:
    htmlLogFileHeader += L"<HTML>\n";
    htmlLogFileHeader += L"<head>\n";
    htmlLogFileHeader += L"   <title>OpenCL calls log - ";
    htmlLogFileHeader += applicationName;
    htmlLogFileHeader += L" - generated by CodeXL</title>\n";
    htmlLogFileHeader += L"</head>\n\n";
    htmlLogFileHeader += L"<BODY style=\"font: 12px/16px Courier, Verdana, sans-serif; background-color: EFEFEF;\">\n";
    htmlLogFileHeader += L"<h3>\n";

    htmlLogFileHeader += L"////////////////////////////////////////////////////////////<br>\n";
    htmlLogFileHeader += L"// This File contain an OpenCL calls log<br>\n";

    htmlLogFileHeader += L"// Application: ";
    htmlLogFileHeader += applicationName;
    htmlLogFileHeader += L"<br>\n";

    gtString dateAsString;
    _logCreationTime.dateAsString(dateAsString, osTime::WINDOWS_STYLE, osTime::LOCAL);
    htmlLogFileHeader += L"// Generation date: ";
    htmlLogFileHeader += dateAsString;
    htmlLogFileHeader += L"<br>\n";

    gtString timeAsString;
    _logCreationTime.timeAsString(timeAsString, osTime::WINDOWS_STYLE, osTime::LOCAL);
    htmlLogFileHeader += L"// Generation time: ";
    htmlLogFileHeader += timeAsString;
    htmlLogFileHeader += L"<br>\n";

    gtString contextText = L"// OpenCL Context id:";
    contextText.appendFormattedString(L" %d<br>\n", _contextId._contextId);
    contextText.append(L"// Context created with ").append(apMonitoredFunctionsManager::instance().monitoredFunctionName(m_contextCreationFunc)).append(L"<br>\n");
    contextText.append(m_contextCreationAttribsString);
    htmlLogFileHeader += contextText;

    htmlLogFileHeader += L"// Generated by CodeXL - OpenGL and OpenCL Debugger, Profiler and Memory Analyzer<br>\n";
    htmlLogFileHeader += L"// <A HREF=\"http://gpuopen.com/\" TARGET=\"_blank\">http://gpuopen.com</A><br>\n";
    htmlLogFileHeader += L"////////////////////////////////////////////////////////////<br>\n";
    htmlLogFileHeader += L"</h3>\n";
    htmlLogFileHeader += L"<br>\n\n";
}

// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::getHTMLLogFileFooter
// Description:
//   Outputs a string that will be used as the generated HTML log file's footer
// Author:      Yaki Tebeka
// Date:        10/11/2009
// ---------------------------------------------------------------------------
void csCallsHistoryLogger::getHTMLLogFileFooter(gtString& htmlLogFileFooter) const
{
    htmlLogFileFooter = L"\n</BODY>\n";
    htmlLogFileFooter += L"</HTML>\n";
}


// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::getPseudoArgumentHTMLLogSection
// Description: Inputs a pseudo argument and output an HTML section that represents it.
// Arguments: pseudoParam - The pseudo argument to be logged.
//            htmlLogFileSection - The HTML section that represents the pseudo argument
// Author:      Yaki Tebeka
// Date:        10/11/2009
// ---------------------------------------------------------------------------
void csCallsHistoryLogger::getPseudoArgumentHTMLLogSection(const apPseudoParameter& pseudoArgument, gtString& htmlLogFileSection)
{
    (void)(pseudoArgument); // unused
    (void)(htmlLogFileSection); // unused
    // There are currently no pseudo OpenCL arguments.
}

// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::reportMemoryAllocationFailure
// Description: Reports to my debugger about memory allocation failure.
// Author:      Yaki Tebeka
// Date:        10/11/2009
// ---------------------------------------------------------------------------
void csCallsHistoryLogger::reportMemoryAllocationFailure()
{
    // TO_DO: OpenCL - report a detected error.

    // Perform base class actions:
    suCallsHistoryLogger::reportMemoryAllocationFailure();
}

// ---------------------------------------------------------------------------
// Name:        csCallsHistoryLogger::parseContextAttribs
// Description: Creates a string to be used in the log header from a list of attributes
// Author:      Uri Shomroni
// Date:        27/7/2015
// ---------------------------------------------------------------------------
void csCallsHistoryLogger::parseContextAttribs(const gtVector<gtString>& contextAttribs)
{
    m_contextCreationAttribsString = L"// OpenCL Context attributes:<br>\n";

    int attribsCount = (int)contextAttribs.size();

    if (0 < attribsCount)
    {
        for (int i = 0; attribsCount > i; ++i)
        {
            const gtString& currentAttrib = contextAttribs[i];
            int attribLen = currentAttrib.length();

            if (60 > attribLen)
            {
                m_contextCreationAttribsString.append(L"//&nbsp;&nbsp;&nbsp;").append(currentAttrib).append(L"<br>\n");
            }
            else
            {
                gtString subStr;

                for (int j = 0; attribLen > j; j += 60)
                {
                    int line = (60 <= attribLen - j) ? 59 : attribLen - j - 1;
                    currentAttrib.getSubString(j, j + line, subStr);
                    m_contextCreationAttribsString.append(L"//&nbsp;&nbsp;&nbsp;").append(subStr).append(L"<br>\n");
                }
            }
        }
    }
}

