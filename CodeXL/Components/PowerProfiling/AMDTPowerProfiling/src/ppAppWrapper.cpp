//==================================================================================
// Copyright (c) 2016 , Advanced Micro Devices, Inc.  All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file ppAppWrapper.cpp
///
//==================================================================================

//------------------------------ ppAppWrapper.h ------------------------------

#include <QtWidgets>

// Infra:
#include <AMDTBaseTools/Include/gtAssert.h>

// Application Framework:
#include <AMDTApplicationFramework/Include/afPluginConnectionManager.h>
#include <AMDTApplicationFramework/Include/afProjectManager.h>
#include <AMDTApplicationFramework/Include/afQtCreatorsManager.h>
#include <AMDTSharedProfiling/inc/ProfileApplicationTreeHandler.h>

// Local:
#include <AMDTPowerProfiling/Include/ppAppWrapper.h>
#include <AMDTPowerProfiling/Include/ppStringConstants.h>
#include <AMDTPowerProfiling/Include/ppWindowsManagerHelper.h>
#include <AMDTPowerProfiling/src/ppAppController.h>
//++KB: LPGPU2
//++AT:LPGPU2
#if LPGPU2_ANDROID_REMOTE_PROTOCOL_COMPILE_IN == 1
  #include <AMDTPowerProfiling/src/ppProjectLPGPU2SettingsExtension.h>
#else
  #include <AMDTPowerProfiling/src/ppProjectSettingsExtension.h>
#endif
//--AT:LPGPU2
//--KB: LPGPU2
#include <AMDTPowerProfiling/src/ppMDIViewCreator.h>
#include <AMDTPowerProfiling/src/ppTreeHandler.h>

//++TLRS: LPGPU2
//#include <AMDTPowerProfiling/src/ppMenuActionsExecutor.h>
#include <AMDTPowerProfiling/src/LPGPU2ppMenuActionExecutor.h>
//--TLRS: LPGPU2

// Static members:
ppAppWrapper* ppAppWrapper::m_spMySingleInstance = nullptr;
//++KB: LPGPU2
//++AT:LPGPU2
#if LPGPU2_ANDROID_REMOTE_PROTOCOL_COMPILE_IN == 1
    ppProjectLPGPU2SettingsExtension* ppAppWrapper::m_psProjectLPGPU2SettingsExtension = nullptr;
#else
    ppProjectSettingsExtension* ppAppWrapper::m_psProjectSettingsExtension = nullptr;
#endif
//--AT:LPGPU2
//--KB: LPGPU2
ppMDIViewCreator* ppAppWrapper::m_psMDIViewCreator = nullptr;
ppMenuActionsExecutor* ppAppWrapper::m_spActionsCreator = nullptr;
bool ppAppWrapper::s_loadEnabled = false;

// ---------------------------------------------------------------------------
// Name:        ppAppWrapper::ppAppWrapper
// Description: Constructor
// Author:      Gilad Yarnitzky
// Date:        25/8/2014
// ---------------------------------------------------------------------------
ppAppWrapper::ppAppWrapper()
{

}
// ---------------------------------------------------------------------------
// Name:        ppAppWrapper::instance
// Description:
// Return Val:  ppAppWrapper&
// Author:      Gilad Yarnitzky
// Date:        25/8/2014
// ---------------------------------------------------------------------------
ppAppWrapper& ppAppWrapper::instance()
{
    // If this class single instance was not already created:
    if (m_spMySingleInstance == nullptr)
    {
        // Create it:
        m_spMySingleInstance = new ppAppWrapper;
        GT_ASSERT(m_spMySingleInstance);
    }

    return *m_spMySingleInstance;
}

// ---------------------------------------------------------------------------
// Name:        CheckValidity
// Description: check validity of the plug in
// Return Val:  void
// Author:      Gilad Yarnitzky
// Date:        25/8/2014
// ---------------------------------------------------------------------------
int ppAppWrapper::CheckValidity(gtString& errString)
{
    GT_UNREFERENCED_PARAMETER(errString);

    int retVal = 0;

    return retVal;
}

// ---------------------------------------------------------------------------
// Name:        initialize
// Description: Entry point for initialize
// Return Val:  void
// Author:      Gilad Yarnitzky
// Date:        25/8/2014
// ---------------------------------------------------------------------------
void ppAppWrapper::initialize()
{
    // These custom types need to be registered to enable Qt's signals mechanism to enqueue them.
    qRegisterMetaType<ppQtEventData>("ppQtEventData");
    qRegisterMetaType<ppQtPwrProfErrorCode>("ppQtPwrProfErrorCode");

    // Create an event observer:
    ppAppController& appController = ppAppController::instance();

    // Set windows manager to the appcontroller:
    appController.SetWindowsHelper(new ppWindowsManagerHelper);

    // Register the run mode manager:
    afPluginConnectionManager::instance().registerRunModeManager(&appController);

//++KB: LPGPU2
//++AT:LPGPU2
#if LPGPU2_ANDROID_REMOTE_PROTOCOL_COMPILE_IN == 1
    m_psProjectLPGPU2SettingsExtension = new ppProjectLPGPU2SettingsExtension;
    afProjectManager::instance().registerProjectSettingsExtension(m_psProjectLPGPU2SettingsExtension);
#else
    // Create and register the project settings object:
    m_psProjectSettingsExtension = new ppProjectSettingsExtension;

    afProjectManager::instance().registerProjectSettingsExtension(m_psProjectSettingsExtension);
#endif
//--AT:LPGPU2
//--KB: LPGPU2

    // Create a debug views creator:
    m_psMDIViewCreator = new ppMDIViewCreator;

    // Initialize (icons):
    m_psMDIViewCreator->initialize();

    // Create the main menu actions creator:
    //++TLRS: LPGPU2: Using the new Menu Action Executor
    //m_spActionsCreator = new ppMenuActionsExecutor();
    m_spActionsCreator = new (std::nothrow) LPGPU2ppMenuActionExecutor();
    //--TLRS: LPGPU2: Using the new Menu Action Executor

    // Register the actions creator:
    afQtCreatorsManager::instance().registerActionExecutor(m_spActionsCreator);

    // Register the views creator:
    afQtCreatorsManager::instance().registerViewCreator(m_psMDIViewCreator);

    // register the profile types
    SharedProfileManager::instance().registerProfileType(PP_STR_OnlineProfileName, &appController, PP_STR_projectSettingExtensionDisplayName, SPM_ALLOW_STOP | SPM_HIDE_PAUSE);

    // register the tree handler
    gtString offlineProfilingName(PP_STR_OnlineProfileName);
    ProfileApplicationTreeHandler::instance()->registerSessionTypeTreeHandler(offlineProfilingName.asASCIICharArray(), &ppTreeHandler::instance());
}

extern "C"
{
    // ---------------------------------------------------------------------------
    // Name:        CheckValidity
    // Description: check validity of the plug in
    // Return Val:  int - error type
    //              0 = no error
    //              != 0 error value and error string contains the error
    // Author:      Gilad Yarnitzky
    // Date:        25/8/2014
    // ---------------------------------------------------------------------------
    int CheckValidity(gtString& errString)
    {
        int retVal = ppAppWrapper::instance().CheckValidity(errString);

        ppAppWrapper::s_loadEnabled = (0 == retVal);

        return retVal;
    }

    // ---------------------------------------------------------------------------
    // Name:        initialize
    // Description: Entry point for initialize
    // Return Val:  void
    // Author:      Gilad Yarnitzky
    // Date:        25/8/2014
    // ---------------------------------------------------------------------------
    void initialize()
    {
        ppAppWrapper::instance().initialize();
    }


    // ---------------------------------------------------------------------------
    // Name:        initializeIndependentWidgets
    // Description: initialize other gui items that can be done only after main window is alive
    // Return Val:  void GW_API
    // Author:      Gilad Yarnitzky
    // Date:        25/8/2014
    // ---------------------------------------------------------------------------
    void initializeIndependentWidgets()
    {
    }

}; // extern "C"

