// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef COLUMN_ATTR_H_189467891346732143213
#define COLUMN_ATTR_H_189467891346732143213

#include <vector>


namespace zen
{
enum class ColumnTypeRim
{
    ITEM_PATH,
    SIZE,
    DATE,
    EXTENSION,
};

struct ColumnAttributeRim
{
    ColumnAttributeRim() {}
    ColumnAttributeRim(ColumnTypeRim type, int offset, int stretch, bool visible) : type_(type), offset_(offset), stretch_(stretch), visible_(visible) {}

    ColumnTypeRim type_    = ColumnTypeRim::ITEM_PATH;
    int           offset_  = 0;
    int           stretch_ = 0;;
    bool          visible_ = false;
};

inline
std::vector<ColumnAttributeRim> getDefaultColumnAttributesLeft()
{
    return //harmonize with main_dlg.cpp::onGridLabelContextRim() => expects stretched ITEM_PATH and non-stretched other columns!
    {
        { ColumnTypeRim::ITEM_PATH, -100, 1, true  },
        { ColumnTypeRim::EXTENSION,   60, 0, false },
        { ColumnTypeRim::DATE,       140, 0, false },
        { ColumnTypeRim::SIZE,       100, 0, true  },
    };
}

inline
std::vector<ColumnAttributeRim> getDefaultColumnAttributesRight()
{
    return getDefaultColumnAttributesLeft(); //*currently* same default
}

enum class ItemPathFormat
{
    FULL_PATH,
    RELATIVE_PATH,
    ITEM_NAME,
};

const ItemPathFormat defaultItemPathFormatLeftGrid  = ItemPathFormat::RELATIVE_PATH;
const ItemPathFormat defaultItemPathFormatRightGrid = ItemPathFormat::RELATIVE_PATH;

//------------------------------------------------------------------

enum class ColumnTypeCenter
{
    CHECKBOX,
    CMP_CATEGORY,
    SYNC_ACTION,
};

//------------------------------------------------------------------

enum class ColumnTypeNavi
{
    FOLDER_NAME,
    ITEM_COUNT,
    BYTES,
};

struct ColumnAttributeNavi
{
    ColumnAttributeNavi() {}
    ColumnAttributeNavi(ColumnTypeNavi type, int offset, int stretch, bool visible) : type_(type), offset_(offset), stretch_(stretch), visible_(visible) {}

    ColumnTypeNavi type_    = ColumnTypeNavi::FOLDER_NAME;
    int            offset_  = 0;
    int            stretch_ = 0;;
    bool           visible_ = false;
};


inline
std::vector<ColumnAttributeNavi> getDefaultColumnAttributesNavi()
{
    return //harmonize with tree_view.cpp::onGridLabelContext() => expects stretched FOLDER_NAME and non-stretched other columns!
    {
        { ColumnTypeNavi::FOLDER_NAME, -120, 1, true }, //stretch to full width and substract sum of fixed size widths
        { ColumnTypeNavi::ITEM_COUNT,    60, 0, true },
        { ColumnTypeNavi::BYTES,         60, 0, true }, //GTK needs a few pixels more width
    };
}

const           bool naviGridShowPercentageDefault    = true;
const ColumnTypeNavi naviGridLastSortColumnDefault    = ColumnTypeNavi::BYTES; //remember sort on navigation panel
const           bool naviGridLastSortAscendingDefault = false;                 //
}

#endif //COLUMN_ATTR_H_189467891346732143213
