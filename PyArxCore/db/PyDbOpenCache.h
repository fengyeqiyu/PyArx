#pragma once
#include "stdafx.h"
#include <map>
#include <mutex>

// 绑定层打开对象缓存：AcDb 同一对象在同一时刻只允许一个打开包装。
// 读请求复用已有包装；写请求遇到已有读包装时走 upgradeOpen 升级；
// 显式 close 时从缓存摘除并关闭底层对象。事务托管对象由事务管理器统一关闭，
// 不在这里重复 close。
class PyDbOpenCache {
public:
    struct Entry {
        AcDbObject* object = nullptr;
        AcDb::OpenMode mode = AcDb::kForRead;
    };

    static AcDbObject* Open(AcDbObjectId id, AcDb::OpenMode mode, Acad::ErrorStatus& es) {
        if (id.isNull()) { es = Acad::eNullObjectPointer; return nullptr; }
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(id);
        if (it != cache_.end()) {
            Entry& e = it->second;
            if (!e.object || e.object->isReallyClosing()) {
                cache_.erase(it);
            } else if (mode == AcDb::kForWrite && e.mode == AcDb::kForRead) {
                es = e.object->upgradeOpen();
                if (es == Acad::eOk) e.mode = AcDb::kForWrite;
                return es == Acad::eOk ? e.object : nullptr;
            } else {
                es = Acad::eOk;
                return e.object;
            }
        }

        AcDbObject* p = nullptr;
        es = acdbOpenObject(p, id, mode);
        if (es == Acad::eOk && p) cache_[id] = Entry{ p, mode };
        return p;
    }

    static AcDbObject* Get(AcDbObjectId id, AcDb::OpenMode mode, Acad::ErrorStatus& es) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(id);
        if (it == cache_.end()) { es = Acad::eNullObjectPointer; return nullptr; }
        Entry& e = it->second;
        if (!e.object || e.object->isReallyClosing()) { cache_.erase(it); es = Acad::eNullObjectPointer; return nullptr; }
        if (mode == AcDb::kForWrite && e.mode == AcDb::kForRead) {
            es = e.object->upgradeOpen();
            if (es == Acad::eOk) e.mode = AcDb::kForWrite;
            return es == Acad::eOk ? e.object : nullptr;
        }
        es = Acad::eOk;
        return e.object;
    }

    static void Track(AcDbObject* p, AcDb::OpenMode mode) {
        if (!p) return;
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[p->objectId()] = Entry{ p, mode };
    }

    static void Close(AcDbObject* p) {
        if (!p) return;
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.object == p) {
                cache_.erase(it);
                break;
            }
        }
    }

    static void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }

private:
    static std::map<AcDbObjectId, Entry> cache_;
    static std::mutex mutex_;
};

inline std::map<AcDbObjectId, PyDbOpenCache::Entry> PyDbOpenCache::cache_;
inline std::mutex PyDbOpenCache::mutex_;
