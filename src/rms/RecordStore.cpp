#include "RecordStore.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <numeric>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef WIN32
#include <libgen.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

#include "RecordStoreException.h"
#include "../utils/FileStream.h"
#include "../utils/String.h"

#ifdef __EMSCRIPTEN__
EM_ASYNC_JS(void, syncRecordStoreFs, (int populate), {
    if (!Module.gravityDefiedRecordStoreMounted) {
        if (!FS.analyzePath('/gravity_defied').exists) {
            FS.mkdir('/gravity_defied');
        }
        FS.mount(IDBFS, {}, '/gravity_defied');
        Module.gravityDefiedRecordStoreMounted = true;
    }

    await new Promise((resolve, reject) => {
        FS.syncfs(!!populate, (err) => {
            if (err) {
                reject(err);
            } else {
                resolve();
            }
        });
    });
});
#endif

RecordStore::RecordStore(std::filesystem::path filePath, RecordEnumerationImpl* records)
{
    this->filePath = filePath;
    this->records.reset(records);
}

RecordEnumeration* RecordStore::enumerateRecords(RecordFilter* filter, RecordComparator* comparator, bool keepUpdated)
{
    assert(filter == nullptr);
    assert(comparator == nullptr);
    assert(!keepUpdated);
    log("enumerateRecords()");
    return records.get();
}

void RecordStore::closeRecordStore()
{
    // nothing
}

int RecordStore::addRecord(std::vector<int8_t> arr, int offset, int numBytes)
{
    log("addRecord()");
    assert(static_cast<int>(arr.size()) == numBytes);
    assert(offset == 0);
    int id = records->addRecord(arr);
    save();
    return id;
}

void RecordStore::setRecord(int recordId, std::vector<int8_t> arr, int offset, int numBytes)
{
    (void)offset;
    (void)numBytes;
    records->setRecord(recordId, arr);
    save();
}

void RecordStore::save()
{
    FileStream outStream(filePath, std::ios::out | std::ios::binary);
    records->serialize(&outStream);
#ifdef __EMSCRIPTEN__
    syncRecordStoreFs(0);
#endif
}

RecordEnumerationImpl* RecordStore::load(std::filesystem::path filePath)
{
    RecordEnumerationImpl* temp = new RecordEnumerationImpl();
    FileStream inStream(filePath, std::ios::in | std::ios::binary);
    temp->deserialize(&inStream);
    return temp;
}

RecordStore* RecordStore::openRecordStore(std::string name, bool createIfNecessary)
{
    if (opened.find(name) == opened.end()) {
        opened[name] = createRecordStore(name, createIfNecessary);
    }

    return opened[name].get();
}

std::unique_ptr<RecordStore> RecordStore::createRecordStore(std::string name, bool createIfNecessary)
{
    log("createRecordStore(" + name + ", " + std::to_string(createIfNecessary) + ")");
    std::filesystem::path filePath = recordStoreDir / std::filesystem::path(name);

    if (std::filesystem::exists(filePath)) {
        return std::unique_ptr<RecordStore>(new RecordStore(filePath, load(filePath)));
    }

    if (createIfNecessary) {
        std::filesystem::create_directories(filePath.parent_path());

        std::unique_ptr<RecordStore> rs(new RecordStore(filePath, new RecordEnumerationImpl()));
        rs->save();
        return rs;
    } else {
        throw RecordStoreException();
    }
}

std::vector<std::string> RecordStore::listRecordStores()
{
    std::vector<std::string> result;

    if (std::filesystem::exists(recordStoreDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(recordStoreDir)) {
            result.push_back(entry.path().filename().string());
        }
    }

    log("listRecordStores() = {" + String::join(result, ", ") + "}");

    return result;
}

void RecordStore::deleteRecordStore(std::string name)
{
    log("deleteRecordStore(" + name + ")");
    opened.erase(name);
    std::filesystem::remove(recordStoreDir / std::filesystem::path(name));
#ifdef __EMSCRIPTEN__
    syncRecordStoreFs(0);
#endif
}

void RecordStore::log(std::string s)
{
    std::cout << s << std::endl;
}

void RecordStore::setRecordStoreDir([[maybe_unused]] const char* progName)
{
#ifdef __EMSCRIPTEN__
    syncRecordStoreFs(1);
    recordStoreDir = std::filesystem::path("/gravity_defied") / "recordStore";
#elif defined(WIN32)
    const char* base = dirname(strdup(progName));
    recordStoreDir = std::filesystem::path(base) / "recordStore";
#else
    const char* homeDir = getenv("HOME");
    if (!homeDir)
        homeDir = getpwuid(getuid())->pw_dir;

    if (!homeDir)
        throw std::system_error(errno, std::system_category(), "Error getting home directory");

    recordStoreDir = std::filesystem::path(homeDir) / ".GravityDefied";
#endif
}
