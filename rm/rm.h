#ifndef _rm_h_
#define _rm_h_

#include <string>
#include <vector>

#include "../rbf/rbfm.h"
#include "../rbf/types.h"
#include "../ix/ix.h"

# define RM_EOF (-1)  // end of a scan operator

// RM_ScanIterator is an iterator to go through tuples
class RM_ScanIterator {
private:

public:
    FileHandle fileHandle;

    RBFM_ScanIterator rbfm_ScanIterator;

    RM_ScanIterator() = default;

    ~RM_ScanIterator() = default;

    void setUp(const std::string &tableFileName, const std::string &conditionAttribute,
               CompOp compOp, const void *value, const std::vector<std::string> &attributeNames,
               const std::vector<Attribute>& descriptor);

    // "data" follows the same format as RelationManager::insertTuple()
    // Return: 0 - success, RM_EOF: end
    RC getNextTuple(RID &rid, void *data);

    // Return: same as RecordBasedFileManager::closeFile
    RC close();
};

// RM_IndexScanIterator is an iterator to go through index entries
class RM_IndexScanIterator {
public:
    IXFileHandle ixFileHandle;
    IX_ScanIterator ixScanIterator;

    RM_IndexScanIterator() {};    // Constructor
    ~RM_IndexScanIterator() {};    // Destructor
    void setUp(const std::string &fileName, const Attribute &attribute, const void *lowKey, const void *highKey, bool lowKeyInclusive, bool highKeyInclusive);

    // "key" follows the same format as in IndexManager::insertEntry()
    RC getNextEntry(RID &rid, void *key);    // Get next matching entry
    RC close();                        // Terminate index scan
};

// Relation Manager
class RelationManager {
private:
    TableID tableNumber;
    // insert and delete record, prevent user operation on system tables
    // sys = true, it is a system operation, otherwise not.
    // return: 0 - success, -2 - table not exist, -1 - external sys operation
    RC insertTuple(const std::string &tableName, const void *data, RID &rid, bool sys);
    RC deleteTuple(const std::string &tableName, const RID &rid, bool sys);
    // get "Tables" descriptor
    void getSysTableAttributes(std::vector<Attribute> &descriptor);
    // get "Columns" descriptor
    void getSysColTableAttributes(std::vector<Attribute> &descriptor);
    // get "Indexes" descriptor
    void getSysIdxTableAttributes(std::vector<Attribute> &descriptor);
    // insert meta info of new table into Tables and Columns
    void addMetaInfo(const std::string &tableName, const std::vector<Attribute> &descriptor);
    // get total table numbers
    TableID getTableNumbers();
    // insert meta info of new table to Tables
    void addTablesInfo(const std::string &tableName, TableID id);
    // insert meta info of new table to Columns;
    void addColumnsInfo(const std::string &table, TableID id, const std::vector<Attribute> &descriptor);
    // delete meta info of a deleted table, always assume table exists
    // Return: same as getTableInfo
    RC deleteMetaInfo(const std::string &tableName);
    // judge if a table is a system table
    bool isSysTable(const std::string &tableName);
    // create TypeVarchar raw data
    void *createVarcharData(const std::string &str);
    // insert corresponding entries after inserting tuple
    void insertIdxEntry(const std::string &tableName, const std::vector<Attribute> attrs, const void* data, const RID &rid);
    // delete corresponding entries after inserting tuple
    void deleteIdxEntry(const std::string &tableName, const std::vector<Attribute> attrs, const void* data, const RID &rid);
    // get index filename
    // 0: success, 1: not exist
    RC getIndexFileName(const std::string &tableName, const Attribute &attribute, std::string &fileName);
    // add index info to sys table
    void addIndexInfo(const std::string &fileName);
    // delete index info
    void deleteIndexInfo(const std::string fileName);
public:
    static RelationManager &instance();
    // return: 0 - succsess, others - fail, same as RecordBasedFileManager::createFile();
    RC createCatalog();
    // return: 0 - succsess, others - fail, same as RecordBasedFileManager::destroyFile();
    RC deleteCatalog();
    // return: 0 - succsess, -2 - table already exists, -1 - sys tables, others - fail, same as RecordBasedFileManager::createFile();
    RC createTable(const std::string &tableName, const std::vector<Attribute> &attrs);
    // return: 0 - succsess, -2 - table not exists, -1 - sys tables, others - fail, same as RecordBasedFileManager::destroyFile();
    RC deleteTable(const std::string &tableName);

    // get table id and fileHandle of a given table;
    // return: 0 - success, -2 - table not exist
    RC getTableInfo(const std::string &tableName, TableID &id, std::string &fileName);

    /* Return:
     *  0 - success
     *  -1 - table not exists
     */
    RC getAttributes(const std::string &tableName, std::vector<Attribute> &attrs);
    // return: same as private insertTuple
    // user operation
    RC insertTuple(const std::string &tableName, const void *data, RID &rid);
    // return: same as private deleteTuple
    // user operation
    RC deleteTuple(const std::string &tableName, const RID &rid);
    // return: 0 - succsess, -2 - table not exists, -1 - sys tables, others - fail, same as RecordBasedFileManager::updateRecord();
    RC updateTuple(const std::string &tableName, const void *data, const RID &rid);
    // return: 0 - succsess, -2 - table not exists, -1 - sys tables, others - fail, same as RecordBasedFileManager::readRecord();
    RC readTuple(const std::string &tableName, const RID &rid, void *data);

    // Print a tuple that is passed to this utility method.
    // The format is the same as printRecord().
    // return: 0 - succsess, others - fail, same as RecordBasedFileManager::printRecord();
    RC printTuple(const std::vector<Attribute> &attrs, const void *data);
    // return: 0 - succsess, -2 - table not exists, -1 - sys tables, others - fail, same as RecordBasedFileManager::readAttribute();
    RC readAttribute(const std::string &tableName, const RID &rid, const std::string &attributeName, void *data);

    // Scan returns an iterator to allow the caller to go through the results one by one.
    // Do not store entire results in the scan iterator.
    // Return:
    //   0: success
    RC scan(const std::string &tableName,
            const std::string &conditionAttribute,
            CompOp compOp,                  // comparison type such as "<" and "="
            const void *value,                    // used in the comparison
            const std::vector <std::string> &attributeNames, // a list of projected attributes
            RM_ScanIterator &rm_ScanIterator);

    // Extra credit work (10 points)
    RC addAttribute(const std::string &tableName, const Attribute &attr);

    RC dropAttribute(const std::string &tableName, const std::string &attributeName);

    // print sys table
    void printSysTable(const std::string &tableName);

    // QE IX related
    // 0: success, 1: table not exist, 2: attribute not exist, 3: index exists, 4: open index file fail
    RC createIndex(const std::string &tableName, const std::string &attributeName);

    // 0: success, 1: table not exist, 2: attribute not exist, 3: index not exists
    RC destroyIndex(const std::string &tableName, const std::string &attributeName);

    // indexScan returns an iterator to allow the caller to go through qualified entries in index
    // 0: success, 1: table not exists, 2: attribute not exist, 3: index not exist;
    RC indexScan(const std::string &tableName,
                 const std::string &attributeName,
                 const void *lowKey,
                 const void *highKey,
                 bool lowKeyInclusive,
                 bool highKeyInclusive,
                 RM_IndexScanIterator &rm_IndexScanIterator);

protected:
    RelationManager();                                                  // Prevent construction
    ~RelationManager();                                                 // Prevent unwanted destruction
    RelationManager(const RelationManager &);                           // Prevent construction by copying
    RelationManager &operator=(const RelationManager &);                // Prevent assignment

private:
    static RelationManager *_relation_manager;
};

#endif