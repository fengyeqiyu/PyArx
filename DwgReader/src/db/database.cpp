#include "dwg_reader/db/database.h"

#include <fstream>
#include <stdexcept>

#include "dwg_reader/db/entity/decode.h"
#include "dwg_reader/value/error.h"

namespace dwr {

Database::Database() = default;

Database::~Database() {
    close();
}

bool Database::open(const std::string& path) {
    close();
    try {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("cannot open: " + path);
        char magic[7] = {};
        f.read(magic, 6);
        f.close();

        m_version = versionFromCode(magic);
        if (m_version == DwgVersion::kUnknown) {
            throw std::runtime_error("unrecognized version code: " + std::string(magic));
        }
        if (m_version < DwgVersion::kAc1018) {
            // R13/R14/R2000：经典节定位器布局。对象地址直接是文件绝对偏移，
            // 因此把整份文件当作对象缓冲，索引按 §2 的地址表建立。
            m_classic = openClassicDwg(path);
            m_objects = m_classic.data;
            m_open = !m_classic.objectAddresses.empty();
            if (m_open) {
                m_index.build(m_objects.data(), m_objects.size(),
                              m_classic.objectAddresses, m_version);
                for (const auto& ref : m_index.objects()) {
                    auto ent = decodeObject(m_objects.data(), m_objects.size(), ref, m_version);
                    if (ent) m_entities.push_back(std::move(ent));
                }
            }
            m_err = 0;
            return m_open;
        }

        if (m_version == DwgVersion::kAc1021) {
            m_container1021 = openDwg1021(path);
            m_objects = m_container1021.readSection("AcDb:AcDbObjects");
            m_handlesBuf = m_container1021.readSection("AcDb:Handles");
        } else {
            // R2004 / R2010 / R2013 / R2018：同一 0x2000 页式容器，数据页/段表口径一致
            m_container = openDwg(path);
            m_objects = m_container.readSection("AcDb:AcDbObjects");
            m_handlesBuf = m_container.readSection("AcDb:Handles");
        }
        m_open = !m_objects.empty();
        if (m_open) {
            m_handles.build(m_handlesBuf.data(), m_handlesBuf.size(), m_version);
            m_index.build(m_objects.data(), m_objects.size(), m_handles.pairs(), m_version);
            // 目录地址是记录前缀起点（数据起点 +2）；少数记录前缀长度不同，
            // ObjectIndex 已按对象头 handle 自校验纠正，这里把结果写回句柄目录。
            for (const auto& ref : m_index.objects()) {
                m_handles.addMapping(ref.handle, ref.address);
            }
            // 公共段/实体装配：按版本口径解码，单条失败隔离。
            for (const auto& ref : m_index.objects()) {
                auto ent = decodeObject(m_objects.data(), m_objects.size(), ref, m_version);
                if (ent) m_entities.push_back(std::move(ent));
            }
        }
        m_err = 0;
    } catch (const std::exception& e) {
        m_errText = e.what();
        m_err = 1;
        m_open = false;
    }
    return m_open;
}

void Database::close() {
    m_objects.clear();
    m_handlesBuf.clear();
    m_entities.clear();
    m_container.data.clear();
    m_container.sections.clear();
    m_container.pageMap.clear();
    m_container1021.data.clear();
    m_container1021.sections.clear();
    m_container1021.pages.clear();
    m_version = DwgVersion::kUnknown;
    m_open = false;
    m_errText.clear();
}

}  // namespace dwr
