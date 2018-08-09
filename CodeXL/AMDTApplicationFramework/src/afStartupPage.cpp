//==================================================================================
// Copyright (c) 2016 , Advanced Micro Devices, Inc.  All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file afStartupPage.cpp
///
//==================================================================================

#include <QtWidgets>
#include <QNetworkRequest>

// Infra:
#include <AMDTBaseTools/Include/gtAssert.h>
#include <AMDTOSWrappers/Include/osDebuggingFunctions.h>
#include <AMDTOSWrappers/Include/osFile.h>
#include <AMDTOSWrappers/Include/osFileLauncher.h>
#include <AMDTAPIClasses/Include/apFrameTerminators.h>
#include <AMDTApplicationComponents/Include/acColours.h>
#include <AMDTApplicationComponents/Include/acFunctions.h>
#include <AMDTApplicationComponents/Include/acMessageBox.h>

// AMDTApplicationFramework:
#include <AMDTApplicationFramework/Include/afAppStringConstants.h>
#include <AMDTApplicationFramework/Include/afApplicationCommands.h>
#include <AMDTApplicationFramework/Include/afExecutionModeManager.h>
#include <AMDTApplicationFramework/Include/afGlobalVariablesManager.h>
#include <AMDTApplicationFramework/Include/afLoadProjectCommand.h>
#include <AMDTApplicationFramework/Include/afPluginConnectionManager.h>
#include <AMDTApplicationFramework/Include/afProjectManager.h>
#include <AMDTApplicationFramework/Include/afQMdiSubWindow.h>
#include <AMDTApplicationFramework/Include/afStartupPage.h>
#include <AMDTApplicationFramework/src/afUtils.h>

// LPGPU2 Project Code Coverage
#if (AMDT_BUILD_TARGET == AMDT_WINDOWS_OS)
#include <../../../../../../CodeXL/Components/LPGPU2_CCVG/CCVGCodeCoveragePlugin/Include/CCVGProjectBuildDefinitions.h>
#else
// Linux build
#include <../../CodeXL/Components/LPGPU2_CCVG/CCVGCodeCoveragePlugin/Include/CCVGProjectBuildDefinitions.h>
#endif // (AMDT_BUILD_TARGET == AMDT_WINDOWS_OS)

//++IOR: LPGPU2 Code Coverage Analysis
#define AF_STR_startup_page_newProjectLPGPU2CCGLink "new_project_code_coverage"
//--IOR: LPGPU2 Code Coverage Analysis
#define AF_STR_startup_page_newProjectLink "new_project"
#define AF_STR_startup_page_newProjectProfileLink "new_project_profile"
#define AF_STR_startup_page_newProjectDebugLink "new_project_debug"
#define AF_STR_startup_page_attachToProcess "attach_to_process"
#define AF_STR_startup_page_sysWide "system_wide"
#define AF_STR_startup_page_newProjectFrameAnalyzeLink "new_project_frame_analyze"
#define AF_STR_startup_page_newClForAnalyze "new_analyze"
#define AF_STR_startup_page_addClForAnalyze "add_analyze"
#define AF_STR_startup_page_openProjectLink "open_project"
#define AF_STR_startup_page_loadTeapotProjectLink "load_teapot"
#define AF_STR_startup_page_loadMatMulProjectLink "load_matmul"
#define AF_STR_startup_page_loadD3D12MultithreadingProjectLink "load_d3d12multithreading"
#define AF_STR_startup_page_helpLink "show_help"
#define AF_STR_startup_page_quickStartLink "show_quick_start"
#define AF_STR_startup_page_aboutLink "show_about"
#define AF_STR_startup_page_webLink "show_website"
#define AF_STR_startup_page_openProjectLinkPrefix AF_STR_startup_page_openProjectLink "_"
#define AF_STR_startup_page_noRecentProjects "No recent projects"


afWebPage::afWebPage(QObject* pParent) : QWebPage(pParent) , m_pApplicationCommands(nullptr)
{
    m_pApplicationCommands = afApplicationCommands::instance();
    GT_ASSERT(m_pApplicationCommands != nullptr);
}

void afWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
{
    (void)(sourceID);
    (void)(lineNumber);

    // This function is called whenever the buttonClick implemented in JS is called in the welcome page HTML.
    // The function is handling the messages related to CodeXL:

    GT_IF_WITH_ASSERT(m_pApplicationCommands != nullptr)
    {
        if (message == AF_STR_startup_page_newProjectLink)
        {
            m_pApplicationCommands->OnFileNewProject();
        }
        else if (message == AF_STR_startup_page_newProjectDebugLink)
        {
            afExecutionModeManager::instance().RunUserStartupAction(AF_NO_PROJECT_USER_ACTION_CREATE_NEW_PROJECT_DEBUG);
        }
        else if (message == AF_STR_startup_page_newProjectProfileLink)
        {
            afExecutionModeManager::instance().RunUserStartupAction(AF_NO_PROJECT_USER_ACTION_CREATE_NEW_PROJECT_PROFILE);
        }
        else if (message == AF_STR_startup_page_attachToProcess)
        {
            afExecutionModeManager::instance().RunUserStartupAction(AF_NO_PROJECT_USER_ACTION_CPU_PROFILE_ATTACH_TO_PROCESS);
        }
        else if (message == AF_STR_startup_page_sysWide)
        {
            afExecutionModeManager::instance().RunUserStartupAction(AF_NO_PROJECT_USER_ACTION_CPU_PROFILE_SYSTEM_WIDE);
        }
        else if (message == AF_STR_startup_page_newProjectFrameAnalyzeLink)
        {
            afExecutionModeManager::instance().RunUserStartupAction(AF_NO_PROJECT_USER_ACTION_CREATE_NEW_PROJECT_FRAME_ANALYZE);
        }
        else if (message == AF_STR_startup_page_newClForAnalyze)
        {
            afExecutionModeManager::instance().RunUserStartupAction(AF_NO_PROJECT_USER_ACTION_NEW_FILE_FOR_ANALYZE);
        }
        else if (message == AF_STR_startup_page_addClForAnalyze)
        {
          afExecutionModeManager::instance().RunUserStartupAction(AF_NO_PROJECT_USER_ACTION_ADD_FILE_FOR_ANALYZE);
        }
//++IOR: LPGPU2 Code Coverage Analysis
#if LPGPU2_CCVG_COMPILE_IN
        else if (message == AF_STR_startup_page_newProjectLPGPU2CCGLink)
        {
          afExecutionModeManager::instance().RunUserStartupAction(AF_NO_PROJECT_USER_ACTION_CREATE_NEW_PROJECT_LPGPU2_CCVG);
        }
#endif // LPGPU2_CCVG_COMPILE_IN
//--IOR: LPGPU2 Code Coverage Analysis
        else if (message == AF_STR_startup_page_openProjectLink)
        {
            m_pApplicationCommands->OnFileOpenProject(L"");
        }
        else if (message == AF_STR_startup_page_loadTeapotProjectLink)
        {
            // Open the teapot sample project:
            m_pApplicationCommands->LoadSample(AF_TEAPOT_SAMPLE);
        }
        else if (message == AF_STR_startup_page_loadMatMulProjectLink)
        {
            // Open the teapot sample project:
            m_pApplicationCommands->LoadSample(AF_MATMUL_SAMPLE);
        }
        else if (message == AF_STR_startup_page_loadD3D12MultithreadingProjectLink)
        {
            // Open the teapot sample project:
            m_pApplicationCommands->LoadSample(AF_D3D12MULTITHREADING_SAMPLE);
        }
        else if (message == AF_STR_startup_page_helpLink)
        {
            m_pApplicationCommands->onHelpUserGuide();
        }
        else if (message == AF_STR_startup_page_quickStartLink)
        {
            m_pApplicationCommands->onHelpQuickStart();
        }
        else if (message == AF_STR_startup_page_webLink)
        {
            m_pApplicationCommands->openCodeXLWebsite();
        }
        else if (message == AF_STR_startup_page_aboutLink)
        {
            m_pApplicationCommands->onHelpAbout();
        }
        else if (message.startsWith(AF_STR_startup_page_openProjectLinkPrefix))
        {
            // Remove the prefix:
            QString messageWithPrefix = message;
            messageWithPrefix.replace(AF_STR_startup_page_openProjectLinkPrefix, "");

            gtString projectName;
            projectName.fromASCIIString(messageWithPrefix.toLatin1().data());

            if (projectName != afProjectManager::instance().currentProjectSettings().projectName())
            {
                // Build the file path for the project:
                osFilePath projectPath;
                m_pApplicationCommands->getProjectsFilePath(projectName, projectPath);
                m_pApplicationCommands->OnFileOpenProject(projectPath.asString());
            }
        }
    }

    else
    {
        GT_ASSERT_EX(false, L"Unsupported action in welcome HTML");
    }
}

bool afWebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, QWebPage::NavigationType type)
{
    GT_UNREFERENCED_PARAMETER(frame);
    GT_UNREFERENCED_PARAMETER(request);
    bool retVal = true;

    // Do not allow navigation clicks:
    if (type == QWebPage::NavigationTypeLinkClicked)
    {
        retVal = false;
    }

    return retVal;
}

// ---------------------------------------------------------------------------
// Name:        afStartupPage::afStartupPage
// Description: Constructor
// Arguments:   QWidget* pParent
// Author:      Sigal Algranaty
// Date:        21/2/2012
// ---------------------------------------------------------------------------
afStartupPage::afStartupPage() : QWebView()
{

    // Set the zoom factor to fit the resolution. QWebView assumes 96 DPI.
    QWidget* pScreen = QApplication::desktop()->screen();


    page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    bool rc = connect(this, SIGNAL(linkClicked(const QUrl&)), this, SLOT(OnLinkClicked(const QUrl&)));
    GT_ASSERT(rc);

    // Allow focus in this widget:
    setFocusPolicy(Qt::ClickFocus);

    afWebPage* pPage = new afWebPage(nullptr);
    setPage(pPage);

    // Update the HTML file:
    UpdateHTML();

    // Hide context menu:
    setContextMenuPolicy(Qt::NoContextMenu);

    if (pScreen != nullptr)
    {
        const int horizontalDpi = pScreen->logicalDpiX();
        setZoomFactor(horizontalDpi / 96.0);
    }
}

// ---------------------------------------------------------------------------
// Name:        afStartupPage::~afStartupPage
// Description: Destructor
// Author:      Sigal Algranaty
// Date:        22/2/2012
// ---------------------------------------------------------------------------
afStartupPage::~afStartupPage()
{

}

// ---------------------------------------------------------------------------
// Name:        afStartupPage::updateHTML
// Description: Update the HTML text with the current recently used projects
// Return Val:  bool - Success / failure.
// Author:      Sigal Algranaty
// Date:        21/2/2012
// ---------------------------------------------------------------------------
bool afStartupPage::UpdateHTML()
{
    bool retVal = true;

    // Find the right HTML file name:
    QString fileName = AF_STR_WelcomeFileName;
    unsigned int installedComponentsBitmask = afGlobalVariablesManager::instance().InstalledAMDComponentsBitmask();

    const bool bAnalyzeNA = !((installedComponentsBitmask & AF_AMD_GPU_COMPONENT) && (installedComponentsBitmask & AF_AMD_CATALYST_COMPONENT));
    if (bAnalyzeNA)
    {
        // If analyze mode is disabled, display the right HTML file:
        fileName = AF_STR_WelcomeFileNameAnalyzeModeNA;
    }

//++IOR: LPGPU2 Code Coverage Analysis
#if LPGPU2_CCVG_COMPILE_IN
#if LPGPU2_CCVG_COMPILE_OUT_CODECOVERAGE_ONLY_EXE_MODE == 1
    fileName = AF_STR_WelcomeFileNameCCVGOnly;
#else
    if (bAnalyzeNA)
      fileName = AF_STR_WelcomeFileNameAnalyzeModeNACCVG;
    else
      fileName = AF_STR_WelcomeFileNameCCVG;
#endif // LPGPU2_CCVG_COMPILE_OUT_CODECOVERAGE_ONLY_EXE_MODE
#endif // LPGPU2_CCVG_COMPILE_IN
//--IOR: LPGPU2 Code Coverage Analysis

    // Get the HTML welcome page from CodeXL binary folder:
    osFilePath codeXLWelcomePagePath;
    QString htmlText, cssText, htmlFilePath;
    bool rc1 = codeXLWelcomePagePath.SetInstallRelatedPath(osFilePath::OS_CODEXL_BINARIES_PATH);
    GT_IF_WITH_ASSERT(rc1)
    {
        // Complete the welcome HTML file path:
        codeXLWelcomePagePath.appendSubDirectory(AF_STR_WelcomeDirectory);
        codeXLWelcomePagePath.setFileName(acQStringToGTString(fileName));

        // Load the file into a QString:
        gtString fileNameWithExt = codeXLWelcomePagePath.asString();
        htmlFilePath = acGTStringToQString(fileNameWithExt);
        QFile file(htmlFilePath);
        bool rc = file.open(QIODevice::ReadOnly | QIODevice::Text);
        GT_IF_WITH_ASSERT(rc)
        {
            QTextStream in(&file);

            while (!in.atEnd())
            {
                QString line = in.readLine();
                htmlText.append(line);
            }
        }

        file.close();

    }

    // Find the base URL (to enable links):
    QUrl baseUrl = QUrl::fromLocalFile(htmlFilePath);

    // Build the recent projects table:
    BuildRecentlyOpenedProjectsTable(htmlText);

    // Set the html text:
    setHtml(htmlText, baseUrl);

    return retVal;
}

// ---------------------------------------------------------------------------
// Name:        afStartupPage::onLinkClick
// Description: Implement the startup page click event
// Return Val:  void
// Author:      Sigal Algranaty
// Date:        21/2/2012
// ---------------------------------------------------------------------------
void afStartupPage::OnLinkClicked(const QUrl& url)
{
    // Make sure the parent mdi window will not close:
    afQMdiSubWindow* pParentWindow = qobject_cast<afQMdiSubWindow*>(parentWidget());

    if (nullptr != pParentWindow)
    {
        pParentWindow->SetAllowedToBeClosed(false);
    }

    // Check if the operation requires a stop debugging:
    bool shouldContinue = CanLinkBeClicked(url);

    if (shouldContinue)
    {
        if (!url.toString().toLower().contains("welcome.html"))
        {
            // External links:
            osFileLauncher fileLauncher(acQStringToGTString(url.toString()));
            fileLauncher.launchFile();
        }
    }

    if (nullptr != pParentWindow)
    {
        pParentWindow->SetAllowedToBeClosed(true);
    }
}

// ---------------------------------------------------------------------------
// Name:        afStartupPage::setSource
// Description: Prevent Qt from activating links
// Arguments:   const QUrl & name
// Author:      Sigal Algranaty
// Date:        28/2/2012
// ---------------------------------------------------------------------------
void afStartupPage::setSource(const QUrl& name)
{
    GT_UNREFERENCED_PARAMETER(name);
}


// ---------------------------------------------------------------------------
// Name:        afStartupPage::BuildRecentlyOpenedProjectsTable
// Description: Append a table with the recently opened projects
// Arguments:   QString& tableStr
// Author:      Sigal Algranaty
// Date:        28/2/2012
// ---------------------------------------------------------------------------
bool afStartupPage::BuildRecentlyOpenedProjectsTable(QString& htmlText)
{
    bool retVal = false;

    // Sanity check:
    afApplicationCommands* pApplicationCommands = afApplicationCommands::instance();
    GT_IF_WITH_ASSERT(pApplicationCommands != nullptr)
    {
        QString recentProjectsTableStr;

        gtString appName;
        gtVector<gtString> recentlyUsedProjectsNames;
        pApplicationCommands->FillRecentlyUsedProjectsNames(recentlyUsedProjectsNames, appName);
        int numberOfRecentProjects = (int)recentlyUsedProjectsNames.size();
        int projectsForDisplayNumber = std::min(numberOfRecentProjects, 5);

        if (0 < projectsForDisplayNumber)
        {
            for (int i = 0; i < projectsForDisplayNumber; i++)
            {
                // Get the current recent project path:
                gtString currentProjectPath = recentlyUsedProjectsNames[i];

                // Build the file path from the project path:
                osFilePath filePath(currentProjectPath);

                // Get the project name:
                gtString currentProjectName;
                filePath.getFileName(currentProjectName);
                QString wsName = acGTStringToQString(currentProjectName);
                QString currentProjectCell = QString(" <tr><td><a onclick = 'clickButton(\"" AF_STR_startup_page_openProjectLinkPrefix "%1\")'>%2</a></td></tr>").arg(wsName).arg(wsName);
                recentProjectsTableStr.append(currentProjectCell);
            }
        }
        else // 0 >= numberOfRecentProjects
        {
            recentProjectsTableStr.append("<p class='indentlargetext'>" AF_STR_startup_page_noRecentProjects "</p>");
        }


        // Find the position of the recent projects table in the original HTML text:
        int tableHeadPos = htmlText.indexOf("<thead><img src='recent_projects.png' /></thead>", 0);

        if (tableHeadPos > 0)
        {
            // Find the position of the table body:
            int tableBodyStartPos = htmlText.indexOf("<tbody>", tableHeadPos + 1);
            int tableBodyEndPos = htmlText.indexOf("</tbody>", tableBodyStartPos + 1);

            GT_IF_WITH_ASSERT((tableBodyStartPos > 0) && (tableBodyEndPos > 0) && (tableBodyEndPos > tableBodyStartPos))
            {
                // Get the length of the replaced string:
                int len = tableBodyEndPos - tableBodyStartPos;

                htmlText = htmlText.replace(tableBodyStartPos + 8, len, recentProjectsTableStr);

                retVal = true;
            }
        }
    }


    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        afStartupPage::canLinkBeClicked
// Description: If the operation requested by url requires stop debugging, ask
//              the user how to behave
// Arguments:   const QUrl& url
// Return Val:  bool - true if operation can continue (debug/profile is stopped or wasn't on), false otherwise
// Author:      Sigal Algranaty
// Date:        27/3/2012
// ---------------------------------------------------------------------------
bool afStartupPage::CanLinkBeClicked(const QUrl& url)
{
    bool retVal = true;
    bool requiresDebugStop = false;

    QString urlAsQString = url.toString();

    if ((urlAsQString == AF_STR_startup_page_newProjectLink) || (urlAsQString == AF_STR_startup_page_openProjectLink) ||
        (urlAsQString == AF_STR_startup_page_loadTeapotProjectLink) || (urlAsQString == AF_STR_startup_page_loadMatMulProjectLink) ||
        (urlAsQString == AF_STR_startup_page_loadD3D12MultithreadingProjectLink) || (urlAsQString.startsWith(AF_STR_startup_page_openProjectLinkPrefix)))
    {
        // If the user is in the middle of debugging:
        afPluginConnectionManager& thePluginConnectionManager = afPluginConnectionManager::instance();

        if (AF_DEBUGGED_PROCESS_EXISTS == (thePluginConnectionManager.getCurrentRunModeMask() & AF_DEBUGGED_PROCESS_EXISTS))
        {
            requiresDebugStop = true;
        }
    }

    if (requiresDebugStop)
    {
        // If the user is in the middle of debugging - ask if he/she wants to stop and exit
        retVal = afApplicationCommands::instance()->promptForExit();
    }

    return retVal;
}