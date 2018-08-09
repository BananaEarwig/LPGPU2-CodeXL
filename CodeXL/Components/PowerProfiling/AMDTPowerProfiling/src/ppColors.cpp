//==================================================================================
// Copyright (c) 2016 , Advanced Micro Devices, Inc.  All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file ppColors.cpp
///
//==================================================================================


#include <AMDTPowerProfiling/src/ppColors.h>
#include <AMDTOSWrappers/Include/osDebugLog.h>

ppColorsMap* ppColorsMap::m_colorsSingleInstance = nullptr;
ppHierarchyMap* ppHierarchyMap::m_hierarchyMapSingleInstance = nullptr;
//++AT:LPGPU2
constexpr const gtUInt32 kColourHashingMask = 0x00000fff;
constexpr const gtInt32 kColourMaskEndRange = 4095;
constexpr const gtInt32 kColourMaskStartRange = 0;
constexpr const gtInt32 kColourHueEndRange = 359;
constexpr const gtInt32 kColourHueStartRange = 0;
constexpr const gtInt32 kColourSaturation= 245;
constexpr const gtInt32 kColourValue= 245;
//--AT:LPGPU2

#define MAX_SUPPORTED_CPU 8

ppColorsMap::ppColorsMap()
{
    m_dgpusCount = 0;

    if (m_colorsMap.isEmpty())
    {
        // Keep the indices running even when changing the selected hue, for contrast:
        //////////////////////////////////////////////////////////////////////////
        // Power
        m_colorsMap[PP_STR_Counter_Power_CU0] = acGetAMDColorScaleColor(AC_AMD_GREEN, 0);
        m_colorsMap[PP_STR_Counter_Power_CU1] = acGetAMDColorScaleColor(AC_AMD_GREEN, 1);

        for (int nCpu = 0; nCpu < MAX_SUPPORTED_CPU; nCpu++)
        {
            QString cpuAvrStr = QString(PP_STR_Counter_Ave_Freq_Core).arg(nCpu);
            m_colorsMap[cpuAvrStr] = acGetAMDColorScaleColor(AC_AMD_GREEN, nCpu);
        }

        // On Kaveri "iGPU Power", on Carrizo "GFX Power"
        m_colorsMap[PP_STR_Counter_Power_IGPU] = acGetAMDColorScaleColor(AC_AMD_RED, 2);
        m_colorsMap[PP_STR_Counter_Power_GFX] = acGetAMDColorScaleColor(AC_AMD_RED, 2);

        m_colorsMap[PP_STR_Counter_Power_DGPU] = acGetAMDColorScaleColor(AC_AMD_RED, 3);
        m_colorsMap[PP_STR_Counter_Power_MemCtrl] = acGetAMDColorScaleColor(AC_AMD_GRAY, 0);
        m_colorsMap[PP_STR_Counter_Power_PCIECtrl] = acGetAMDColorScaleColor(AC_AMD_GRAY, 1);
        m_colorsMap[PP_STR_Counter_Power_IOCtrl] = acGetAMDColorScaleColor(AC_AMD_GRAY, 2);
        m_colorsMap[PP_STR_Counter_Power_DisplayCtrl] = acGetAMDColorScaleColor(AC_AMD_PURPLE, 0);
        m_colorsMap[PP_STR_Counter_Power_GFXCtrl] = acGetAMDColorScaleColor(AC_AMD_PURPLE, 1);
        m_colorsMap[PP_STR_Counter_Power_VddIO] = acGetAMDColorScaleColor(AC_AMD_GRAY, 0);
        m_colorsMap[PP_STR_Counter_Power_VddNB] = acGetAMDColorScaleColor(AC_AMD_GRAY, 1);
        m_colorsMap[PP_STR_Counter_Power_Vddp] = acGetAMDColorScaleColor(AC_AMD_PURPLE, 0);
        m_colorsMap[PP_STR_Counter_Power_VddGFX] = acGetAMDColorScaleColor(AC_AMD_PURPLE, 1);
        m_colorsMap[PP_STR_Counter_Power_Other] = acGetAMDColorScaleColor(AC_AMD_CYAN, 7);
        m_colorsMap[PP_STR_Counter_Power_TotalAPU] = acGetAMDColorScaleColor(AC_AMD_CYAN, 0); // This should always be the darkest color, for contrast against the background.

        m_colorsMap[PP_STR_Counter_Power_UVD] = acGetCodeXLColorScaleColor(AC_CODEXL_BLUE, 0);
        m_colorsMap[PP_STR_Counter_Power_VCE] = acGetCodeXLColorScaleColor(AC_CODEXL_BLUE, 1);
        m_colorsMap[PP_STR_Counter_Power_ACP] = acGetCodeXLColorScaleColor(AC_CODEXL_BLUE, 2);
        m_colorsMap[PP_STR_Counter_Power_UNB] = acGetCodeXLColorScaleColor(AC_CODEXL_BLUE, 3);
        m_colorsMap[PP_STR_Counter_Power_NB] = acGetCodeXLColorScaleColor(AC_CODEXL_BLUE, 3);
        m_colorsMap[PP_STR_Counter_Power_SMU] = acGetCodeXLColorScaleColor(AC_CODEXL_BLUE, 4);
        m_colorsMap[PP_STR_Counter_Power_RoC] = acGetAMDColorScaleColor(AC_AMD_GRAY, 3);

        //////////////////////////////////////////////////////////////////////////
        // Temperature
        m_colorsMap[PP_STR_Counter_Temp_CU0] = acGetAMDColorScaleColor(AC_AMD_GREEN, 0);
        m_colorsMap[PP_STR_Counter_MeasuredTemp_CU0] = acGetAMDColorScaleColor(AC_AMD_GREEN, 1);
        m_colorsMap[PP_STR_Counter_Temp_CU1] = acGetAMDColorScaleColor(AC_AMD_GREEN, 2);
        m_colorsMap[PP_STR_Counter_MeasuredTemp_CU1] = acGetAMDColorScaleColor(AC_AMD_GREEN, 3);

        // On Kaveri "iGPU Temp", on Carrizo "GFX Temp"
        m_colorsMap[PP_STR_Counter_Temp_IGPU] = acGetAMDColorScaleColor(AC_AMD_RED, 4);
        m_colorsMap[PP_STR_Counter_Temp_GFX] = acGetAMDColorScaleColor(AC_AMD_RED, 4);
        m_colorsMap[PP_STR_Counter_MeasuredTemp_IGPU] = acGetAMDColorScaleColor(AC_AMD_RED, 5);
        m_colorsMap[PP_STR_Counter_MeasuredTemp_GFX] = acGetAMDColorScaleColor(AC_AMD_RED, 5);

        m_colorsMap[PP_STR_Counter_Temp_DGPU] = acGetAMDColorScaleColor(AC_AMD_RED, 6);
        m_colorsMap[PP_STR_Counter_Temp_Node] = acGetAMDColorScaleColor(AC_AMD_RED, 6);

        //////////////////////////////////////////////////////////////////////////
        // Frequency

        // CodeXL 1.6 used the name "Frequency"
        m_colorsMap[PP_STR_Counter_Freq_Core0] = acGetAMDColorScaleColor(AC_AMD_GREEN, 0);
        m_colorsMap[PP_STR_Counter_Freq_Core1] = acGetAMDColorScaleColor(AC_AMD_GREEN, 1);
        m_colorsMap[PP_STR_Counter_Freq_Core2] = acGetAMDColorScaleColor(AC_AMD_GREEN, 2);
        m_colorsMap[PP_STR_Counter_Freq_Core3] = acGetAMDColorScaleColor(AC_AMD_GREEN, 3);
        m_colorsMap[PP_STR_Counter_Freq_IGPU] = acGetAMDColorScaleColor(AC_AMD_RED, 4);

        // CodeXL 1.7 uses the name "Avg. Frequency"
        m_colorsMap[PP_STR_Counter_AvgFreq_Core0] = acGetAMDColorScaleColor(AC_AMD_GREEN, 0);
        m_colorsMap[PP_STR_Counter_AvgFreq_Core1] = acGetAMDColorScaleColor(AC_AMD_GREEN, 1);
        m_colorsMap[PP_STR_Counter_AvgFreq_Core2] = acGetAMDColorScaleColor(AC_AMD_GREEN, 2);
        m_colorsMap[PP_STR_Counter_AvgFreq_Core3] = acGetAMDColorScaleColor(AC_AMD_GREEN, 3);
        m_colorsMap[PP_STR_Counter_AvgFreq_IGPU] = acGetAMDColorScaleColor(AC_AMD_RED, 4);
        m_colorsMap[PP_STR_Counter_AvgFreq_GFX] = acGetAMDColorScaleColor(AC_AMD_RED, 4);
        m_colorsMap[PP_STR_Counter_AvgFreq_DGPU] = acGetAMDColorScaleColor(AC_AMD_RED, 5);
        m_colorsMap[PP_STR_Counter_AvgFreq_ACP] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 0);

        //////////////////////////////////////////////////////////////////////////
        // Current
        m_colorsMap[PP_STR_Counter_Current_CPUCore] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 0);
        m_colorsMap[PP_STR_Counter_Current_NB] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 1);

        //////////////////////////////////////////////////////////////////////////
        // Voltage
        m_colorsMap[PP_STR_Counter_Voltage_CPUCore] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 0);
        m_colorsMap[PP_STR_Counter_Voltage_NB] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 1);

        //////////////////////////////////////////////////////////////////////////
        // DVFS
        m_colorsMap[PP_STR_Counter_PState_Core0] = acGetAMDColorScaleColor(AC_AMD_GREEN, 0);
        m_colorsMap[PP_STR_Counter_PState_Core1] = acGetAMDColorScaleColor(AC_AMD_GREEN, 1);
        m_colorsMap[PP_STR_Counter_PState_Core2] = acGetAMDColorScaleColor(AC_AMD_GREEN, 2);
        m_colorsMap[PP_STR_Counter_PState_Core3] = acGetAMDColorScaleColor(AC_AMD_GREEN, 3);
        for (int nCpu = 0; nCpu < MAX_SUPPORTED_CPU; nCpu++)
        {
            QString cpuPStateStr = QString(PP_STR_Counter_PState_Core).arg(nCpu);
            m_colorsMap[cpuPStateStr] = acGetAMDColorScaleColor(AC_AMD_GREEN, nCpu);
        }

        m_colorsMap[PP_STR_Counter_C0Residency_CU0] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 0);
        m_colorsMap[PP_STR_Counter_C1Residency_CU0] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 1);
        m_colorsMap[PP_STR_Counter_CC6Residency_CU0] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 2);
        m_colorsMap[PP_STR_Counter_PC6Residency_CU0] = acGetAMDColorScaleColor(AC_AMD_ORANGE, 3);

        m_colorsMap[PP_STR_Counter_C0Residency_CU1] = acGetAMDColorScaleColor(AC_AMD_CYAN, 0);
        m_colorsMap[PP_STR_Counter_C1Residency_CU1] = acGetAMDColorScaleColor(AC_AMD_CYAN, 1);
        m_colorsMap[PP_STR_Counter_CC6Residency_CU1] = acGetAMDColorScaleColor(AC_AMD_CYAN, 2);
        m_colorsMap[PP_STR_Counter_PC6Residency_CU1] = acGetAMDColorScaleColor(AC_AMD_CYAN, 3);


        //////////////////////////////////////////////////////////////////////////
        // Count
        m_colorsMap[PP_STR_Counter_Process_Id_C0] = acGetAMDColorScaleColor(AC_AMD_GREEN, 0);
        m_colorsMap[PP_STR_Counter_Process_Id_C1] = acGetAMDColorScaleColor(AC_AMD_GREEN, 1);
        m_colorsMap[PP_STR_Counter_Process_Id_C2] = acGetAMDColorScaleColor(AC_AMD_GREEN, 2);
        m_colorsMap[PP_STR_Counter_Process_Id_C3] = acGetAMDColorScaleColor(AC_AMD_GREEN, 3);
    }
}

ppColorsMap& ppColorsMap::Instance()
{
    // If this class single instance was not already created
    if (m_colorsSingleInstance == nullptr)
    {
        // Create it
        m_colorsSingleInstance = new ppColorsMap;
        GT_ASSERT(m_colorsSingleInstance);
    }

    return *m_colorsSingleInstance;
}

//++AT:LPGPU2
/// @brief Convert from a range to another for signed integers
/// @param oldVal Old value to change to new range
/// @param oldMin Old minimum value of range
/// @param oldMax Old maximum value of range
/// @param newMin New minimum value of range
/// @param newMax New maximum value of range
/// @return Old value in new range
static gtInt32 ConvertToNewRange(gtInt32 oldVal, gtInt32 oldMin, gtInt32 oldMax,
    gtInt32 newMin, gtInt32 newMax)
{
  return (((oldVal - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
}

// Taken from https://stackoverflow.com/a/8317622
/// @brief Hash an unsigned integer
/// @param x The integer to hash
/// @return The hashed integer
static unsigned int Hash(unsigned int x)
{
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

/// @brief Get a QColor given an integer representing a counter ID
/// @param counterId The counter ID to hash to get a colour
/// @return A QColor calculated from the hash of counterId
QColor ppColorsMap::GetColorForCounterID(gtUInt32 counterId) const
{
  counterId = Hash(counterId);

  return QColor::fromHsv(
    static_cast<int>(ConvertToNewRange((kColourHashingMask & counterId),
      kColourMaskStartRange, kColourMaskEndRange, kColourHueStartRange,
        kColourHueEndRange)),
      kColourSaturation,
      kColourValue);
}
//--AT:LPGPU2

QColor ppColorsMap::GetColorForCounterName(const QString& counterName)
{
    QColor retColor = Qt::red;

    if (m_colorsMap.contains(counterName))
    {
        retColor = m_colorsMap[counterName];
    }
    else
    {
        // if the counter is a dGPU counter that is not yet in the list
        // define its color dynamically (as we don't know how many dGPUs there are)
        if (counterName.contains(PP_STR_TimeLineDgpuCounterPart))
        {
            m_colorsMap[counterName] = acGetAMDColorScaleColor(AC_AMD_RED, m_dgpusCount);
            m_dgpusCount++;

            retColor = m_colorsMap[counterName];
        }

#if AMDT_BUILD_TARGET == AMDT_DEBUG_BUILD
        else
        {
            gtString errMsg;
            QByteArray nameArr = counterName.toLocal8Bit();
            errMsg.fromASCIIString(nameArr.data());
            errMsg.prepend(L"No color defined for counter named: ");
            GT_ASSERT_EX(false, errMsg.asCharArray());
        }

#endif
    }

    return retColor;
}

//-----------------------------------------------------------------------------

ppHierarchyMap::ppHierarchyMap()
{
    m_hierarchyMap[PP_STR_Counter_Power_UVD] = PP_STR_Counter_Power_NB;
    m_hierarchyMap[PP_STR_Counter_Power_VCE] = PP_STR_Counter_Power_NB;
    m_hierarchyMap[PP_STR_Counter_Power_ACP] = PP_STR_Counter_Power_NB;
    m_hierarchyMap[PP_STR_Counter_Power_UNB] = PP_STR_Counter_Power_NB;
    m_hierarchyMap[PP_STR_Counter_Power_SMU] = PP_STR_Counter_Power_NB;
}

ppHierarchyMap& ppHierarchyMap::Instance()
{
    // If this class single instance was not already created
    if (m_hierarchyMapSingleInstance == nullptr)
    {
        // Create it
        m_hierarchyMapSingleInstance = new ppHierarchyMap;
        GT_ASSERT(m_hierarchyMapSingleInstance);
    }

    return *m_hierarchyMapSingleInstance;
}

bool ppHierarchyMap::IsChildCounter(const QString& counterName) const
{
    bool ret = false;

    if (m_hierarchyMap.contains(counterName))
    {
        ret = true;
    }

    return ret;
}

QString ppHierarchyMap::GetCounterParent(const QString& counterName) const
{
    QString ret = "";

    if (m_hierarchyMap.contains(counterName))
    {
        ret = m_hierarchyMap[counterName];
    }

    return ret;
}

QList<QString> ppHierarchyMap::GetChildrenListForCounter(const QString& counterName)
{
    QList<QString> childrenList;

    QMap<QString, QString>::Iterator it = m_hierarchyMap.begin();
    QMap<QString, QString>::Iterator itEnd = m_hierarchyMap.end();

    for (; it != itEnd; it++)
    {
        QString tmp = it.value();

        if (counterName == tmp)
        {
            childrenList.append(it.key());
        }
    }

    return childrenList;
}
