//==================================================================================
// Copyright (c) 2016 , Advanced Micro Devices, Inc.  All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file PowerProfilerCore.cpp
///
//==================================================================================

#include <AMDTPowerProfilingMidTier/include/PowerProfilerCore.h>
#include <AMDTPowerProfilingMidTier/include/PPPollingThread.h>
#include <AMDTPowerProfilingMidTier/include/PowerProfilerMidTierUtil.h>

// Power Profiling Backend
#include <AMDTPowerProfileAPI/inc/AMDTPowerProfileApi.h>

// Local Backend Adapter.
#include <AMDTPowerProfilingMidTier/include/IPowerProfilerBackendAdapter.h>
#include <AMDTPowerProfilingMidTier/include/LocalBackendAdapter.h>
#include <AMDTPowerProfilingMidTier/include/BackendDataConvertor.h>

// Remote Backend Adapter.
#include <AMDTPowerProfilingMidTier/include/RemoteBackendAdapter.h>

//++AT:LPGPU2
// Android Backend Adapter
#include <AMDTPowerProfilingMidTier/include/LPGPU2_AndroidBackendAdapter.h>
// The Android-specific polling thread
#include <AMDTPowerProfilingMidTier/include/LPGPU2_PPAndroidPollingThread.h>
// Used to read system time
#include <AMDTOSWrappers/Include/osTimer.h>
// Function status
#include <AMDTPowerProfiling/src/LPGPU2ppFnStatus.h>
//--AT:LPGPU2

// Framework.
#include <AMDTBaseTools/Include/gtAssert.h>
#include <AMDTOSWrappers/Include/osTime.h>
#include <AMDTOSWrappers/Include/osTimeInterval.h>
#include <AMDTBaseTools/Include/gtSet.h>
//++AT:LPGPU2
#include <LPGPU2Database/LPGPU2Database/LPGPU2_db_DatabaseAdapter.h>
#include <chrono>
//--AT:LPGPU2

// C++.
#include <algorithm>
#include <iostream>

#ifndef NULL
    #define NULL 0
#endif


//++AT:LPGPU2
constexpr auto kThreadStopTimeout = std::chrono::milliseconds{10000};
//--AT:LPGPU2

class PowerProfilerCore::Impl
{
public:

    // By default we go with the local adapter.
    // When remote profiling will be supported, we will be able to
    // pass a RemoteBackendAdapter (to be implemented).
    Impl()
        : m_pBEAdapter(new LocalBackendAdapter())
        , m_pPollingThread(NULL)
//++AT:LPGPU2
// We always use the LPGPU2 DB Adapter, even when the profiling session is of
// AMD type since the class inherits from the AMD DB adapters
       , m_pDataAdapter{ new lpgpu2::db::LPGPU2DatabaseAdapter() }
//--AT:LPGPU2
        , m_currentSamplingIntervalMs(0)
        , m_powerProfilingSupportLevel(POWER_PROFILING_LEVEL_UNKNOWN)
    {
//++AT:LPGPU2
// Modified ctor because API to create DB has been updated and we need
// to call another method to actually create the DB other than the
// ctor
      GT_ASSERT_EX(m_pDataAdapter->Initialise(), L"Cannot initialize database adapter")
//--AT:LPGPU2
    }

    ~Impl()
    {
        delete m_pBEAdapter;
        m_pBEAdapter = NULL;

        delete m_pPollingThread;
        m_pPollingThread = NULL;

        delete m_pDataAdapter;
        m_pDataAdapter = NULL;
    }

    void ClearSystemTopologyCache()
    {
        for (PPDevice*& pDevice : m_sysTopologyCache)
        {
            delete pDevice;
            pDevice = NULL;
        }

        m_sysTopologyCache.clear();
    }

    void ClearEnabledCountersCache()
    {
        m_enabledCountersCache.clear();
    }

    // Initialization.
    PPResult InitPowerProfiler(const PowerProfilerCoreInitDetails& initDetails)
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;

        m_initDetails = initDetails;

        // Clear the system topology cache.
        ClearSystemTopologyCache();

        // Clear the enabled counters cache.
        ClearEnabledCountersCache();

        // Reset the sampling interval.
        m_currentSamplingIntervalMs = 0;

        // Create a new backend adapter.
        IPowerProfilerBackendAdapter* pBeAdapter = NULL;

        if (m_initDetails.m_isRemoteProfiling)
        {
            // Create a remote backend adapter.
            RemoteBackendAdapter* pRemoteAdapter = new RemoteBackendAdapter();

            // Set the remote target.
            pRemoteAdapter->SetRemoteTarget(initDetails.m_remoteHostAddr, initDetails.m_remotePortNumber);

            // Set the current backend adapter to the remote one.
            pBeAdapter = pRemoteAdapter;
        }

//++AT:LPGPU2
        else if (m_initDetails.m_isAndroidProfiling)
        {
            // Create and android backend adapter
            lpgpu2::AndroidBackendAdapter *pRemoteAdapter =
              new lpgpu2::AndroidBackendAdapter{
                initDetails.m_collectionDefs,
                initDetails.m_targetDefinition,
                initDetails.m_targetCharacteristics};

            // Set the remote target.
            auto rc = pRemoteAdapter->SetRemoteTarget(initDetails.m_remoteHostAddr, initDetails.m_remotePortNumber);
            if (rc != lpgpu2::PPFnStatus::success) {
              delete pRemoteAdapter;
              GT_ASSERT(false);
              return PPR_WRONG_PROJECT_SETTINGS;
            }

            // Set the current backend adapter to the android one
            pBeAdapter = pRemoteAdapter;
        }
//--AT:LPGPU2
        else
        {
            // Create a local backend adapter.
            pBeAdapter = new LocalBackendAdapter;
        }

        // Replace the existing adapter with the new one.
        delete m_pBEAdapter;
        m_pBEAdapter = pBeAdapter;

        // Initialize the backend.
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            ret = m_pBEAdapter->InitializeBackend();
        }

        if (ret == PPR_NOT_SUPPORTED)
        {
            m_powerProfilingSupportLevel = POWER_PROFILING_LEVEL_PROFILING_NOT_SUPPORTED;
        }
        else
        {
            m_powerProfilingSupportLevel = POWER_PROFILING_LEVEL_PROFILING_SUPPORTED;
        }

        return ret;
    }

    // Shutdown.
    PPResult ShutdownPowerProfiler()
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            ret = m_pBEAdapter->CloseProfileSession();
        }
        return ret;
    }

    // Control.
    PPResult StartProfiling(const AMDTProfileSessionInfo& sessionInfo, const ApplicationLaunchDetails& targetAppDetails,
                            AppLaunchStatus& targetAppLaunchStatus, PPSamplesDataHandler profileOnlineDataHandler, void* pDataHandlerParams,
                            PPFatalErrorHandler fatalErrorHandler, void* pErrorHandlerParams, PowerProfilerCore *parent)
    {
        // Reset the output variable.
        targetAppLaunchStatus = rasUnknown;

        unsigned samplingIntervalMs = 0;
        PPResult ret =  GetCurrentSamplingIntervalMS(samplingIntervalMs);
        GT_IF_WITH_ASSERT(ret < PPR_FIRST_ERROR)
        {
            unsigned minSamplingIntervalMs = 0;
            ret = GetMinSamplingIntervalMs(minSamplingIntervalMs);
            GT_IF_WITH_ASSERT(ret < PPR_FIRST_ERROR)
            {
                GT_IF_WITH_ASSERT(samplingIntervalMs >= minSamplingIntervalMs)
                {
                    // Create a new data adapter.
                    delete m_pDataAdapter;
                    m_pDataAdapter = new lpgpu2::db::LPGPU2DatabaseAdapter();
                    GT_ASSERT_EX(m_pDataAdapter->Initialise(), L"Cannot initialize database adapter")


                    gtList<PPDevice*>* systemTopology;
                    GetSystemTopology(systemTopology);

                    gtVector<int> enabledCounters;
                    GetEnabledCounters(enabledCounters);

                    // Create the session DB.
                    bool isDbCreated = m_pDataAdapter->CreateDb(sessionInfo.m_sessionDbFullPath, AMDT_PROFILE_MODE_TIMELINE);

                    GT_IF_WITH_ASSERT(isDbCreated)
                    {
                        // This is the beginning of the power profiling session.
                        // Save the session configuration to the db.
                        //int quantizedTime = 0;

                        // Get the list of devices
                        gtVector<AMDTProfileDevice*> profileDeviceVec;
                        bool rc = BackendDataConvertor::ConvertToProfileDevice(*systemTopology, profileDeviceVec);

                        gtList<AMDTPwrCounterDesc*> pwrCounterDescList;
                        gtVector<AMDTProfileCounterDesc*> profileCounterVec;

                        if (rc)
                        {
                            m_pBEAdapter->GetDeviceCounters(AMDT_PWR_ALL_DEVICES, pwrCounterDescList);
                            rc = BackendDataConvertor::ConvertToProfileCounterDescVec(pwrCounterDescList, profileCounterVec);
                        }

                        if (rc)
                        {
                            rc = m_pDataAdapter->InsertAllSessionInfo(sessionInfo,
                                                                      samplingIntervalMs,
                                                                      profileDeviceVec,
                                                                      profileCounterVec,
                                                                      enabledCounters);
                        }

                        GT_IF_WITH_ASSERT(rc)
                        {
                            GT_IF_WITH_ASSERT(m_pPollingThread == NULL)
                            {

//++AT:LPGPU2
                                if (targetAppDetails.m_isAndroidSession)
                                {
                                    // Create the polling thread.
                                    m_pPollingThread = new lpgpu2::PPAndroidPollingThread(samplingIntervalMs, profileOnlineDataHandler,
                                                                           pDataHandlerParams, fatalErrorHandler, pErrorHandlerParams, parent, m_pDataAdapter);

                                    

                                }
                                else
                                { 
                                    // Set the application launch details.
                                    m_pBEAdapter->SetApplicationLaunchDetails(targetAppDetails);

                                    // Create the polling thread.
                                    m_pPollingThread = new PPPollingThread(samplingIntervalMs, profileOnlineDataHandler,
                                                                           pDataHandlerParams, fatalErrorHandler, pErrorHandlerParams, m_pBEAdapter, m_pDataAdapter);

                                }
//--AT:LPGPU2

//++TLRS: LPGPU2: Registering that the session is a LPGPU2 session
                                m_pDataAdapter->SetLPGPU2TargetDevice(targetAppDetails.m_isAndroidSession);
//--TLRS: LPGPU2: Registering that the session is a LPGPU2 session
//++AT:LPGPU2
                                if (targetAppDetails.m_isAndroidSession)
                                { 
                                  auto *androidBackendAdapter = static_cast<lpgpu2::AndroidBackendAdapter *>(
                                    m_pBEAdapter);
                                  GT_ASSERT(androidBackendAdapter);
                                  auto *targetDefs = androidBackendAdapter->GetTargetDefinition();
                                  GT_ASSERT(targetDefs);

                                  m_pDataAdapter->SetLPGPU2Platform(
                                    targetDefs->GetTargetElement().GetPlatform());
                                  m_pDataAdapter->SetLPGPU2Hardware(
                                    targetDefs->GetTargetElement().GetHardware());
                                  m_pDataAdapter->SetLPGPU2DCAPI(
                                    targetDefs->GetTargetElement().GetDCAPIVersion());
                                  m_pDataAdapter->SetLPGPU2RAgent(
                                    targetDefs->GetTargetElement().GetRAgentVersion());
                                  m_pDataAdapter->SetLPGPU2HardwareID(
                                    targetDefs->GetTargetElement().GetHardwareID());
                                  m_pDataAdapter->SetLPGPU2BlobSize(
                                    targetDefs->GetTargetElement().GetBlobSize());
                                  m_pDataAdapter->FlushDb();

                                  gtString platform;
                                  m_pDataAdapter->GetLPGPU2Platform(platform);
                                  std::cerr << platform.asASCIICharArray() << std::endl;
                                }
//--AT:LPGPU2

                                // Launch the polling thread.
                                bool isPollingThreadDispatchSuccess = m_pPollingThread->execute();
                                GT_ASSERT_EX(isPollingThreadDispatchSuccess, L"Dispatch of PP polling thread");

                                if (isPollingThreadDispatchSuccess)
                                {
                                    // Wait for the signal from the polling thread.
                                    ret = m_pPollingThread->GetSessionStartedStatus();
                                    const unsigned MAX_NUM_OF_ITERATIONS = 100;
                                    unsigned currIteration = 0;

                                    while (ret == PPR_COMMUNICATION_FAILURE && (currIteration++ < MAX_NUM_OF_ITERATIONS))
                                    {
                                        osSleep(50);
                                        ret = m_pPollingThread->GetSessionStartedStatus();
                                    }

                                    // Retrieve the status of launching the target application.
                                    targetAppLaunchStatus = m_pPollingThread->GetApplicationLaunchStatus();
                                }
                            }
                        }
                        else
                        {
                            ret = PPR_POLLING_THREAD_ALREADY_RUNNING;
                        }
                    }
                    else
                    {
                        ret = PPR_DB_CREATION_FAILURE;
                    }
                }
                else
                {
                    ret = PPR_INVALID_SAMPLING_INTERVAL;
                }
            }
        }
        return ret;
    }


    PPResult StopProfiling()
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;

//++AT:LPGPU2
        gtString errMsg(L"Beginning StopProfiling");
        OS_OUTPUT_DEBUG_LOG(errMsg.asCharArray(), OS_DEBUG_LOG_INFO);

        // Stop the polling activity.
        GT_IF_WITH_ASSERT(m_pPollingThread != NULL)
        {
            m_pPollingThread->requestExit();
            osTimeInterval timeout;
            timeout.setAsMilliSeconds(static_cast<double>(kThreadStopTimeout.count()));
            m_pPollingThread->waitForThreadEnd(timeout);
            m_pPollingThread->terminate();
            delete m_pPollingThread;
            m_pPollingThread = NULL;
        }
        errMsg = L"Finished closing thread";
        OS_OUTPUT_DEBUG_LOG(errMsg.asCharArray(), OS_DEBUG_LOG_INFO);

        // Stop the backend.
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            ret = m_pBEAdapter->StopProfiling();
        }

        GT_IF_WITH_ASSERT(m_pDataAdapter != NULL)
        {
          // Set the end session time
          osTime timing;
          timing.setFromCurrentTime();
          gtString sessionEndTime;
          timing.dateAsString(sessionEndTime, osTime::NAME_SCHEME_FILE,
              osTime::LOCAL);

          GT_ASSERT(!sessionEndTime.isEmpty());
          GT_ASSERT(m_pDataAdapter->IsTimelineMode());
          if (m_pDataAdapter->IsTimelineMode() && (!sessionEndTime.isEmpty()))
          {
            auto isOk =
              m_pDataAdapter->InsertSessionInfoKeyValue(
                  AMDT_SESSION_INFO_KEY_SESSION_END_TIME,
                  sessionEndTime);
            GT_ASSERT(isOk);
            if (!isOk)
            {
              return PPR_COMMUNICATION_FAILURE;
            }
          }
        }
//--AT:LPGPU2

        GT_IF_WITH_ASSERT(m_pDataAdapter != NULL)
        {
            // Flush any pending data, and close all DB connections.
            m_pDataAdapter->FlushDb();
            m_pDataAdapter->CloseDb();
        }
        return ret;
    }

    PPResult PauseProfiling()
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            ret = m_pBEAdapter->PauseProfiling();
        }
        return ret;
    }

    PPResult ResumeProfiling()
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            ret = m_pBEAdapter->ResumeProfiling();
        }
        return ret;
    }

    PPResult EnableCounter(int counterId)
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;

        GT_IF_WITH_ASSERT(m_pDataAdapter != NULL)
        {
            GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
            {
                // Enable the counter.
                ret = m_pBEAdapter->EnableCounter(counterId);
                GT_IF_WITH_ASSERT(ret < PPR_FIRST_ERROR)
                {
                    // Update the cache.
                    m_enabledCountersCache.insert(counterId);

                    // If the session has already started, document this action in the DB.
                    // Otherwise, this info will be documented together with all other session
                    // configurations just before the session starts running.
                    unsigned currentQuantizedTime = 0;

                    if (m_pPollingThread != NULL)
                    {
                        currentQuantizedTime = m_pPollingThread->GetCurrentQuantizedTime();

                        if (currentQuantizedTime > 0)
                        {
                            m_pDataAdapter->InsertCounterEnabled(counterId, currentQuantizedTime);
                        }
                    }
                }
            }
        }
        return ret;
    }

    PPResult DisableCounter(int counterId)
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;

        GT_IF_WITH_ASSERT(m_pDataAdapter != NULL)
        {
            GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
            {
                // Disable the counter.
                ret = m_pBEAdapter->DisableCounter(counterId);
                GT_IF_WITH_ASSERT(ret < PPR_FIRST_ERROR)
                {
                    // Update the cache.
                    auto iter = m_enabledCountersCache.find(counterId);

                    if (iter != m_enabledCountersCache.end())
                    {
                        m_enabledCountersCache.erase(iter);
                    }

                    // If the session has already started, document this action in the DB.
                    // Otherwise, this info will be documented together with all other session
                    // configurations just before the session starts running.
                    unsigned currentQuantizedTime = 0;

                    if (m_pPollingThread != NULL)
                    {
                        currentQuantizedTime = m_pPollingThread->GetCurrentQuantizedTime();

                        if (currentQuantizedTime > 0)
                        {
                            m_pDataAdapter->InsertCounterDisabled(counterId, currentQuantizedTime);
                        }
                    }
                }
            }
        }
        return ret;
    }

    // Returns PPR_NO_ERROR if and only if ALL counters were enabled successfully.
    PPResult EnableCounter(const gtList<int>& counterIds)
    {
        PPResult ret = PPR_NO_ERROR;
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            PPResult tmpRc = PPR_NO_ERROR;

            for (const int& counterId : counterIds)
            {
                tmpRc = this->EnableCounter(counterId);

                if (tmpRc != PPR_NO_ERROR && ret < PPR_FIRST_ERROR)
                {
                    ret = tmpRc;
                }
            }
        }
        return ret;
    }

    PPResult GetMinSamplingIntervalMs(unsigned& minSamplingInterval)
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            ret = m_pBEAdapter->GetMinTimerSamplingPeriodMS(minSamplingInterval);
        }
        return ret;
    }

    PPResult GetCurrentSamplingIntervalMS(unsigned& samplingInterval)
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            if (m_currentSamplingIntervalMs == 0)
            {
                ret = m_pBEAdapter->GetCurrentSamplingInterval(samplingInterval);
            }
            else
            {
                samplingInterval = m_currentSamplingIntervalMs;
                ret = PPR_NO_ERROR;
            }
        }
        return ret;
    }

    PPResult SetSamplingIntervalMS(unsigned int samplingInterval)
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            ret = m_pBEAdapter->SetTimerSamplingInterval(samplingInterval);

            if (ret == AMDT_STATUS_OK)
            {
                m_currentSamplingIntervalMs = samplingInterval;
            }
        }
        return ret;
    }

    PPResult IsCounterEnabled(int counterId, bool& isEnabled)
    {
        PPResult ret = PPR_UNKNOWN_FAILURE;
        GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
        {
            isEnabled = (m_enabledCountersCache.find(counterId) != m_enabledCountersCache.end());
            ret = PPR_NO_ERROR;
        }
        return ret;
    }

    PPResult GetSystemTopology(gtList<PPDevice*>*& pSysTopology)
    {
        PPResult ret = PPR_NO_ERROR;

        if (m_sysTopologyCache.empty())
        {
            GT_IF_WITH_ASSERT(m_pBEAdapter != NULL)
            {
                ret = m_pBEAdapter->GetSystemTopology(m_sysTopologyCache);
            }
        }

        // Assign the pointer to point to the cached structure.
        if (!m_sysTopologyCache.empty())
        {
            pSysTopology = &m_sysTopologyCache;
        }
        else
        {
            pSysTopology = NULL;
        }

        return ret;
    }

    PPResult GetEnabledCounters(gtVector<int>& enabledCounters)
    {
        PPResult ret = PPR_NO_ERROR;
        bool isEnabled = false;

        // Build the enabled counters map.
        gtMap<int, AMDTPwrCounterDesc*> countersMap;
        ret = GetAllCountersDetails(countersMap);

        GT_IF_WITH_ASSERT(ret < PPR_FIRST_ERROR)
        {
            for (auto& counterDetailsPair : countersMap)
            {
                int counterId = counterDetailsPair.first;

                // If the counter is enabled, and it hasn't been added yet, add its id to our container.
                if ((IsCounterEnabled(counterId, isEnabled) < PPR_FIRST_ERROR) && isEnabled &&
                    (std::find(enabledCounters.begin(), enabledCounters.end(), counterId) == enabledCounters.end()))
                {
                    enabledCounters.push_back(counterId);
                }

                // We can delete the counter details structure, as it won't be needed.
                delete counterDetailsPair.second;
                counterDetailsPair.second = NULL;
            }
        }

        return ret;
    }

    void UpdateCountersMap(const PPDevice* pDevice, gtMap<int, AMDTPwrCounterDesc*>& countersMap)
    {
        GT_IF_WITH_ASSERT(pDevice != NULL)
        {
            for (AMDTPwrCounterDesc* pCounterDesc : pDevice->m_supportedCounters)
            {
                GT_IF_WITH_ASSERT(pCounterDesc != NULL)
                {
                    // Copy the required counter's value.
                    countersMap[pCounterDesc->m_counterID] =
                        BackendDataConvertor::CopyPwrCounterDesc(*pCounterDesc);
                }
            }

            for (const PPDevice* pSubDevice : pDevice->m_subDevices)
            {
                UpdateCountersMap(pSubDevice, countersMap);
            }
        }
    }

    void BuildCountersMap(const gtList<PPDevice*>& systemTopology, gtMap<int, AMDTPwrCounterDesc*>& countersMap)
    {
        for (const PPDevice* pDevice : systemTopology)
        {
            UpdateCountersMap(pDevice, countersMap);
        }
    }

    PPResult GetAllCountersDetails(gtMap<int, AMDTPwrCounterDesc*>& counterDetailsPerCounter)
    {
        // This implementation can be optimized in the future by caching the system topology.
        gtList<PPDevice*>* pSystemTopology;

        // Get the system topology.
        PPResult ret = GetSystemTopology(pSystemTopology);

        GT_IF_WITH_ASSERT(ret < PPR_FIRST_ERROR)
        {
            GT_IF_WITH_ASSERT(pSystemTopology != NULL)
            {
                // Build the counters map.
                BuildCountersMap(*pSystemTopology, counterDetailsPerCounter);
            }
        }

        return ret;
    }

    void GetLastErrorMessage(gtString& msg) const
    {
        if (m_pBEAdapter != nullptr)
        {
            m_pBEAdapter->GetLastErrorMessage(msg);
        }
    }

    PowerProfilingSupportLevel GetPowerProfilingSupportLevel() const
    {
        return m_powerProfilingSupportLevel;
    }

    bool IsRemotePP() const
    {
        return m_initDetails.m_isRemoteProfiling;
    }

//++AT:LPGPU2
    bool IsAndroidPP() const
    {
        return m_initDetails.m_isAndroidProfiling;
    }

    const IPowerProfilerBackendAdapter *GetBEAdapter() const
    {
      return m_pBEAdapter;
    }
//--AT:LPGPU2

    // This object connects us to the backend.
    // It might be a local session or remote session.
    IPowerProfilerBackendAdapter* m_pBEAdapter;
    PPPollingThread* m_pPollingThread;

    //++TLRS: Changing the type to access LPGPU2 specific API
    lpgpu2::db::LPGPU2DatabaseAdapter* m_pDataAdapter;
    //--TLRS: Changing the type to access LPGPU2 specific API

    PowerProfilerCoreInitDetails m_initDetails;
    gtList<PPDevice*> m_sysTopologyCache;
    unsigned m_currentSamplingIntervalMs;
    gtSet<int> m_enabledCountersCache;


    /// Indicates whether power profiling is supported with the latest configuration that
    /// was fed to the profiler middle tier.
    /// This value is refreshed each time the middle tier is initialized.
    /// This value is cached and can be queried even after calling shutdown on the middle tier.
    PowerProfilingSupportLevel m_powerProfilingSupportLevel;
};

PowerProfilerCore::PowerProfilerCore() : m_pImpl(new PowerProfilerCore::Impl())
{
}


PowerProfilerCore::~PowerProfilerCore()
{
    delete m_pImpl;
    m_pImpl = nullptr;
}

PPResult PowerProfilerCore::InitPowerProfiler(const PowerProfilerCoreInitDetails& initDetails)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->InitPowerProfiler(initDetails);
        gtString logMsg;
        logMsg.appendFormattedString(L"Power Profiler initialization returned %ls", PowerProfilerMidTierUtil::CodeDescription(ret).asCharArray());

        if (PowerProfilerMidTierUtil::isNonFailureResult(ret))
        {
            OS_OUTPUT_DEBUG_LOG(logMsg.asCharArray(), OS_DEBUG_LOG_INFO);
        }
        else
        {
            OS_OUTPUT_DEBUG_LOG(logMsg.asCharArray(), OS_DEBUG_LOG_ERROR);
        }
    }
    return ret;
}

PPResult PowerProfilerCore::StartProfiling(const AMDTProfileSessionInfo& sessionInfo, const ApplicationLaunchDetails& targetAppDetails, AppLaunchStatus& targetAppLaunchStatus,
                                           PPSamplesDataHandler profileOnlineDataHandler, void* pDataHandlerParams, PPFatalErrorHandler profileOnlineFatalErrorHandler, void* pErrorHandlerParams)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->StartProfiling(sessionInfo, targetAppDetails, targetAppLaunchStatus,
                                      profileOnlineDataHandler, pDataHandlerParams, profileOnlineFatalErrorHandler, pErrorHandlerParams,
                                      this);
    }
    return ret;
}

PPResult PowerProfilerCore::StopProfiling()
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->StopProfiling();
    }
    return ret;
}

PPResult PowerProfilerCore::PauseProfiling()
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->PauseProfiling();
    }
    return ret;
}

PPResult PowerProfilerCore::ResumeProfiling()
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->ResumeProfiling();
    }
    return ret;
}

PPResult PowerProfilerCore::GetEnabledCounters(gtVector<int>& enabledCounters)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->GetEnabledCounters(enabledCounters);
    }
    return ret;
}

PPResult PowerProfilerCore::EnableCounter(int counterId)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->EnableCounter(counterId);
    }
    return ret;
}

PPResult PowerProfilerCore::EnableCounter(const gtList<int>& counterIdsList)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->EnableCounter(counterIdsList);
    }
    return ret;
}

PPResult PowerProfilerCore::DisableCounter(int counterId)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->DisableCounter(counterId);
    }
    return ret;
}

PPResult PowerProfilerCore::GetMinSamplingIntervalMS(unsigned int& samplingInterval)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->GetMinSamplingIntervalMs(samplingInterval);
    }
    return ret;
}

PPResult PowerProfilerCore::GetCurrentSamplingIntervalMS(unsigned int& samplingInterval)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->GetCurrentSamplingIntervalMS(samplingInterval);
    }
    return ret;
}

PPResult PowerProfilerCore::SetSamplingIntervalMS(unsigned int samplingInterval)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->SetSamplingIntervalMS(samplingInterval);
    }
    return ret;
}

PPResult PowerProfilerCore::IsCounterEnabled(int counterId, bool& isEnabled)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    isEnabled = false;

    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->IsCounterEnabled(counterId, isEnabled);
    }
    return ret;
}

PPResult PowerProfilerCore::ShutdownPowerProfiler()
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->ShutdownPowerProfiler();
    }
    return ret;
}

PPResult PowerProfilerCore::GetSystemTopology(gtList<PPDevice*>& systemDevices)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        // Get a reference to the implementation's structure.
        gtList<PPDevice*>* pSysDevices = NULL;
        ret = m_pImpl->GetSystemTopology(pSysDevices);

        // Create a deep copy of the structure, to avoid returning
        // the cached value, which might be cached by the user and
        // become invalidated when this object (PowerProfilerCore)
        // becomes re-initialized.
        GT_IF_WITH_ASSERT(pSysDevices != NULL)
        {
            systemDevices.clear();

            for (PPDevice* pDevice : *pSysDevices)
            {
                if (pDevice != NULL)
                {
                    PPDevice* pDeviceCopy = new PPDevice(*pDevice);
                    systemDevices.push_back(pDeviceCopy);
                }
            }
        }

    }
    return ret;
}

PPResult PowerProfilerCore::GetAllCountersDetails(gtMap<int, AMDTPwrCounterDesc*>& counterDetailsPerCounter)
{
    PPResult ret = PPR_UNKNOWN_FAILURE;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->GetAllCountersDetails(counterDetailsPerCounter);
    }
    return ret;
}

void PowerProfilerCore::GetLastErrorMessage(gtString& errorMessage) const
{
    errorMessage.makeEmpty();
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        m_pImpl->GetLastErrorMessage(errorMessage);
    }
}

PowerProfilerCore::PowerProfilingSupportLevel PowerProfilerCore::GetPowerProfilingSupportLevel() const
{
    PowerProfilingSupportLevel ret = POWER_PROFILING_LEVEL_UNKNOWN;
    GT_IF_WITH_ASSERT(m_pImpl != NULL)
    {
        ret = m_pImpl->GetPowerProfilingSupportLevel();
    }
    return ret;
}

bool PowerProfilerCore::isRemotePP() const
{
    bool rc = m_pImpl->IsRemotePP();
    return rc;
}
 
//++AT:LPGPU2
bool PowerProfilerCore::isAndroidPP() const
{
  return m_pImpl->IsAndroidPP();
}

const IPowerProfilerBackendAdapter *PowerProfilerCore::GetBEAdapter() const
{
  return m_pImpl->GetBEAdapter();
}
//--AT:LPGPU2
