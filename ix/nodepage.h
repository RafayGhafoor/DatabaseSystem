#include "../types.h"

#ifndef CS222P_WINTER20_NODEPAGE_H
#define CS222P_WINTER20_NODEPAGE_H

class NodePage {

    /*
    * Basic page format (shared structure between KeyNode and LeafNode):
    * ┌────────────────────────────────────────────────────────────────────────────────┐
    * │ DATA SECTION                                                                   │
    * │                                                                                │
    * │    ┌───────────────────┬────────────────┬─────────────────┬────────────────────┤
    * │    │   SLOT DIRECTORY  │   FREE SPACE   │   SLOT NUMBER   │       IS LEAF      │
    * │    ├───────────────────┼────────────────┼─────────────────┼────────────────────┤
    * │    │      <slots>      │   free space   │   slot number   │  node type (bool)  │
    * └────┴───────────────────┴────────────────┴─────────────────┴────────────────────┘
    */

protected:
    PageFreeSpace freeSpace;
    SlotNumber slotNumber;
    bool isLeaf;
    void *pageData;

    // passed page data, will not be delete in destructor
    // init empty data
    explicit NodePage(void *pageData);
    NodePage(const NodePage&) = delete;                             // copy constructor, implement when needed
    NodePage(NodePage&&) = delete;                                  // move constructor, implement when needed
    NodePage& operator=(const NodePage&) = delete;                  // copy assignment, implement when needed
    NodePage& operator=(NodePage&&) = delete;                       // move assignment, implement when needed
    ~NodePage() = default;

    void moveData(PageOffset startOffset, PageOffset targetOffset, PageOffset length);

public:
    PageFreeSpace getFreeSpace();
    SlotNumber getSlotNumber();
    bool isLeafNode() { return isLeaf; }
    void *getPageData() { return pageData; }

    /* Delete keys start from keyIndex */
    // return 0 - success, other - fail
    void deleteKeysStartFrom(const KeyNumber &keyIndex);

    /* Get nth key data, n starts from 0 */
    // if key type is varchar, "length" should be passed as 0,
    // and will return a char pointer with "length" assigned with varchar's length
    // if key type is int/float, return a int/float pointer
    // error, return nullptr
    void* getNthKey(const KeyNumber &keyIndex, AttrLength &length);

    /* Copy data and slots */
    // suppose one page is:
    //  k0, k1, k2,.................
    //  ............kn,.............
    //  ......s0, s1, s2..., sn,....
    // after executing:
    //   "void* block = copyToEnd(1, dataLength, slotDataLength);"
    // block will points to a memory block: k1, k2, ....., kn, s1, s2, ...., sn
    //                                     |    dataLength + slotDataLength  |
    // dataLength = address of kn - address of k1
    // slotDataLength = address if sn - address of s1
    // keyNumbers = n - 1 + 1 = n
    //
    // for key node page: kn is actually kn and pointer after kn
    // for leaf node page: kn is actually kn and rids of kn
    //
    // if fail, return nullptr
    void* copyToEnd(const KeyNumber &startKey, PageOffset &dataLength, PageOffset &slotDataLength, KeyNumber &keyNumbers);

};


class KeyNodePage: public NodePage {

    /* page format: same as NodePage */

public:
    // passed page data, will not be delete in destructor
    // init empty data
    explicit KeyNodePage(void *pageData);

    // used in spliting, *block comes from "copyToEnd"
    KeyNodePage(void *pageData, const void* block, const KeyNumber &keyNumbers,
                const PageOffset &dataLength, const PageOffset &slotDataLength,
                const PageNum &prevKeyNodePageID);

    KeyNodePage(const KeyNodePage&) = delete;                             // copy constructor, implement when needed
    KeyNodePage(KeyNodePage&&) = delete;                                  // move constructor, implement when needed
    KeyNodePage& operator=(const KeyNodePage&) = delete;                  // copy assignment, implement when needed
    KeyNodePage& operator=(KeyNodePage&&) = delete;                       // move assignment, implement when needed
    ~KeyNodePage() = default;

    KeyNumber getKeyNumber() { return getSlotNumber(); };

    /* NOTE: All methods below will throw assert error when key indexes are not valid */

    void* getNthKey(const KeyNumber &keyIndex, AttrLength &length);
    void deleteKeysStartFrom(const KeyNumber &keyIndex);

    /* Add a key */
    // not assign pointers of the key, only add a key to the page
    // when key is varchar, "key" is char pointer, and "length" is varchar's length
    // return 0 - success, other- fail
    void addKey(const void *key, const AttrLength &length);

    /* Set pointers of a key */
    void setLeftPointer(const KeyNumber &keyIndex, const PageNum &pageID);
    void setRightPointer(const KeyNumber &keyIndex, const PageNum &pageID);

    /* Get pointers of a key */
    PageNum getLeftPointer(const KeyNumber &keyIndex);
    PageNum getRightPointer(const KeyNumber &keyIndex);

};


class LeafNodePage: public NodePage {

    /*
    * Leaf page format:
    * ┌────────────────────────────────────────────────────────────────────────────────────────────────────┐
    * │ <key, [rid, rid, rid]>, ...                                                                        │
    * │                                                                                                    │
    * │    ┌───────────────────┬───────────────────┬────────────────┬─────────────────┬────────────────────┤
    * │    │   SLOT DIRECTORY  │    NEXT POINTER   │   FREE SPACE   │   SLOT NUMBER   │       IS LEAF      │
    * │    ├───────────────────┼───────────────────┼────────────────┼─────────────────┼────────────────────┤
    * │    │      <slots>      │ next leaf page id │   free space   │   slot number   │  node type (bool)  │
    * └────┴───────────────────┴───────────────────┴────────────────┴─────────────────┴────────────────────┘
    */

    // default is 0 indicating no next leaf page
    PageNum nextLeafPage;

public:
    // passed page data, will not be delete in destructor
    // init empty data
    explicit LeafNodePage(void *pageData);

    // used in spliting, *block comes from "copyToEnd"
    LeafNodePage(void *pageData, const void* block, const KeyNumber &keyNumbers,
                 const PageOffset &dataLength, const PageOffset &slotDataLength);

    LeafNodePage(const LeafNodePage&) = delete;                        // copy constructor, implement when needed
    LeafNodePage(LeafNodePage&&) = delete;                             // move constructor, implement when needed
    LeafNodePage& operator=(const LeafNodePage&) = delete;             // copy assignment, implement when needed
    LeafNodePage& operator=(LeafNodePage&&) = delete;                  // move assignment, implement when needed
    ~LeafNodePage() = default;

    void* getNthKey(const KeyNumber &keyIndex, AttrLength &length);
    void deleteKeysStartFrom(const KeyNumber &keyIndex);

    /* Add a RID */
    // if key type is varchar, *key is a char pointer and length is varchar's length
    // if key already exists, the new rid will append to the existed rids of the same key
    void addKey(const void *key, const AttrLength &length, const RID &rid);

    /* Delete a RID */
    // if rid not exist, throw assert error
    // if key type is varchar, *key is a char pointer and length is varchar's length
    void deleteKey(const void *key, const AttrLength &length, const RID &rid);

    /* Get next leaf page's pointer */
    // return 0 - success, other - no next leaf page
    RC getNextLeafPageID(PageNum &nextPageID);

    /* Set next leaf page's pointer */
    void setNextLeafPageID(const PageNum &nextPageID);

};

#endif //CS222P_WINTER20_NODEPAGE_H