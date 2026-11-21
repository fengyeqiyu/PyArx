#include "stdafx.h"
#include "PyDbDynBlockReference.h"
#include "PyDbDynBlockReferenceProperty.h"
#include "PyErrorStatusException.h"

// #5 修复：无效对象（空 ObjectId 构造）调用 SDK 方法可能抛异常。
// 统一捕获并打印到命令行，避免 C++ 异常逃逸中断 Python 脚本（符合"SDK 失败不崩溃"规范）。
#define DBREF_TRY try {
#define DBREF_CATCH_RET(DEFAULT) \
	} catch (const std::exception& e) { acutPrintf(L"\n[DynBlockReference] %hs\n", e.what()); return (DEFAULT); } \
	catch (...) { acutPrintf(L"\n[DynBlockReference] unknown SDK exception\n"); return (DEFAULT); }
// 无效对象 guard：空 ObjectId 构造时 m_pObj==nullptr，SDK 方法不可调（new 空对象会挂死），直接返回默认
#define DBREF_GUARD(DEFAULT) \
	if (!d.isValid()) { acutPrintf(L"\n[DynBlockReference] 无效对象（空 ObjectId 构造）\n"); return (DEFAULT); }

PyDbDynBlockReference::PyDbDynBlockReference(uint64_t blockRefOldId) {
	// 空 ObjectId（0）不构造 SDK 对象：new AcDbDynBlockReference(nullId) 会挂死，改为 m_pObj=nullptr
	if (blockRefOldId == 0ULL) { m_pObj = nullptr; return; }
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::UIntPtr>(blockRefOldId));
	m_pObj = new AcDbDynBlockReference(id);
}

PyDbDynBlockReference::~PyDbDynBlockReference() {
	delete m_pObj;
	m_pObj = nullptr;
}

void bind_PyDbDynBlockReference(pym& m) {
	py::class_<PyDbDynBlockReference>(m, "DynBlockReference")
		.def(py::init<uint64_t>(), py::arg("block_ref_old_id"))
		// 静态：判断指定 BlockTableRecord 是否为动态块（改名避免与实例方法同名触发 pybind 重载限制）
		.def_static("isDynamicBlockByObjectId", [](uint64_t btrOldId) -> bool {
			DBREF_TRY
			AcDbObjectId id;
			id.setFromOldId(static_cast<Adesk::UIntPtr>(btrOldId));
			return AcDbDynBlockReference::isDynamicBlock(id);
			DBREF_CATCH_RET(false)
		}, py::arg("btr_old_id"))
		// 实例：当前块参照是否为动态块
		.def("isDynamicBlock", [](const PyDbDynBlockReference& d) -> bool {
			DBREF_GUARD(false)
			DBREF_TRY
			return d.impObj()->isDynamicBlock();
			DBREF_CATCH_RET(false)
		})
		// blockId -> uint64_t
		.def("blockId", [](const PyDbDynBlockReference& d) -> uint64_t {
			DBREF_GUARD(0ULL)
			DBREF_TRY
			auto id = d.impObj()->blockId();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
			DBREF_CATCH_RET(0ULL)
		})
		// getBlockProperties -> list[DynBlockReferenceProperty]
		.def("getBlockProperties", [](const PyDbDynBlockReference& d) -> std::vector<PyDbDynBlockReferenceProperty> {
			DBREF_GUARD(std::vector<PyDbDynBlockReferenceProperty>{})
			DBREF_TRY
			AcDbDynBlockReferencePropertyArray arr;
			d.impObj()->getBlockProperties(arr);
			std::vector<PyDbDynBlockReferenceProperty> out;
			out.reserve(arr.length());
			for (int i = 0; i < arr.length(); ++i) {
				out.emplace_back(arr[i]);
			}
			return out;
			DBREF_CATCH_RET(std::vector<PyDbDynBlockReferenceProperty>{})
		})
		// resetBlock -> bool
		.def("resetBlock", [](PyDbDynBlockReference& d) -> bool {
			DBREF_GUARD(false)
			DBREF_TRY
			return PyWarnIfError(d.impObj()->resetBlock(), L"AcDbDynBlockReference::resetBlock");
			DBREF_CATCH_RET(false)
		})
		// convertToStaticBlock（无参版：转为匿名静态块）
		.def("convertToStaticBlock", [](PyDbDynBlockReference& d) -> bool {
			DBREF_GUARD(false)
			DBREF_TRY
			return PyWarnIfError(d.impObj()->convertToStaticBlock(), L"AcDbDynBlockReference::convertToStaticBlock");
			DBREF_CATCH_RET(false)
		})
		// convertToStaticBlock（命名版：指定新块名）
		.def("convertToStaticBlock", [](PyDbDynBlockReference& d, const AcString& newName) -> bool {
			DBREF_GUARD(false)
			DBREF_TRY
			return PyWarnIfError(d.impObj()->convertToStaticBlock(newName.kwszPtr()),
				L"AcDbDynBlockReference::convertToStaticBlock(name)");
			DBREF_CATCH_RET(false)
		}, py::arg("new_block_name"))
		// dynamicBlockTableRecord -> uint64_t
		.def("dynamicBlockTableRecord", [](const PyDbDynBlockReference& d) -> uint64_t {
			DBREF_GUARD(0ULL)
			DBREF_TRY
			auto id = d.impObj()->dynamicBlockTableRecord();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
			DBREF_CATCH_RET(0ULL)
		})
		// anonymousBlockTableRecord -> uint64_t
		.def("anonymousBlockTableRecord", [](const PyDbDynBlockReference& d) -> uint64_t {
			DBREF_GUARD(0ULL)
			DBREF_TRY
			auto id = d.impObj()->anonymousBlockTableRecord();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
			DBREF_CATCH_RET(0ULL)
		})
		;
}
