// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef TREE_VIEW_H_841703190201835280256673425
#define TREE_VIEW_H_841703190201835280256673425

#include <functional>
#include <zen/optional.h>
#include <wx+/grid.h>
#include "column_attr.h"
#include "../file_hierarchy.h"


namespace zen
{
//tree view of FolderComparison
class TreeView
{
public:
    TreeView() {}

    void setData(FolderComparison& newData); //set data, taking (partial) ownership

    //apply view filter: comparison results
    void updateCmpResult(bool showExcluded,
                         bool leftOnlyFilesActive,
                         bool rightOnlyFilesActive,
                         bool leftNewerFilesActive,
                         bool rightNewerFilesActive,
                         bool differentFilesActive,
                         bool equalFilesActive,
                         bool conflictFilesActive);

    //apply view filter: synchronization preview
    void updateSyncPreview(bool showExcluded,
                           bool syncCreateLeftActive,
                           bool syncCreateRightActive,
                           bool syncDeleteLeftActive,
                           bool syncDeleteRightActive,
                           bool syncDirOverwLeftActive,
                           bool syncDirOverwRightActive,
                           bool syncDirNoneActive,
                           bool syncEqualActive,
                           bool conflictFilesActive);

    enum NodeStatus
    {
        STATUS_EXPANDED,
        STATUS_REDUCED,
        STATUS_EMPTY
    };

    //---------------------------------------------------------------------
    struct Node
    {
        Node(int percent, uint64_t bytes, int itemCount, unsigned int level, NodeStatus status) :
            percent_(percent), level_(level), status_(status), bytes_(bytes), itemCount_(itemCount) {}
        virtual ~Node() {}

        const int percent_; //[0, 100]
        const unsigned int level_;
        const NodeStatus status_;
        const uint64_t bytes_;
        const int itemCount_;
    };

    struct FilesNode : public Node
    {
        FilesNode(int percent, uint64_t bytes, int itemCount, unsigned int level, const std::vector<FileSystemObject*>& filesAndLinks) :
            Node(percent, bytes, itemCount, level, STATUS_EMPTY), filesAndLinks_(filesAndLinks)  {}

        std::vector<FileSystemObject*> filesAndLinks_; //files and symlinks matching view filter; pointers are bound!
    };

    struct DirNode : public Node
    {
        DirNode(int percent, uint64_t bytes, int itemCount, unsigned int level, NodeStatus status, FolderPair& folder) : Node(percent, bytes, itemCount, level, status), folder_(folder) {}
        FolderPair& folder_;
    };

    struct RootNode : public Node
    {
        RootNode(int percent, uint64_t bytes, int itemCount, NodeStatus status, BaseFolderPair& baseFolder, const Zstring& displayName) :
            Node(percent, bytes, itemCount, 0, status), baseFolder_(baseFolder), displayName_(displayName) {}

        BaseFolderPair& baseFolder_;
        const Zstring displayName_;
    };

    std::unique_ptr<Node> getLine(size_t row) const; //return nullptr on error
    size_t linesTotal() const { return flatTree.size(); }

    void expandNode(size_t row);
    void reduceNode(size_t row);
    NodeStatus getStatus(size_t row) const;
    ptrdiff_t getParent(size_t row) const; //return < 0 if none

    void setSortDirection(ColumnTypeNavi colType, bool ascending); //apply permanently!
    std::pair<ColumnTypeNavi, bool> getSortDirection() { return std::make_pair(sortColumn, sortAscending); }
    static bool getDefaultSortDirection(ColumnTypeNavi colType); //ascending?

private:
    struct DirNodeImpl;

    struct Container
    {
        uint64_t bytesGross = 0;
        uint64_t bytesNet   = 0; //bytes for files on view in this directory only
        int itemCountGross  = 0;
        int itemCountNet    = 0; //number of files on view for in this directory only

        std::vector<DirNodeImpl> subDirs;
        FileSystemObject::ObjectId firstFileId = nullptr; //weak pointer to first FilePair or SymlinkPair
        //- "compress" algorithm may hide file nodes for directories with a single included file, i.e. itemCountGross == itemCountNet == 1
        //- a ContainerObject* would be a better fit, but we need weak pointer semantics!
        //- a std::vector<FileSystemObject::ObjectId> would be a better design, but we don't want a second memory structure as large as custom grid!
    };

    struct DirNodeImpl : public Container
    {
        FileSystemObject::ObjectId objId = nullptr; //weak pointer to FolderPair
    };

    struct RootNodeImpl : public Container
    {
        std::shared_ptr<BaseFolderPair> baseFolder;
        Zstring displayName;
    };

    enum NodeType
    {
        TYPE_ROOT,      //-> RootNodeImpl
        TYPE_DIRECTORY, //-> DirNodeImpl
        TYPE_FILES      //-> Container
    };

    struct TreeLine
    {
        TreeLine(unsigned int level, int percent, const Container* node, enum NodeType type) : level_(level), percent_(percent), node_(node), type_(type) {}

        unsigned int level_;
        int percent_; //[0, 100]
        const Container* node_; //
        NodeType type_;         //we increase size of "flatTree" using C-style types rather than have a polymorphic "folderCmpView"
    };

    static void compressNode(Container& cont);
    template <class Function>
    static void extractVisibleSubtree(ContainerObject& hierObj, Container& cont, Function includeObject);
    void getChildren(const Container& cont, unsigned int level, std::vector<TreeLine>& output);
    template <class Predicate> void updateView(Predicate pred);
    void applySubView(std::vector<RootNodeImpl>&& newView);

    template <bool ascending> static void sortSingleLevel(std::vector<TreeLine>& items, ColumnTypeNavi columnType);
    template <bool ascending> struct LessShortName;

    std::vector<TreeLine> flatTree; //collapsable/expandable sub-tree of folderCmpView -> always sorted!
    /*             /|\
                    | (update...)
                    |                         */
    std::vector<RootNodeImpl> folderCmpView; //partial view on folderCmp -> unsorted (cannot be, because files are not a separate entity)
    std::function<bool(const FileSystemObject& fsObj)> lastViewFilterPred; //buffer view filter predicate for lazy evaluation of files/symlinks corresponding to a TYPE_FILES node
    /*             /|\
                    | (update...)
                    |                         */
    std::vector<std::shared_ptr<BaseFolderPair>> folderCmp; //full raw data

    ColumnTypeNavi sortColumn = naviGridLastSortColumnDefault;
    bool sortAscending        = naviGridLastSortAscendingDefault;
};


Zstring getShortDisplayNameForFolderPair(const AbstractPath& itemPathL, const AbstractPath& itemPathR);


namespace treeview
{
void init(Grid& grid, const std::shared_ptr<TreeView>& treeDataView);

void setShowPercentage(Grid& grid, bool value);
bool getShowPercentage(const Grid& grid);

std::vector<Grid::ColumnAttribute> convertConfig(const std::vector<ColumnAttributeNavi  >& attribs); //+ make consistent
std::vector<ColumnAttributeNavi>   convertConfig(const std::vector<Grid::ColumnAttribute>& attribs); //
}
}

#endif //TREE_VIEW_H_841703190201835280256673425
