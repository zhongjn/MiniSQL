#pragma once

#include "lru.h"
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <stdint.h>

using namespace std;

// 块大小
const int BLOCK_SIZE = 4096;

// 用于唯一确定一个块，由文件路径、文件内块号构成
struct BlockEntry {
    string file_path;
    int block_index = -1;
    BlockEntry() = default;
    BlockEntry(string file_path, int block_index);
};

namespace std {
    template<>
    struct hash<BlockEntry> {
        size_t operator()(const BlockEntry& entry) const {
            size_t h1 = hash<string>()(entry.file_path);
            size_t h2 = hash<int>()(entry.block_index);
            return  h1 ^ (h2 << 1);
        }
    };

    template<>
    struct equal_to<BlockEntry> {
        bool operator()(const BlockEntry& lhs, const BlockEntry& rhs) const
        {
            return lhs.block_index == rhs.block_index && lhs.file_path == rhs.file_path;
        }
    };
}

struct ActiveBlockInfo {
    int use_count;
    void* addr;
    LRUNodeHandle<BlockEntry> handle;
    bool modified = false;
};

class BlockManager {
    friend class LRUEvictor<BlockEntry>;
private:
    LRU<BlockEntry>* lru;
    unordered_map<BlockEntry, ActiveBlockInfo> active_blocks;
    unordered_map<string, FILE*> active_files;
    FILE* use_file(const string& path);
    void block_writeback(const BlockEntry& entry, void* data);
public:
    BlockManager(int max_blocks = 1024);
    BlockManager(const BlockManager&) = delete;

    // 销毁。写回所有改动到硬盘
    ~BlockManager();

    // 将所有改动立刻写回硬盘
    void flush();

    // 获取一个文件内的块数量
    int file_blocks(const string& file_path);

    // 在文件末尾追加一个块，其内容为全0
    int file_append_block(const string& file_path);

    void file_delete(const string& file_path);

    // 获取一个块的访问权，返回块内存地址，大小为BLOCK_SIZE
    void* use_block(const BlockEntry& entry);

    // 返还一个块的访问权
    void return_block(const BlockEntry& entry);

    // 标记该块已被修改，将在之后写回文件
    void set_block_modified(const BlockEntry& entry);
};

// 利用RAII简化块的获取与返还。在BlockGuard构造时获取块访问权，析构时返还访问权。
class BlockGuard {
private:
    BlockManager* mgr;
    BlockEntry entry;
    void* m_addr = nullptr;
public:
    BlockGuard(BlockManager* mgr, const BlockEntry& entry);
    BlockGuard(BlockManager* mgr, const string& file_path, int block_index);
    BlockGuard(const BlockGuard& guard) = delete;
    BlockGuard(BlockGuard&& guard) noexcept;
    ~BlockGuard();

    // 获取块地址，T参数设置地址类型
    template<typename T> T* addr() { return (T*)m_addr; }

    // 获取块地址，T参数设置地址类型，offset为地址偏移量
    template<typename T> T* addr(int offset) { return (T*)((uint8_t*)m_addr + offset); }

    // 获取块地址
    void* addr() { return m_addr; }

    // 设置该块已被修改
    void set_modified();
};