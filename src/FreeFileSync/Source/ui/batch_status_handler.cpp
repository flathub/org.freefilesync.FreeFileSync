// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#include "batch_status_handler.h"
#include <zen/shell_execute.h>
#include <zen/thread.h>
#include <zen/shutdown.h>
#include <wx+/popup_dlg.h>
#include <wx/app.h>
#include "../lib/ffs_paths.h"
#include "../lib/resolve_path.h"
#include "../lib/status_handler_impl.h"
#include "../lib/generate_logfile.h"
#include "../fs/concrete.h"

using namespace zen;
using namespace xmlAccess;


namespace
{
//"Backup FreeFileSync 2013-09-15 015052.123.log" ->
//"Backup FreeFileSync 2013-09-15 015052.123 [Error].log"

//return value always bound!
std::unique_ptr<AFS::OutputStream> prepareNewLogfile(const AbstractPath& logFolderPath, //throw FileError
                                                     const std::wstring& jobName,
                                                     const std::chrono::system_clock::time_point& batchStartTime,
                                                     const std::wstring& failStatus,
                                                     ProcessCallback& pc)
{
    assert(!jobName.empty());

    //create logfile folder if required
    AFS::createFolderIfMissingRecursion(logFolderPath); //throw FileError

    //const std::string colon = "\xcb\xb8"; //="modifier letter raised colon" => regular colon is forbidden in file names on Windows and OS X
    //=> too many issues, most notably cmd.exe is not Unicode-aware: https://www.freefilesync.org/forum/viewtopic.php?t=1679

    //assemble logfile name
    const TimeComp timeStamp = getLocalTime(std::chrono::system_clock::to_time_t(batchStartTime));
    const auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(batchStartTime.time_since_epoch()).count() % 1000;

    Zstring logFileName = utfTo<Zstring>(jobName) +
                          Zstr(" ") + formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"), timeStamp) +
                          Zstr(".") + printNumber<Zstring>(Zstr("%03d"), static_cast<int>(timeMs)); //[ms] should yield a fairly unique name
    if (!failStatus.empty())
        logFileName += utfTo<Zstring>(L" [" + failStatus + L"]");
    logFileName += Zstr(".log");

    const AbstractPath logFilePath = AFS::appendRelPath(logFolderPath, logFileName);

    return AFS::getOutputStream(logFilePath, //throw FileError
                                nullptr, /*streamSize*/
                                OnUpdateLogfileStatusNoThrow(pc, AFS::getDisplayPath(logFilePath)));
}


struct LogTraverserCallback: public AFS::TraverserCallback
{
    LogTraverserCallback(const Zstring& prefix, const std::function<void()>& onUpdateStatus) :
        prefix_(prefix),
        onUpdateStatus_(onUpdateStatus) {}

    void onFile(const FileInfo& fi) override
    {
        if (startsWith(fi.itemName, prefix_, CmpFilePath() /*even on Linux!*/) && endsWith(fi.itemName, Zstr(".log"), CmpFilePath()))
            logFileNames_.push_back(fi.itemName);

        if (onUpdateStatus_) onUpdateStatus_();
    }
    std::unique_ptr<TraverserCallback> onFolder (const FolderInfo&  fi) override { return nullptr; }
    HandleLink                         onSymlink(const SymlinkInfo& si) override { return TraverserCallback::LINK_SKIP; }

    HandleError reportDirError (const std::wstring& msg, size_t retryNumber                         ) override { setError(msg); return ON_ERROR_CONTINUE; }
    HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zstring& itemName) override { setError(msg); return ON_ERROR_CONTINUE; }

    const std::vector<Zstring>& refFileNames() const { return logFileNames_; }
    const Opt<FileError>& getLastError() const { return lastError_; }

private:
    void setError(const std::wstring& msg) //implement late failure
    {
        if (!lastError_)
            lastError_ = FileError(msg);
    }

    const Zstring prefix_;
    const std::function<void()> onUpdateStatus_;
    std::vector<Zstring> logFileNames_; //out
    Opt<FileError> lastError_;
};


void limitLogfileCount(const AbstractPath& logFolderPath, const std::wstring& jobname, size_t maxCount, const std::function<void()>& onUpdateStatus) //throw FileError
{
    const Zstring prefix = utfTo<Zstring>(jobname);

    LogTraverserCallback lt(prefix, onUpdateStatus); //traverse source directory one level deep
    AFS::traverseFolder(logFolderPath, lt);

    std::vector<Zstring> logFileNames = lt.refFileNames();
    Opt<FileError> lastError = lt.getLastError();

    if (logFileNames.size() > maxCount)
    {
        //delete oldest logfiles: take advantage of logfile naming convention to find them
        std::nth_element(logFileNames.begin(), logFileNames.end() - maxCount, logFileNames.end(), LessFilePath());

        std::for_each(logFileNames.begin(), logFileNames.end() - maxCount, [&](const Zstring& logFileName)
        {
            try
            {
                AFS::removeFilePlain(AFS::appendRelPath(logFolderPath, logFileName)); //throw FileError
            }
            catch (const FileError& e) { if (!lastError) lastError = e; };

            if (onUpdateStatus) onUpdateStatus();
        });
    }

    if (lastError) //late failure!
        throw* lastError;
}
}

//##############################################################################################################################

BatchStatusHandler::BatchStatusHandler(bool showProgress,
                                       const std::wstring& jobName,
                                       const Zstring& soundFileSyncComplete,
                                       const std::chrono::system_clock::time_point& batchStartTime,
                                       const Zstring& logFolderPathPhrase, //may be empty
                                       int logfilesCountLimit,
                                       size_t lastSyncsLogFileSizeMax,
                                       bool ignoreErrors,
                                       BatchErrorDialog batchErrorDialog,
                                       size_t automaticRetryCount,
                                       size_t automaticRetryDelay,
                                       FfsReturnCode& returnCode,
                                       const Zstring& postSyncCommand,
                                       PostSyncCondition postSyncCondition,
                                       PostSyncAction postSyncAction) :
    logfilesCountLimit_(logfilesCountLimit),
    lastSyncsLogFileSizeMax_(lastSyncsLogFileSizeMax),
    batchErrorDialog_(batchErrorDialog),
    returnCode_(returnCode),
    automaticRetryCount_(automaticRetryCount),
    automaticRetryDelay_(automaticRetryDelay),
    progressDlg_(createProgressDialog(*this, [this] { this->onProgressDialogTerminate(); },
*this,
nullptr, //parentWindow
showProgress,
jobName,
soundFileSyncComplete,
ignoreErrors,
postSyncAction)),
               jobName_(jobName),
               batchStartTime_(batchStartTime),
               logFolderPathPhrase_(logFolderPathPhrase),
               postSyncCommand_(postSyncCommand),
               postSyncCondition_(postSyncCondition)
{
    //ATTENTION: "progressDlg_" is an unmanaged resource!!! However, at this point we already consider construction complete! =>
    //ZEN_ON_SCOPE_FAIL( cleanup(); ); //destructor call would lead to member double clean-up!!!

    //...

    //if (logFile)
    //  ::wxSetEnv(L"logfile", utfTo<wxString>(logFile->getFilename()));
}


BatchStatusHandler::~BatchStatusHandler()
{
    const int totalErrors   = errorLog_.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log
    const int totalWarnings = errorLog_.getItemCount(TYPE_WARNING);

    //finalize error log
    SyncProgressDialog::SyncResult finalStatus = SyncProgressDialog::RESULT_FINISHED_WITH_SUCCESS;
    std::wstring finalStatusMsg;
    std::wstring failStatus; //additionally indicate errors in log file name
    if (getAbortStatus())
    {
        finalStatus = SyncProgressDialog::RESULT_ABORTED;
        raiseReturnCode(returnCode_, FFS_RC_ABORTED);
        finalStatusMsg = _("Synchronization stopped");
        errorLog_.logMsg(finalStatusMsg, TYPE_ERROR);
        failStatus = _("Stopped");
    }
    else if (totalErrors > 0)
    {
        finalStatus = SyncProgressDialog::RESULT_FINISHED_WITH_ERROR;
        raiseReturnCode(returnCode_, FFS_RC_FINISHED_WITH_ERRORS);
        finalStatusMsg = _("Synchronization completed with errors");
        errorLog_.logMsg(finalStatusMsg, TYPE_ERROR);
        failStatus = _("Error");
    }
    else if (totalWarnings > 0)
    {
        finalStatus = SyncProgressDialog::RESULT_FINISHED_WITH_WARNINGS;
        raiseReturnCode(returnCode_, FFS_RC_FINISHED_WITH_WARNINGS);
        finalStatusMsg = _("Synchronization completed with warnings");
        errorLog_.logMsg(finalStatusMsg, TYPE_WARNING);
        failStatus = _("Warning");
    }
    else
    {
        if (getItemsTotal(PHASE_SYNCHRONIZING) == 0 && //we're past "initNewPhase(PHASE_SYNCHRONIZING)" at this point!
            getBytesTotal(PHASE_SYNCHRONIZING) == 0)
            finalStatusMsg = _("Nothing to synchronize"); //even if "ignored conflicts" occurred!
        else
            finalStatusMsg = _("Synchronization completed successfully");
        errorLog_.logMsg(finalStatusMsg, TYPE_INFO);
    }

    //post sync command
    Zstring commandLine = [&]
    {
        if (!getAbortStatus() || *getAbortStatus() != AbortTrigger::USER) //user cancelled => don't run post sync command!
            switch (postSyncCondition_)
            {
                case PostSyncCondition::COMPLETION:
                    return postSyncCommand_;
                case PostSyncCondition::ERRORS:
                    if (finalStatus == SyncProgressDialog::RESULT_ABORTED ||
                        finalStatus == SyncProgressDialog::RESULT_FINISHED_WITH_ERROR)
                        return postSyncCommand_;
                    break;
                case PostSyncCondition::SUCCESS:
                    if (finalStatus == SyncProgressDialog::RESULT_FINISHED_WITH_WARNINGS ||
                        finalStatus == SyncProgressDialog::RESULT_FINISHED_WITH_SUCCESS)
                        return postSyncCommand_;
                    break;
            }
        return Zstring();
    }();
    trim(commandLine);

    if (!commandLine.empty())
        errorLog_.logMsg(replaceCpy(_("Executing command %x"), L"%x", fmtPath(commandLine)), TYPE_INFO);

    //----------------- write results into user-specified logfile ------------------------
    const SummaryInfo summary =
    {
        jobName_,
        finalStatusMsg,
        getItemsCurrent(PHASE_SYNCHRONIZING), getBytesCurrent(PHASE_SYNCHRONIZING),
        getItemsTotal  (PHASE_SYNCHRONIZING), getBytesTotal  (PHASE_SYNCHRONIZING),
        std::time(nullptr) - startTime_
    };

    //create not before destruction: 1. avoid issues with FFS trying to sync open log file 2. simplify transactional retry on failure 3. no need to rename log file to include status
    // 4. failure to write to particular stream must not be retried!
    if (logfilesCountLimit_ != 0)
    {
        auto requestUiRefreshNoThrow = [&] { try { requestUiRefresh(); /*throw X*/ } catch (...) {} };

        const AbstractPath logFolderPath = createAbstractPath(trimCpy(logFolderPathPhrase_).empty() ? getConfigDirPathPf() + Zstr("Logs") : logFolderPathPhrase_); //noexcept

        try
        {
            tryReportingError([&] //errors logged here do not impact final status calculation above! => not a problem!
            {
                std::unique_ptr<AFS::OutputStream> logFileStream = prepareNewLogfile(logFolderPath, jobName_, batchStartTime_, failStatus, *this); //throw FileError; return value always bound!

                streamToLogFile(summary, errorLog_, *logFileStream); //throw FileError, (X)
                logFileStream->finalize();                           //throw FileError, (X)
            }, *this); //throw X! by ProcessCallback!
        }
        catch (...) {}

        if (logfilesCountLimit_ > 0)
        {
            try { reportStatus(_("Cleaning up old log files...")); }
            catch (...) {}

            try
            {
                tryReportingError([&]
                {
                    limitLogfileCount(logFolderPath, jobName_, logfilesCountLimit_, requestUiRefreshNoThrow); //throw FileError
                }, *this); //throw X
            }
            catch (...) {}
        }
    }
    //write results into LastSyncs.log
    try
    {
        saveToLastSyncsLog(summary, errorLog_, lastSyncsLogFileSizeMax_, OnUpdateLogfileStatusNoThrow(*this, utfTo<std::wstring>(getLastSyncsLogfilePath()))); //throw FileError
    }
    catch (FileError&) { assert(false); }

    //execute post sync command *after* writing log files, so that user can refer to the log via the command!
    if (!commandLine.empty())
        try
        {
            //use EXEC_TYPE_ASYNC until there is reason not to: https://www.freefilesync.org/forum/viewtopic.php?t=31
            tryReportingError([&] { shellExecute(expandMacros(commandLine), EXEC_TYPE_ASYNC); /*throw FileError*/ }, *this); //throw X
        }
        catch (...) {}

    if (progressDlg_)
    {
        //post sync action
        bool showSummary = true;
        bool triggerSleep = false;
        if (!getAbortStatus() || *getAbortStatus() != AbortTrigger::USER) //user cancelled => don't run post sync action!
            switch (progressDlg_->getOptionPostSyncAction())
            {
                case PostSyncAction::SUMMARY:
                    break;
                case PostSyncAction::EXIT:
                    showSummary = false;
                    break;
                case PostSyncAction::SLEEP:
                    triggerSleep = true;
                    break;
                case PostSyncAction::SHUTDOWN:
                    showSummary = false;
                    try
                    {
                        tryReportingError([&] { shutdownSystem(); /*throw FileError*/ }, *this); //throw X
                    }
                    catch (...) {}
                    break;
            }
        if (switchToGuiRequested_) //-> avoid recursive yield() calls, thous switch not before ending batch mode
            showSummary = false;

        //close progress dialog
        if (showSummary) //warning: wxWindow::Show() is called within showSummary()!
            //notify about (logical) application main window => program won't quit, but stay on this dialog
            //setMainWindow(progressDlg_->getAsWindow()); -> not required anymore since we block waiting until dialog is closed below
            progressDlg_->showSummary(finalStatus, errorLog_);
        else
            progressDlg_->closeDirectly(true /*restoreParentFrame: n/a here*/); //progressDlg_ is main window => program will quit shortly after

        if (triggerSleep) //sleep *after* showing results dialog (consider total time!)
            try
            {
                tryReportingError([&] { suspendSystem(); /*throw FileError*/ }, *this); //throw X
            }
            catch (...) {}

        //wait until progress dialog notified shutdown via onProgressDialogTerminate()
        //-> required since it has our "this" pointer captured in lambda "notifyWindowTerminate"!
        //-> nicely manages dialog lifetime
        while (progressDlg_)
        {
            wxTheApp->Yield(); //*first* refresh GUI (removing flicker) before sleeping!
            std::this_thread::sleep_for(std::chrono::milliseconds(UI_UPDATE_INTERVAL_MS));
        }
    }
}


void BatchStatusHandler::initNewPhase(int itemsTotal, int64_t bytesTotal, ProcessCallback::Phase phaseID)
{
    StatusHandler::initNewPhase(itemsTotal, bytesTotal, phaseID);
    if (progressDlg_)
        progressDlg_->initNewPhase(); //call after "StatusHandler::initNewPhase"

    forceUiRefresh(); //throw ?; OS X needs a full yield to update GUI and get rid of "dummy" texts
}


void BatchStatusHandler::updateProcessedData(int itemsDelta, int64_t bytesDelta)
{
    StatusHandler::updateProcessedData(itemsDelta, bytesDelta);

    if (progressDlg_)
        progressDlg_->notifyProgressChange(); //noexcept
    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void BatchStatusHandler::reportInfo(const std::wstring& text)
{
    errorLog_.logMsg(text, TYPE_INFO); //log first!
    StatusHandler::reportInfo(text); //throw X
}


void BatchStatusHandler::reportWarning(const std::wstring& warningMessage, bool& warningActive)
{
    if (!progressDlg_) abortProcessNow();

    errorLog_.logMsg(warningMessage, TYPE_WARNING);

    if (!warningActive)
        return;

    if (!progressDlg_->getOptionIgnoreErrors())
        switch (batchErrorDialog_)
        {
            case BatchErrorDialog::SHOW:
            {
                PauseTimers dummy(*progressDlg_);
                forceUiRefresh();

                bool dontWarnAgain = false;
                switch (showQuestionDialog(progressDlg_->getWindowIfVisible(), DialogInfoType::WARNING, PopupDialogCfg().
                                           setDetailInstructions(warningMessage + L"\n\n" + _("You can switch to FreeFileSync's main window to resolve this issue.")).
                                           setCheckBox(dontWarnAgain, _("&Don't show this warning again"), QuestionButton2::NO),
                                           _("&Ignore"), _("&Switch")))
                {
                    case QuestionButton2::YES: //ignore
                        warningActive = !dontWarnAgain;
                        break;

                    case QuestionButton2::NO: //switch
                        errorLog_.logMsg(_("Switching to FreeFileSync's main window"), TYPE_INFO);
                        switchToGuiRequested_ = true; //treat as a special kind of cancel
                        userRequestAbort();           //
                        throw BatchRequestSwitchToMainDialog();

                    case QuestionButton2::CANCEL:
                        userAbortProcessNow(); //throw AbortProcess
                        break;
                }
            }
            break; //keep it! last switch might not find match

            case BatchErrorDialog::CANCEL:
                abortProcessNow(); //not user-initiated! throw AbortProcess
                break;
        }
}


ProcessCallback::Response BatchStatusHandler::reportError(const std::wstring& errorMessage, size_t retryNumber)
{
    if (!progressDlg_) abortProcessNow();

    //auto-retry
    if (retryNumber < automaticRetryCount_)
    {
        errorLog_.logMsg(errorMessage + L"\n-> " +
                         _P("Automatic retry in 1 second...", "Automatic retry in %x seconds...", automaticRetryDelay_), TYPE_INFO);
        //delay
        const int iterations = static_cast<int>(1000 * automaticRetryDelay_ / UI_UPDATE_INTERVAL_MS); //always round down: don't allow for negative remaining time below
        for (int i = 0; i < iterations; ++i)
        {
            reportStatus(_("Error") + L": " + _P("Automatic retry in 1 second...", "Automatic retry in %x seconds...",
                                                 (1000 * automaticRetryDelay_ - i * UI_UPDATE_INTERVAL_MS + 999) / 1000)); //integer round up
            std::this_thread::sleep_for(std::chrono::milliseconds(UI_UPDATE_INTERVAL_MS));
        }
        return ProcessCallback::RETRY;
    }


    //always, except for "retry":
    auto guardWriteLog = zen::makeGuard<ScopeGuardRunMode::ON_EXIT>([&] { errorLog_.logMsg(errorMessage, TYPE_ERROR); });


    if (!progressDlg_->getOptionIgnoreErrors())
    {
        switch (batchErrorDialog_)
        {
            case BatchErrorDialog::SHOW:
            {
                PauseTimers dummy(*progressDlg_);
                forceUiRefresh();

                switch (showConfirmationDialog(progressDlg_->getWindowIfVisible(), DialogInfoType::ERROR2, PopupDialogCfg().
                                               setDetailInstructions(errorMessage),
                                               _("&Ignore"), _("Ignore &all"), _("&Retry")))
                {
                    case ConfirmationButton3::ACCEPT: //ignore
                        return ProcessCallback::IGNORE_ERROR;

                    case ConfirmationButton3::ACCEPT_ALL: //ignore all
                        progressDlg_->setOptionIgnoreErrors(true);
                        return ProcessCallback::IGNORE_ERROR;

                    case ConfirmationButton3::DECLINE: //retry
                        guardWriteLog.dismiss();
                        errorLog_.logMsg(errorMessage + L"\n-> " + _("Retrying operation..."), TYPE_INFO);
                        return ProcessCallback::RETRY;

                    case ConfirmationButton3::CANCEL:
                        userAbortProcessNow(); //throw AbortProcess
                        break;
                }
            }
            break; //used if last switch didn't find a match

            case BatchErrorDialog::CANCEL:
                abortProcessNow(); //not user-initiated! throw AbortProcess
                break;
        }
    }
    else
        return ProcessCallback::IGNORE_ERROR;

    assert(false);
    return ProcessCallback::IGNORE_ERROR; //dummy value
}


void BatchStatusHandler::reportFatalError(const std::wstring& errorMessage)
{
    if (!progressDlg_) abortProcessNow();

    errorLog_.logMsg(errorMessage, TYPE_FATAL_ERROR);

    if (!progressDlg_->getOptionIgnoreErrors())
        switch (batchErrorDialog_)
        {
            case BatchErrorDialog::SHOW:
            {
                PauseTimers dummy(*progressDlg_);
                forceUiRefresh();

                switch (showConfirmationDialog(progressDlg_->getWindowIfVisible(), DialogInfoType::ERROR2,
                                               PopupDialogCfg().setTitle(_("Serious Error")).
                                               setDetailInstructions(errorMessage),
                                               _("&Ignore"), _("Ignore &all")))
                {
                    case ConfirmationButton2::ACCEPT:
                        break;

                    case ConfirmationButton2::ACCEPT_ALL:
                        progressDlg_->setOptionIgnoreErrors(true);
                        break;

                    case ConfirmationButton2::CANCEL:
                        userAbortProcessNow(); //throw AbortProcess
                        break;
                }
            }
            break;

            case BatchErrorDialog::CANCEL:
                abortProcessNow(); //not user-initiated! throw AbortProcess
                break;
        }
}


void BatchStatusHandler::forceUiRefresh()
{
    if (progressDlg_)
        progressDlg_->updateGui();
}


void BatchStatusHandler::onProgressDialogTerminate()
{
    progressDlg_ = nullptr;
}
