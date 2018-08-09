// Copyright (C) 2002-2018 Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief CodeXL execution mode Code Coverage. LPGPU2 (== LP2) Code Coverage 
///        DLL plugin to sit along side other DLL plugins (execution modes)  
///        such as GPUProfilng or CPUProfiling.
///
/// CCVGCmdCmdProjectNew implementation.
///
/// @copyright
/// Copyright (C) 2002-2018 Codeplay Software Limited. All Rights Reserved.

// Qt:
#include <QDir>
#include <QDirIterator>

// Infra:
#include <AMDTApplicationComponents/Include/acFunctions.h>
#include <AMDTOSWrappers/Include/osFilePath.h>

// Local:
#include <CCVGCodeCoveragePlugin/Include/CCVG_cmd_CmdFilesReadDrivers.h>
#include <CCVGCodeCoveragePlugin/Include/CCVGResourcesString.h>

namespace ccvg {
namespace cmd {

/// @brief  Retrieve a new instance of *this class.
/// @return CCVGCmdBase* Pointer to new object, NULL on allocation failure.
CCVGCmdBase* CCVGCmdFilesReadDrivers::CreateSelf()
{
  return new (std::nothrow) CCVGCmdFilesReadDrivers();
}

/// @brief Class constructor. Initialise *this command's mandatory attributes.
CCVGCmdFilesReadDrivers::CCVGCmdFilesReadDrivers()
: m_paramFileDirDrivers(kParamID_FileDirDrivers, false, L"string", L"Driver", L"The folder directory path")
, m_paramFileExtnDriver(kParamID_FileExtnDriver, false, L"string", L"Driver file extension", L"Filter files by extension")
, m_paramFlagReadDriverFiles(kParamID_FlagReadDriverFiles, true, L"bool", L"Drivers flag", L"Read folder yes no")
{
  m_cmdID = CmdID::kFilesReadDrivers;
  m_cmdName = L"CmdFilesReadDrivers";
  m_pSelfCreatorFn = &CCVGCmdFilesReadDrivers::CreateSelf;

  m_listCmdParameters.push_back(&m_paramFileDirDrivers);
  m_listCmdParameters.push_back(&m_paramFileExtnDriver);
  m_listCmdParameters.push_back(&m_paramFlagReadDriverFiles);
}

/// @brief  Class destructor. Tidy up release resources used by *this instance.
CCVGCmdFilesReadDrivers::~CCVGCmdFilesReadDrivers()
{
  Shutdown();
}

/// @brief Clear the data gathered by this command.
void CCVGCmdFilesReadDrivers::ClearData()
{
  m_mapFileNames.clear();
}

/// @brief  Class initialise, setup resources or bindings.
/// @return status success = all ok, 
///                failure = error occurred see error description.
status CCVGCmdFilesReadDrivers::Initialise()
{
  m_refCount++;
  if (m_bBeenInitialised)
  {
    return success;
  }

  m_bBeenInitialised = true;

  return success;
}

/// @brief  Class shutdown, tear down resources or bindings.
/// @return status success = all ok, 
///                failure = error occurred see error description.
status CCVGCmdFilesReadDrivers::Shutdown()
{
  if (m_bBeenShutdown || (--m_refCount != 0))
  {
    return success;
  }

  ClearData();
  m_bBeenShutdown = true;

  return success;
}

/// @brief  Retrieve *this command's creation function.
/// @return CCVGCmdFactory::CmdCreatorFnPtr Function pointer.
CCVGCmdFactory::CmdCreatorFnPtr CCVGCmdFilesReadDrivers::GetCmdCreatorFn() const
{
  return m_pSelfCreatorFn;
}

/// @brief  The Execute() is the command work function to carry out work. Part 
///         of the command pattern.
/// @return status Succcess = task completed ok, failure = task failed.
status CCVGCmdFilesReadDrivers::Execute()
{
  m_bFinishedTask = false;
  ClearData();

  const bool bDoReadDrivers = *m_paramFlagReadDriverFiles.param.Get<bool>();
  if (!bDoReadDrivers)
  {
    m_bFinishedTask = true;
    return success;
  }
   
  const gtString &rFileDirPath = *m_paramFileDirDrivers.param.Get<gtString>();
  osFilePath folder(rFileDirPath);
  if (!folder.exists())
  {
    const wchar_t *pErrTxt = CCVGRSRC(kIDS_CCVG_STR_cmd_filesRead_err_dirNotExist);
    gtString errMsg;
    errMsg.appendFormattedString(pErrTxt, m_cmdName.asCharArray(), m_paramFileDirDrivers.pParamName, rFileDirPath.asCharArray());
    return ErrorSet(errMsg);
  }

  gtString fn;
  bool bError = false;
  const QString fileDir(acGTStringToQString(rFileDirPath));
  const QString fileFilter("*" + acGTStringToQString(*m_paramFileExtnDriver.param.Get<gtString>()));
  const gtString &rFileExtn();
  const QDir dirResults(fileDir, fileFilter, QDir::Name, QDir::Files | QDir::NoSymLinks);
  QDirIterator dirIt(dirResults);
  while (dirIt.hasNext())
  {
    dirIt.next();
    const QString fileName(dirIt.fileName());
    if (!fileName.isEmpty())
    {
      const gtString fn(acQStringToGTString(fileName));
      const osFilePath file(fn);
      gtString name;
      if (file.getFileName(name))
      {
        const MapPairFileNames_t pair(name, 0);
        m_mapFileNames.insert(pair);
      }
      else
      {
        bError = true;
        break;
      }
    }
  }
  
  if (bError)
  {
    const wchar_t *pErrTxt = CCVGRSRC(kIDS_CCVG_STR_cmd_filesReadCcvgd_err_fileNameInValid);
    gtString errMsg;
    errMsg.appendFormattedString(pErrTxt, m_cmdName.asCharArray(), fn.asCharArray());
    return ErrorSet(errMsg);
  }

  m_bFinishedTask = true;
  return success;
}

/// @brief  The opposite function to Execute(). Part of the command pattern.
/// @return status Succcess = task completed ok, failure = task failed.
status CCVGCmdFilesReadDrivers::Undo()
{
  m_bFinishedTask = false;
  // Do work
  
  if (!GetIsUndoableFlag())
  {
    m_bFinishedTask = true;
    return ErrorSet(CCVGRSRC(kIDS_CCVG_STR_pkg_cmd_err_called_undo_when_not_undoable));
  }

  m_bFinishedTask = true;
  return success;
}

/// @brief  Ask the command if it should be undoable, it is able to reverse
///         the task carried out in its Execute() function. It may be the has
///         indicate is cannot undo a previous action because it for example
///         performing a GUI operation i.e. open a dialog.
/// @return bool True = can undo, false = cannot undo.
bool CCVGCmdFilesReadDrivers::GetIsUndoableFlag() const
{
  return true;
}

/// @brief Retrieve the list of CCVG result file names.
/// @return MapFileNames_t& Container of file names.
const CCVGCmdFilesReadDrivers::MapFileNames_t& CCVGCmdFilesReadDrivers::GetFilesNames() const
{
  return m_mapFileNames;
}

} // namespace ccvg 
} // namespace cmd 