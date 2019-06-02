#include "block_manager.h"

BlockGuard::BlockGuard(BlockManager* mgr, const BlockEntry& entry) : entry(entry), mgr(mgr)
{
    m_addr = this->mgr->use_block(this->entry);
}

BlockGuard::BlockGuard(BlockManager* mgr, const string& file_path, int block_index) : entry(BlockEntry(file_path, block_index)), mgr(mgr)
{
    m_addr = this->mgr->use_block(this->entry);
}

BlockGuard::BlockGuard(BlockGuard&& guard) noexcept : entry(guard.entry), mgr(guard.mgr)
{
    m_addr = guard.m_addr;
    guard.m_addr = nullptr;
}

void BlockGuard::set_modified()
{
    mgr->set_block_modified(entry);
}

BlockGuard::~BlockGuard()
{
    if (m_addr) {
        mgr->return_block(this->entry);
    }
}

BlockEntry::BlockEntry(string file_path, int block_index)
{
    this->file_path = file_path;
    this->block_index = block_index;
}

template<>
class LRUEvictor<BlockEntry> {
    BlockManager* mgr;
public:
    LRUEvictor(BlockManager* mgr) : mgr(mgr) {};
    bool try_evict(BlockEntry tag) {
        auto& info = mgr->active_blocks.at(tag);
        bool succ = info.use_count == 0;
        if (succ) {
            if (info.modified) mgr->block_writeback(tag, info.addr);
            delete info.addr;
            mgr->active_blocks.erase(tag);
        }
        return succ;
    }
};

FILE* BlockManager::use_file(const string& path) {
    auto& active = active_files.find(path);
    if (active == active_files.end()) {
        FILE* fp = fopen(path.c_str(), "rb+");
        if (!fp) {
            fp = fopen(path.c_str(), "wb+");
        }
        active_files.insert(pair<string, FILE*>(path, fp));
        return fp;
    }
    else {
        return active->second;
    }
}

void BlockManager::block_writeback(const BlockEntry& entry, void* data) {
    FILE* fp = use_file(entry.file_path.c_str());
    fseek(fp, entry.block_index * BLOCK_SIZE, SEEK_SET);
    fwrite(data, BLOCK_SIZE, 1, fp);
}


BlockManager::BlockManager(int max_blocks)
{
    auto evictor = new LRUEvictor<BlockEntry>(this);
    lru = new LRU<BlockEntry>(max_blocks, evictor);
}

BlockManager::~BlockManager()
{
    flush();
    for (auto& p : active_blocks) {
        delete p.second.addr;
    }
    for (auto& p : active_files) {
        fclose(p.second);
    }
    delete lru;
}

void BlockManager::flush() {
    for (auto& p : active_blocks) {
        if (p.second.modified) block_writeback(p.first, p.second.addr);
        p.second.modified = false;
    }
    for (auto& p : active_files) {
        fflush(p.second);
    }
}

int BlockManager::file_blocks(const string& file_path)
{
    FILE* fp = use_file(file_path.c_str());
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    return size / BLOCK_SIZE;
}

int BlockManager::file_append_block(const string& file_path) {
    FILE* fp = use_file(file_path.c_str());
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    int index = ftell(fp) / BLOCK_SIZE;
    static uint8_t data[BLOCK_SIZE] = { 0 };
    fwrite(&data, sizeof(data), 1, fp);
    return index;
}

void* BlockManager::use_block(const BlockEntry& entry)
{
    auto& active = active_blocks.find(entry);
    if (active == active_blocks.end()) {

        FILE* fp = use_file(entry.file_path.c_str());
        fseek(fp, 0, SEEK_END);

        int size = ftell(fp);
        int size_expect = (entry.block_index + 1) * BLOCK_SIZE;
        if (size < size_expect) {
            throw logic_error("File is not long enough. Check block count first.");
        }
        else {
            fseek(fp, entry.block_index * BLOCK_SIZE, SEEK_SET);
            void* addr = new uint8_t[BLOCK_SIZE];
            fread(addr, BLOCK_SIZE, 1, fp);

            ActiveBlockInfo info;
            info.use_count = 1;
            info.addr = addr;
            info.handle = lru->add(entry);
            active_blocks.insert(pair<BlockEntry, ActiveBlockInfo> { entry, info });
            return addr;
        }
    }
    else {
        auto& info = active->second;
        info.use_count++;
        lru->use(info.handle);
        return info.addr;
    }
}

void BlockManager::return_block(const BlockEntry& entry)
{
    auto& info = active_blocks.at(entry);
    info.use_count--;
}

void BlockManager::set_block_modified(const BlockEntry& entry)
{
    auto& info = active_blocks.at(entry);
    info.modified = true;
}
