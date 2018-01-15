// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef MONITOR_H_345087425834253425
#define MONITOR_H_345087425834253425

#include <functional>
#include <zen/zstring.h>


namespace rts
{
const int UI_UPDATE_INTERVAL_MS = 100; //unit: [ms]; perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss


struct MonitorCallback
{
    virtual ~MonitorCallback() {}

    enum WatchPhase
    {
        MONITOR_PHASE_ACTIVE,
        MONITOR_PHASE_WAITING,
    };
    virtual void setPhase(WatchPhase mode) = 0;
    virtual void executeExternalCommand () = 0;
    virtual void requestUiRefresh       () = 0;
    virtual void reportError(const std::wstring& msg) = 0; //automatically retries after return!
};
void monitorDirectories(const std::vector<Zstring>& folderPathPhrases,
                        //non-formatted dirnames that yet require call to getFormattedDirectoryName(); empty directories must be checked by caller!
                        unsigned int delay,
                        MonitorCallback& callback);
}

#endif //MONITOR_H_345087425834253425
