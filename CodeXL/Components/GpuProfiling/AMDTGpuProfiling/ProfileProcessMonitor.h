//=============================================================
// (c) 2012 Advanced Micro Devices, Inc.
//
/// \author GPU Developer Tools
/// \version $Revision: #9 $
/// \brief  This file contains ProfileProcessMonitor which monitors the CodeXLGpuProfiler process to know when it terminates.
//
//=============================================================
// $Id: //devtools/main/CodeXL/Components/GpuProfiling/AMDTGpuProfiling/ProfileProcessMonitor.h#9 $
// Last checkin:   $DateTime: 2015/10/06 11:59:06 $
// Last edited by: $Author: chesik $
// Change list:    $Change: 542961 $
//=============================================================
#ifndef _PROFILE_PROCESS_MONITOR_H
#define _PROFILE_PROCESS_MONITOR_H

#include <AMDTOSWrappers/Include/osThread.h>

/// Thread class that monitors a process for the GPU Profiler
class ProfileProcessMonitor : public osThread
{
public:

    /// Enum defining the reason that CodeXLGpuProfiler is being called
    enum ProfileServerRunType
    {
        ProfileServerRunType_Unknown = -1,     ///< CodeXLGpuProfiler is called for unknown reason
        ProfileServerRunType_Profile = 0,      ///< CodeXLGpuProfiler is called to profile an application
        ProfileServerRunType_GenSummary = 1,   ///< CodeXLGpuProfiler is called to generate summary pages
        ProfileServerRunType_GenOccupancy = 2, ///< CodeXLGpuProfiler is called to generate an occupancy display page
        PerfStudioServerRunType_Application,   ///< PerfStudio is running an application process monitoring
    };

    /// Initializes a new instance of the ProfileProcessMonitor class
    /// \param launchedProcessId the process id of the process to monitor
    /// \param runType the process run type (profile or pages generation)
    ProfileProcessMonitor(osProcessId launchedProcessId, ProfileServerRunType runType, bool showProgress = true);

    /// Destroys an instance of the ProfileProcessMonitor class
    virtual ~ProfileProcessMonitor();

    /// Overrides osThread's entryPoint -- the thread function
    /// \return zero
    virtual int entryPoint();

    /// Overrides osThreads beforeTermination -- kills the monitored process if it is still alive
    virtual void beforeTermination();

private:
    /// Disallow use of my default constructor:
    ProfileProcessMonitor();

    /// Updates the status bar while profiling
    /// \param status the status message
    /// \param halfSeconds the number of half seconds elapsed while profiling
    void updateStatus(const gtString& status, unsigned long halfSeconds);

    osProcessId m_launcherProcessId;  ///< The process Id of the launcher process
    gtString    m_strProgressMessage; ///< The progress message to show while the process is running
    long        m_exitCode;           ///< The process exit code
    ProfileServerRunType m_runType;   ///< The process run type
};

#endif //_PROFILE_PROCESS_MONITOR_H
