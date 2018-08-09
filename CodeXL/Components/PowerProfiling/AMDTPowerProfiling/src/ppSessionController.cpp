//==================================================================================
// Copyright (c) 2016 , Advanced Micro Devices, Inc.  All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file ppSessionController.cpp
///
//==================================================================================

//------------------------------ ppSessionController.cpp ------------------------------

// Infra:
#include <AMDTBaseTools/Include/gtAssert.h>
#include <AMDTBaseTools/Include/gtIgnoreCompilerWarnings.h>

// Local:
#include <AMDTPowerProfiling/src/ppSessionController.h>
#include <AMDTPowerProfiling/src/ppAppController.h>

ppSessionController::ppSessionController() : m_allCountersDetailsDbCache(), m_apuCounterId(-1)
{

}

ppSessionController::~ppSessionController()
{
    for (auto it = m_allCounterDescriptions.begin(); it != m_allCounterDescriptions.end(); it++)
    {
        AMDTPwrCounterDesc* pDesc = it->second;
        // FIXME: delete name and description
        delete pDesc;
    }

    for (auto it = m_allCountersDetailsDbCache.begin(); it != m_allCountersDetailsDbCache.end(); it++)
    {
        AMDTPwrCounterDesc* pDesc = it->second;
        // FIXME: delete name and description
        delete pDesc;
    }

    for (auto it = m_systemDevices.begin(); it != m_systemDevices.end(); it++)
    {
        delete *it;
    }

    // Close all connections to the database:
    CloseDB();
}

void ppSessionController::OpenDB()
{
    if (m_currentSessionState != PP_SESSION_STATE_NEW)
    {
        // Open the database connection:
        const bool bIsReadOnly = false;
        m_powerProfilingBL.OpenPowerProfilingDatabaseForRead(m_dbFilePath.asString(), bIsReadOnly);
    }
}


void ppSessionController::CloseDB()
{
    m_powerProfilingBL.CloseAllConnections();
}

void ppSessionController::GetSessionTimeRange(SamplingTimeRange& samplingTimeRange)
{
    if (m_currentSessionState != PP_SESSION_STATE_NEW)
    {
        m_powerProfilingBL.GetSessionTimeRange(samplingTimeRange);
    }
}

//++CF:LPGPU2
void ppSessionController::GetSessionTimeRangeNanoSec(SamplingTimeRange& samplingTimeRange)
{
    // Convert millisecond time range to nanoseconds
    GetSessionTimeRange(samplingTimeRange);
    samplingTimeRange.m_fromTime *= 1e6;
    samplingTimeRange.m_toTime   *= 1e6;
}
//--CF:LPGPU2

void ppSessionController::GetSessionCountersFromDB(const gtVector<AMDTDeviceType>& deviceTypes, AMDTPwrCategory counterCategory, gtVector<int>& counterIds)
{
    GT_IF_WITH_ASSERT(m_currentSessionState != PP_SESSION_STATE_NEW)
    {
        if (deviceTypes.size() > 0)
        {
            m_powerProfilingBL.GetSessionCounters(deviceTypes, counterCategory, counterIds);
        }
        else
        {
            m_powerProfilingBL.GetSessionCounters(counterCategory, counterIds);
        }
    }
}

bool ppSessionController::GetDeviceType(int deviceId, AMDTDeviceType& deviceType)
{
    bool ret = false;

    // If the DB is not up yet
    if (m_currentSessionState == PP_SESSION_STATE_NEW)
    {
        PPDevice* pRootDevice = GetSystemDevices().front();
        const PPDevice* pFoundDevice = PowerProfilerBL::GetDevice(deviceId, pRootDevice);

        if (pFoundDevice != nullptr)
        {
            deviceType = pFoundDevice->m_deviceType;
            ret = true;
        }
    }
    else
    {
        ret = m_powerProfilingBL.GetDeviceType(deviceId, deviceType);
    }

    return ret;
}

const gtList<PPDevice*>& ppSessionController::GetSystemDevices()
{
    // if the collection of system devices has not been retrieved from the backend yet
    if (m_systemDevices.empty())
    {
        // Need to retrieve the system devices first
        ppAppController& appController = ppAppController::instance();
        PPResult sysTopologyRes = appController.GetMiddleTierController().GetSystemTopology(m_systemDevices);
        GT_ASSERT(sysTopologyRes < PPR_FIRST_ERROR);
    }

    return m_systemDevices;
}

//++AT:LPGPU2
/// @brief Flatten a hierarchy of devices into a vector, with subdevices too
/// @param[in ] devicesTree List of devices
/// @param[out] devicesVec Flattened vector of devices
void TransformDevicesHierarchyToVector(const gtList<PPDevice*> &devicesTree,
    gtVector<PPDevice> &devicesVec)
{
  for (const PPDevice *pDevice: devicesTree)
  {
    GT_IF_WITH_ASSERT(pDevice != NULL)
    {
      devicesVec.push_back(*pDevice);

      TransformDevicesHierarchyToVector(pDevice->m_subDevices, devicesVec);
    }
  }
}

/// @brief Retrieve the system devices as a flattened vector
/// @return gtVector<PPDevice> Vector of devices
const gtVector<PPDevice> ppSessionController::GetSystemDevicesAsFlatVector()
{
  gtVector<PPDevice> devicesAsVector;

  if (m_currentSessionState != PP_SESSION_STATE_COMPLETED)
  {
    const auto &sysDevices = GetSystemDevices();
    TransformDevicesHierarchyToVector(sysDevices, devicesAsVector);
  }
  else
  {
    m_powerProfilingBL.GetAllDevicesAsVector(devicesAsVector);
  }

  return devicesAsVector;
}
//--AT:LPGPU2

const gtMap<int, AMDTPwrCounterDesc*>& ppSessionController::GetAllCounterDescriptions()
{
    const gtMap<int, AMDTPwrCounterDesc*>* pCounterDescriptors = nullptr;

    if (m_currentSessionState != PP_SESSION_STATE_COMPLETED)
    {
        // if the collection of system devices has not been retrieved from the backend yet
        if (m_allCounterDescriptions.empty())
        {
            // Need to retrieve the system devices first
            ppAppController& appController = ppAppController::instance();
            PPResult resultValue = appController.GetMiddleTierController().GetAllCountersDetails(m_allCounterDescriptions);
            GT_ASSERT(resultValue < PPR_FIRST_ERROR);
        }

        pCounterDescriptors = &m_allCounterDescriptions;
    }
    else
    {
        // Cache the counter details from the DB
        if (m_allCountersDetailsDbCache.empty())
        {
            bool isOk = m_powerProfilingBL.GetAllSessionCountersDescription(m_allCountersDetailsDbCache);
            GT_ASSERT(isOk);
        }

        pCounterDescriptors = &m_allCountersDetailsDbCache;
    }

    return *pCounterDescriptors;
}

void ppSessionController::GetEnabledCountersByTypeAndCategory(const gtVector<AMDTDeviceType>& requestedDeviceTypes, AMDTPwrCategory requestedCounterCategory, gtVector<int>& countersVector)
{
    countersVector.clear();

//++AT:LPGPU2
    if (m_currentSessionState == PP_SESSION_STATE_NEW || m_currentSessionState == PP_SESSION_STATE_RUNNING)
//--AT:LPGPU2
    {
        // Get the counters from project settings:
        gtVector<int> enabledCounters;
        ppAppController::instance().GetCurrentProjectEnabledCountersByCategory(requestedCounterCategory, enabledCounters);
        int numEnabled = enabledCounters.size();

        for (int nCounter = 0; nCounter < numEnabled; nCounter++)
        {
            int enabledCounterId = enabledCounters[nCounter];

            if (requestedDeviceTypes.size() > 0)
            {
                AMDTPwrCounterDesc* pcounterDesc = GetAllCounterDescriptions().at(enabledCounterId);
                GT_IF_WITH_ASSERT(pcounterDesc != nullptr)
                {
                    AMDTDeviceType counterDeviceType;
                    GetDeviceType(pcounterDesc->m_deviceId, counterDeviceType);

                    // Check if the device type associated with this counter matches one of the requested devicce types
                    for (AMDTDeviceType requestedDeviceType : requestedDeviceTypes)
                    {
                        if (requestedDeviceType == counterDeviceType)
                        {
                            // found a device that matches the requested device types and is supported by this counter, so store this counter
                            countersVector.push_back(enabledCounterId);
                            break;
                        }
                    }
                }
            }
            else
            {
                countersVector.push_back(enabledCounterId);
            }
        }
    }
    else
    {
        countersVector.clear();
        GetSessionCountersFromDB(requestedDeviceTypes, requestedCounterCategory, countersVector);
    }
}

//++AT:LPGPU2
/// @brief Get enabled counters given their type, category and parent device ID
/// @param[in] searchTypeVector Vector of types of counters to search for
/// @param[in] searchCategory Category of counters to search for
/// @param[in] deviceId Parent device ID
/// @param[out] countersVector Vector of counter ids
void ppSessionController::GetEnabledCountersByTypeAndCategoryAndDeviceId(
    const gtVector<AMDTDeviceType> &searchTypeVector,
    AMDTPwrCategory searchCategory,
    int deviceId,
    gtVector<int> &countersVector)
{
    countersVector.clear();
    const auto &allCounterDescs = GetAllCounterDescriptions();
    gtVector<int> enabledCounters;

    if (m_currentSessionState == PP_SESSION_STATE_NEW ||
        m_currentSessionState == PP_SESSION_STATE_RUNNING)
    {
      // Get the counters from project settings:
      ppAppController::instance().GetCurrentProjectEnabledCountersByCategory(
          searchCategory, enabledCounters);
    }
    else
    {
      GetSessionCountersFromDB(searchTypeVector, searchCategory,
          enabledCounters);
    }

    auto deviceIdUInt = static_cast<gtUInt32>(deviceId);
    for (auto ec : enabledCounters)
    {
      auto *pCounterDesc = allCounterDescs.at(ec);
      GT_IF_WITH_ASSERT(pCounterDesc != nullptr)
      {
        if (!searchTypeVector.empty())
        {
          AMDTDeviceType counterDeviceType;
          GetDeviceType(pCounterDesc->m_deviceId, counterDeviceType);

          for (const auto &devType : searchTypeVector)
          {
            if (devType == counterDeviceType &&
                pCounterDesc->m_deviceId == deviceIdUInt)
            {
              countersVector.push_back(ec);
              break;
            }
          }
        }
        else if (pCounterDesc->m_deviceId == deviceIdUInt)
        {
          countersVector.push_back(ec);
        }
      }
    }
}
//--AT:LPGPU2

int ppSessionController::GetAPUCounterID()
{
    if (m_apuCounterId == -1)
    {
        // Get the counters from backend
        if (m_currentSessionState == PP_SESSION_STATE_NEW)
        {
            // Get the counters from backend
            m_apuCounterId = ppAppController::instance().GetAPUCounterIdFromBackend();
        }
        else
        {
            // Get the APU counter id (if exists on the current machine):
            bool rc = m_powerProfilingBL.GetApuPowerCounterId(m_apuCounterId);

            if (!rc)
            {
                // cannot relay on GetApuPowerCounterId not changing the init value of apuCounterId when fail
                m_apuCounterId = -1;
            }
        }
    }

    return m_apuCounterId;
}

unsigned int ppSessionController::GetSamplingTimeInterval()
{
    unsigned int retVal = 100;

    if (m_currentSessionState == PP_SESSION_STATE_NEW)
    {
        // Get the interval from the project settings:
        retVal = ppAppController::instance().GetCurrentProjectSamplingInterval();
    }
    else if (m_currentSessionState == PP_SESSION_STATE_RUNNING)
    {
        PowerProfilerCore& midTierController = ppAppController::instance().GetMiddleTierController();
        midTierController.GetCurrentSamplingIntervalMS(retVal);
    }
    else
    {
        bool rc = m_powerProfilingBL.GetSessionSamplingIntervalMs(retVal);
        GT_ASSERT(rc);
    }

    return retVal;
}

QString ppSessionController::GetCounterNameById(int counterId)
{
    QString retVal;

    if (m_currentSessionState != PP_SESSION_STATE_COMPLETED)
    {
        ppControllerCounterData* info = ppAppController::instance().GetCounterInformationById(counterId);

        if (info != nullptr)
        {
            retVal = info->m_name;
        }
    }
    else
    {
        const AMDTPwrCounterDesc* pCounterDesc = GetCounterDescriptor(counterId);

        if (pCounterDesc != nullptr)
        {
            retVal = pCounterDesc->m_name;
        }
    }

    return retVal;
}


int ppSessionController::GetCounterIDByName(const QString& counterName)
{
    int retVal = -1;

    // Get all counters descriptions:
    const gtMap<int, AMDTPwrCounterDesc*> counterDescriptions = GetAllCounterDescriptions();
    auto iter = counterDescriptions.begin();
    auto iterEnd = counterDescriptions.end();

    for (; iter != iterEnd; iter++)
    {
        AMDTPwrCounterDesc* pDescription = iter->second;

        if (pDescription != nullptr)
        {
            if (pDescription->m_name == counterName)
            {
                // Found the requested counter:
                retVal = iter->first;
                break;
            }
        }
    }

    return retVal;
}

bool ppSessionController::IsCounterInPercentUnits(int counterId)
{
    bool retVal = false;

    if (m_currentSessionState != PP_SESSION_STATE_COMPLETED)
    {
        ppControllerCounterData* info = ppAppController::instance().GetCounterInformationById(counterId);

        if (nullptr != info)
        {
            retVal = (info->m_units == AMDT_PWR_UNIT_TYPE_PERCENT);
        }
    }
    else
    {
        const AMDTPwrCounterDesc* pCounterDesc = GetCounterDescriptor(counterId);

        if (pCounterDesc != nullptr)
        {
            retVal = (pCounterDesc->m_units == AMDT_PWR_UNIT_TYPE_PERCENT);
        }
    }

    return retVal;
}

const AMDTPwrCounterDesc* ppSessionController::GetCounterDescriptor(int counterId)
{
    const gtMap<int, AMDTPwrCounterDesc*>& counterDescriptors = GetAllCounterDescriptions();
    const AMDTPwrCounterDesc* pCounterDesc = nullptr;

    // Get the correct counter name.
    auto iter = counterDescriptors.find(counterId);

    if (iter != counterDescriptors.end())
    {
        pCounterDesc = iter->second;
    }

    return pCounterDesc;
}

QColor ppSessionController::GetColorForCounter(int counterId)
{
    QColor retVal = Qt::red;

    if (m_currentSessionState != PP_SESSION_STATE_COMPLETED)
    {
        ppControllerCounterData* info = ppAppController::instance().GetCounterInformationById(counterId);

        if (nullptr != info)
        {
            retVal = info->m_color;
        }
    }
    else
    {
        // Get the correct color from the colors map.
        const AMDTPwrCounterDesc* pCounterDesc = GetCounterDescriptor(counterId);

        if (pCounterDesc != nullptr)
        {
//++AT:LPGPU2
            retVal = ppColorsMap::Instance().GetColorForCounterID(
                pCounterDesc->m_counterID);
//--AT:LPGPU2
        }
    }

    return retVal;
}

bool ppSessionController::IsChildCounter(int counterId)
{
    bool ret = false;

    QString counterName = GetCounterNameById(counterId);
    ret = ppAppController::instance().IsChildCounter(counterName);

    return ret;
}

QString ppSessionController::GetCounterParent(int counterId)
{
    QString ret;

    QString counterName = GetCounterNameById(counterId);
    ret = ppAppController::instance().GetCounterParent(counterName);

    return ret;
}

QString ppSessionController::GetCounterDescription(int counterId)
{
    QString retVal;
    const AMDTPwrCounterDesc* pCounterDesc = GetCounterDescriptor(counterId);

    if (pCounterDesc != nullptr)
    {
        retVal = pCounterDesc->m_description;
    }

    return retVal;
}

