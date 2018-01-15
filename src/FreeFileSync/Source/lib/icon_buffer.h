// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef ICON_BUFFER_H_8425703245726394256
#define ICON_BUFFER_H_8425703245726394256

#include <vector>
#include <memory>
#include <zen/zstring.h>
#include <zen/optional.h>
#include <wx/bitmap.h>
#include "../fs/abstract.h"


namespace zen
{
class IconBuffer
{
public:
    enum IconSize
    {
        SIZE_SMALL,
        SIZE_MEDIUM,
        SIZE_LARGE
    };

    IconBuffer(IconSize sz);
    ~IconBuffer();

    static int getSize(IconSize sz); //expected and *maximum* icon size in pixel
    int getSize() const { return getSize(iconSizeType); } //

    bool          readyForRetrieval(const AbstractPath& filePath);
    Opt<wxBitmap> retrieveFileIcon (const AbstractPath& filePath); //... and mark as hot
    void          setWorkload      (const std::vector<AbstractPath>& load); //(re-)set new workload of icons to be retrieved;

    wxBitmap getIconByExtension(const Zstring& filePath); //...and add to buffer

    static wxBitmap genericFileIcon(IconSize sz);
    static wxBitmap genericDirIcon (IconSize sz);
    static wxBitmap linkOverlayIcon(IconSize sz);

private:
    struct Impl;
    const std::unique_ptr<Impl> pimpl_;

    const IconSize iconSizeType;
};

bool hasLinkExtension(const Zstring& filepath);
}

#endif //ICON_BUFFER_H_8425703245726394256
