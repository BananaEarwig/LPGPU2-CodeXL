//==================================================================================
// Copyright (c) 2013-2016 , Advanced Micro Devices, Inc.  All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file FunctionsDataTable.cpp
///
//==================================================================================

// Qt
#include <qtIgnoreCompilerWarnings.h>

// Infra:
#include <AMDTOSWrappers/Include/osFilePath.h>
#include <AMDTBaseTools/Include/gtAlgorithms.h>
#include <AMDTApplicationComponents/Include/acMessageBox.h>
#include <AMDTApplicationComponents/Include/acItemDelegate.h>
#include <AMDTOSWrappers/Include/osDebugLog.h>

// AMDTApplicationFramework:
#include <AMDTApplicationFramework/src/afUtils.h>
#include <AMDTApplicationFramework/Include/afGlobalVariablesManager.h>
#include <AMDTApplicationFramework/Include/afProgressBarWrapper.h>

/// Local:
#include <inc/Auxil.h>
#include <inc/CPUProfileUtils.h>
#include <inc/DebugUtils.h>
#include <inc/FunctionsDataTable.h>
#include <inc/StringConstants.h>
#include <inc/SessionWindow.h>
#include <SessionTreeNodeData.h>

#define FunctionDataIndexRole  ((int)(Qt::UserRole) + 0)
const AMDTFunctionId INVALID_FUNCTION_ID = 0;

FunctionsDataTable::FunctionsDataTable(QWidget* pParent,
                                       const gtVector<TableContextMenuActionType>& additionalContextMenuActions,
                                       SessionTreeNodeData* pSessionData,
                                       CpuSessionWindow* pSessionWindow) :
    CPUProfileDataTable(pParent, additionalContextMenuActions, pSessionData),
    m_pParentSessionWindow(pSessionWindow)
{
    m_functionNameColIndex = -1;
}

FunctionsDataTable::~FunctionsDataTable()
{
}

QString FunctionsDataTable::getModuleName(int rowIndex) const
{
    QString name;

    // module column index varies for CLU and
    // other profiling
    int moduleColIdx = 4;

    if (m_isCLU)
    {
        moduleColIdx = 3;
    }

    GT_IF_WITH_ASSERT((rowIndex >= 0) && (rowIndex < rowCount()))
    {
        QTableWidgetItem* pNameTableItem = item(rowIndex, moduleColIdx);

        GT_IF_WITH_ASSERT(nullptr != pNameTableItem)
        {
            name = pNameTableItem->toolTip();
        }
    }

    return name;
}

QString FunctionsDataTable::getFunctionName(int rowIndex) const
{
    QString name;

    GT_IF_WITH_ASSERT((rowIndex >= 0) && (rowIndex < rowCount()))
    {
        QTableWidgetItem* pNameTableItem = item(rowIndex, 1);

        GT_IF_WITH_ASSERT(nullptr != pNameTableItem)
        {
            name = pNameTableItem->text();
        }
    }

    return name;
}

QString FunctionsDataTable::getFunctionId(int rowIndex) const
{
    QString name;

    GT_IF_WITH_ASSERT((rowIndex >= 0) && (rowIndex < rowCount()))
    {
        QTableWidgetItem* pNameTableItem = item(rowIndex, 0);

        GT_IF_WITH_ASSERT(nullptr != pNameTableItem)
        {
            name = pNameTableItem->text();
        }
    }

    return name;
}

bool FunctionsDataTable::HandleHotSpotIndicatorSet()
{
    // call directly to setHotSpotIndicatorValues, without calling to displayProfileData first
    // boolean to function has no meaning
    //return setHotSpotIndicatorValues();
    return true;
}

void FunctionsDataTable::onAboutToShowContextMenu()
{
    // Call the base class implementation:
    CPUProfileDataTable::onAboutToShowContextMenu();

    GT_IF_WITH_ASSERT((m_pContextMenu != nullptr) &&
                      (m_pTableDisplaySettings != nullptr) &&
                      (m_pParentSessionWindow != nullptr) &&
                      (m_pProfDataRdr != nullptr))
    {
        foreach (QAction* pAction, m_pContextMenu->actions())
        {
            if (pAction != nullptr)
            {
                bool isActionEnabled = true;

                if (pAction->data().isValid())
                {
                    TableContextMenuActionType actionType = (TableContextMenuActionType)pAction->data().toInt();

                    if (selectedItems().count() > 0)
                    {
                        AMDTFunctionId funcId = INVALID_FUNCTION_ID;

                        // get the selected item (only one item)
                        QTableWidgetItem* pItem = selectedItems().first();

                        if (nullptr != pItem)
                        {
                            int rowIndex = pItem->row();
                            QString funcIdStr = getFunctionId(rowIndex);
                            funcId = funcIdStr.toInt();
                        }

                        // if its empty or is the "empty row" string item
                        if (nullptr == pItem || pItem->row() == -1)
                        {
                            isActionEnabled = false;
                        }

                        //if more then one row selected
                        else if (columnCount() != 0 &&
                                 (selectedItems().count() / columnCount()) > 1)
                        {
                            isActionEnabled = false;
                        }

                        else if (DISPLAY_FUNCTION_IN_CALLGRAPH_VIEW == actionType)
                        {
                            isActionEnabled = false;

                            //get supported call graph process
                            gtVector<AMDTProcessId> cssProcesses;
                            bool rc = m_pProfDataRdr->GetCallGraphProcesses(cssProcesses);

                            if ((INVALID_FUNCTION_ID != funcId) && (true == rc))
                            {
                                for (auto const& process : cssProcesses)
                                {
                                    AMDTProfileFunctionData  functionData;
                                    bool retVal = m_pProfDataRdr->GetFunctionData(funcId,
                                                                                  process,
                                                                                  AMDT_PROFILE_ALL_THREADS,
                                                                                  functionData);

                                    if (retVal)
                                    {
                                        if (!functionData.m_pidsList.empty())
                                        {
                                            if (process == functionData.m_pidsList.at(0))
                                            {
                                                isActionEnabled = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if (DISPLAY_FUNCTION_IN_SOURCE_CODE_VIEW == actionType)
                        {
                            QTableWidgetItem* pItem = selectedItems().first();

                            if (nullptr != pItem)
                            {
                                int rowIndex = pItem->row();
                                gtString funcName = acQStringToGTString(getFunctionName(rowIndex));

                                if (funcName.startsWith(L"Unknown Module"))
                                {
                                    isActionEnabled = false;
                                }
                            }

                            if (INVALID_FUNCTION_ID == funcId)
                            {
                                isActionEnabled = false;
                            }
                        }
                    }
                    else
                    {
                        // if no items selected
                        isActionEnabled = false;
                    }

                    // Enable / disable the action:
                    pAction->setEnabled(isActionEnabled);
                }
            }
        }
    }
}

CPUProfileDataTable::TableType FunctionsDataTable::GetTableType() const
{
    return CPUProfileDataTable::FUNCTION_DATA_TABLE;
}

bool FunctionsDataTable::setModuleIcon(int row,
                                       const AMDTProfileModuleInfo& moduleInfo)
{
    bool retVal = false;

    GT_IF_WITH_ASSERT((m_pProfDataRdr != nullptr) &&
                      (m_pDisplayFilter != nullptr) &&
                      (m_pTableDisplaySettings != nullptr))
    {
        osFilePath iconFile;
        iconFile.setFileName(moduleInfo.m_name);
        iconFile.setFullPathFromString(moduleInfo.m_path);

        QPixmap* pIcon = CPUProfileDataTable::moduleIcon(iconFile, true);

        // Initialize the function row:
        QTableWidgetItem* pNameItem = item(row, 0);

        if (pNameItem != nullptr)
        {
            // Set the original position in function vector:
            pNameItem->setData(FunctionDataIndexRole, QVariant(0));

            if (pIcon != nullptr)
            {
                pNameItem->setIcon(QIcon(*pIcon));
            }
        }

        retVal = true;
    }

    return retVal;
}

bool FunctionsDataTable::fillSummaryTables(int counterIdx)
{
    bool retVal = false;

    if (nullptr != m_pProfDataRdr)
    {
        AMDTProfileCounterDescVec counterDesc;
        bool rc = m_pProfDataRdr->GetSampledCountersList(counterDesc);

        AMDTProfileDataVec funcProfileData;
        rc = m_pProfDataRdr->GetFunctionSummary(counterDesc.at(counterIdx).m_id,
                                                funcProfileData);
        GT_ASSERT(rc);

        setSortingEnabled(false);

        for (auto profData : funcProfileData)
        {
            // create QstringList to hold the values
            QStringList list;

            // insert the function id
            QVariant mId(static_cast<qlonglong>(profData.m_id));
            list << mId.toString();

            list << profData.m_name.asASCIICharArray();

            int precision = SAMPLE_VALUE_PRECISION;

            const AMDTSampleValueVec& sampleVector = profData.m_sampleValue;

            if (sampleVector.empty() || sampleVector.at(0).m_sampleCount == 0)
            {
                continue;
            }

            if (m_isCLU)
            {
                if (m_pDisplayFilter->isCLUPercentCaptionSet())
                {
                    precision = SAMPLE_PERCENT_PRECISION;
                }

                list << QString::number(sampleVector.at(0).m_sampleCount, 'f', precision);
            }
            else
            {
                QVariant sampleCount(sampleVector.at(0).m_sampleCount);
                list << sampleCount.toString();

                QVariant sampleCountPercent(sampleVector.at(0).m_sampleCountPercentage);
                list << QString::number(sampleVector.at(0).m_sampleCountPercentage, 'f', precision);
            }

            AMDTProfileModuleInfoVec procInfo;
            rc = m_pProfDataRdr->GetModuleInfo(AMDT_PROFILE_MAX_VALUE, profData.m_moduleId, procInfo);
            GT_ASSERT(rc);

            if (procInfo.empty())
            {
                continue;
            }

            if (profData.m_moduleId == AMDT_PROFILE_ALL_MODULES)
            {
                list << "";
            }
            else
            {
                list << procInfo.at(0).m_name.asASCIICharArray();
            }

            addRow(list, nullptr);

            int functionCol = AMDT_FUNC_SUMMMARY_FUNC_NAME_COL;
            int sampleCol = AMDT_FUNC_SUMMMARY_FUNC_MODULE_COL;

            if (true == rc)
            {
                if (!m_isCLU)
                {
                    rc = delegateSamplePercent(AMDT_FUNC_SUMMMARY_FUNC_PER_SAMPLE_COL);
                }
                else
                {
                    if (m_pDisplayFilter->isCLUPercentCaptionSet())
                    {
                        delegateSamplePercent(AMDT_FUNC_SUMMMARY_FUNC_SAMPLE_COL);
                    }
                    else
                    {
                        setItemDelegateForColumn(AMDT_FUNC_SUMMMARY_FUNC_SAMPLE_COL, &acNumberDelegateItem::Instance());
                    }

                    sampleCol = AMDT_FUNC_SUMMMARY_FUNC_SAMPLE_COL + 1;
                }
            }

            SetIcon(procInfo.at(0).m_path,
                    rowCount() - 1,
                    functionCol,
                    sampleCol,
                    !procInfo.at(0).m_is64Bit,
                    FunctionDataIndexRole);
        }

        setSortingEnabled(true);

        hideColumn(AMDT_FUNC_SUMMMARY_FUNC_ID_COL);

        setColumnWidth(AMDT_FUNC_SUMMMARY_FUNC_NAME_COL, MAX_FUNCTION_NAME_LEN);
        resizeColumnToContents(AMDT_FUNC_SUMMMARY_FUNC_SAMPLE_COL);
        resizeColumnToContents(AMDT_FUNC_SUMMMARY_FUNC_PER_SAMPLE_COL);
        resizeColumnToContents(AMDT_FUNC_SUMMMARY_FUNC_MODULE_COL);

        retVal = true;
    }

    return retVal;
}

bool FunctionsDataTable::AddRowToTable(const gtVector<AMDTProfileData>& allModuleData)
{
    if (!allModuleData.empty())
    {
        setSortingEnabled(false);

        for (auto profData : allModuleData)
        {
            QStringList list;

            CounterNameIdVec selectedCounterList;

            // insert the function id
            QVariant mId(static_cast<qlonglong>(profData.m_id));
            list << mId.toString();

            // Insert function name
            list << profData.m_name.asASCIICharArray();

            // insert module name
            AMDTProfileModuleInfoVec procInfo;
            m_pProfDataRdr->GetModuleInfo(AMDT_PROFILE_ALL_PROCESSES, profData.m_moduleId, procInfo);

            // if module info null return
            if (procInfo.empty())
            {
                continue;
            }

            list << acGTStringToQString(procInfo.at(0).m_name);

            m_pDisplayFilter->GetSelectedCounterList(selectedCounterList);
            int i = 0;

            for (auto counter : selectedCounterList)
            {
                // get counter type
                AMDTProfileCounterType counterType = static_cast<AMDTProfileCounterType>(std::get<4>(counter));
                bool setSampleValue = true;

                if (counterType == AMDT_PROFILE_COUNTER_TYPE_RAW)
                {
                    if (m_pDisplayFilter->GetSamplePercent() == true)
                    {
                        list << QString::number(profData.m_sampleValue.at(i++).m_sampleCountPercentage, 'f', SAMPLE_PERCENT_PRECISION);
                        delegateSamplePercent(AMDT_FUNC_FUNC_MODULE_COL + i);
                        setSampleValue = false;
                    }
                }

                if (true == setSampleValue)
                {
                    double sampleCnt = profData.m_sampleValue.at(i++).m_sampleCount;
                    setItemDelegateForColumn(AMDT_FUNC_FUNC_MODULE_COL + i, &acNumberDelegateItem::Instance());

                    if (0 == sampleCnt)
                    {
                        list << "";
                    }
                    else
                    {
                        list << QString::number(sampleCnt, 'f', SAMPLE_VALUE_PRECISION);
                    }
                }
            }

            addRow(list, nullptr);

            SetIcon(procInfo.at(0).m_path,
                    rowCount() - 1,
                    AMDT_FUNC_SUMMMARY_FUNC_NAME_COL,
                    AMDT_FUNC_SUMMMARY_FUNC_MODULE_COL,
                    !procInfo.at(0).m_is64Bit,
                    FunctionDataIndexRole);

        }
    }

    return true;
}

bool FunctionsDataTable::fillTableData(AMDTProcessId procId, AMDTModuleId modId, std::vector<AMDTUInt64> modIdVec)
{
    bool retVal = false;

    GT_IF_WITH_ASSERT((m_pProfDataRdr != nullptr) &&
                      (m_pDisplayFilter != nullptr) &&
                      (m_pTableDisplaySettings != nullptr))
    {
        gtVector<AMDTProfileData> allModuleData;

        if (modIdVec.empty())
        {
            AMDTProfileSessionInfo sessionInfo;

            bool rc = m_pProfDataRdr->GetProfileSessionInfo(sessionInfo);
            GT_ASSERT(rc);

            rc = m_pProfDataRdr->GetFunctionProfileData(procId, modId, allModuleData);
            GT_ASSERT(rc);

        }
        else
        {
            gtVector<AMDTProfileData> moduleData;

            for (const auto& moduleId : modIdVec)
            {
                moduleData.clear();
                bool rc = m_pProfDataRdr->GetFunctionProfileData(procId, moduleId, moduleData);
                GT_ASSERT(rc);
                allModuleData.insert(allModuleData.end(), moduleData.begin(), moduleData.end());
            }

            retVal = true;
        }

        AddRowToTable(allModuleData);

        setColumnWidth(AMDT_FUNC_SUMMMARY_FUNC_NAME_COL, MAX_FUNCTION_NAME_LEN);
        hideColumn(AMDT_FUNC_SUMMMARY_FUNC_ID_COL);
        setColumnWidth(AMDT_FUNC_SUMMMARY_FUNC_NAME_COL + 1, MAX_MODULE_NAME_LEN);

    }

    return retVal;
}
