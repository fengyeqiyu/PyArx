#include "dwg_reader/rx/class.h"

namespace dwr {

RxRegistry& RxRegistry::instance() {
    static RxRegistry inst;
    return inst;
}

void RxRegistry::registerName(int typeCode, std::string dxfName) {
    m_names[typeCode] = std::move(dxfName);
}

const std::string* RxRegistry::nameOf(int typeCode) const {
    auto it = m_names.find(typeCode);
    return it == m_names.end() ? nullptr : &it->second;
}

bool RxRegistry::isRegistered(int typeCode) const {
    return m_names.find(typeCode) != m_names.end();
}

}  // namespace dwr
