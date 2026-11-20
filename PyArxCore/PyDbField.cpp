#include "PyDbField.h"
#include "PyErrorStatusException.h"
#include "PyDbDatabase.h"

PyWrapper_Cpp(DbField);

PyDbField* PyDbField::create(const AcString& fieldCode, bool bTextField)
{
	AcDbField* pField = new AcDbField(fieldCode, bTextField);
	if (pField == nullptr)
		throw py::value_error("new AcDbField failed");
	return new PyDbField(pField);
}

int PyDbField::state() const
{
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->state());
}

int PyDbField::evaluationStatus() const
{
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->evaluationStatus());
}

int PyDbField::evaluationOption() const
{
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->evaluationOption());
}

bool PyDbField::setEvaluationOption(int option)
{
	if (!impObj()) return false;
	Acad::ErrorStatus es = impObj()->setEvaluationOption(static_cast<AcDbField::EvalOption>(option));
	return PyWarnIfError(es, L"PyDbField::setEvaluationOption");
}

int PyDbField::filingOption() const
{
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->filingOption());
}

bool PyDbField::setFilingOption(int option)
{
	if (!impObj()) return false;
	Acad::ErrorStatus es = impObj()->setFilingOption(static_cast<AcDbField::FilingOption>(option));
	return PyWarnIfError(es, L"PyDbField::setFilingOption");
}

AcString PyDbField::evaluatorId() const
{
	if (!impObj()) return AcString(L"");
	const ACHAR* psz = impObj()->evaluatorId();
	return AcString(psz != nullptr ? psz : L"");
}

bool PyDbField::setEvaluatorId(const AcString& evaluatorId)
{
	if (!impObj()) return false;
	Acad::ErrorStatus es = impObj()->setEvaluatorId(evaluatorId);
	return PyWarnIfError(es, L"PyDbField::setEvaluatorId");
}

bool PyDbField::isTextField() const
{
	if (!impObj()) return false;
	return impObj()->isTextField();
}

bool PyDbField::convertToTextField()
{
	if (!impObj()) return false;
	Acad::ErrorStatus es = impObj()->convertToTextField();
	return PyWarnIfError(es, L"PyDbField::convertToTextField");
}

int PyDbField::dataType() const
{
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->dataType());
}

int PyDbField::childCount() const
{
	if (!impObj()) return 0;
	return impObj()->childCount();
}

AcString PyDbField::getFieldCode(int flag) const
{
	if (!impObj()) return AcString(L"");
	AcString sCode;
	if (!PyWarnIfError(impObj()->getFieldCode(sCode, static_cast<AcDbField::FieldCodeFlag>(flag)), L"PyDbField::getFieldCode"))
		return AcString(L"");
	return sCode;
}

bool PyDbField::setFieldCode(const AcString& fieldCode, int flag)
{
	if (!impObj()) return false;
	Acad::ErrorStatus es = impObj()->setFieldCode(fieldCode, static_cast<AcDbField::FieldCodeFlag>(flag));
	return PyWarnIfError(es, L"PyDbField::setFieldCode");
}

AcString PyDbField::getFormat() const
{
	if (!impObj()) return AcString(L"");
	AcString sFormat;
	if (!PyWarnIfError(impObj()->getFormat(sFormat), L"PyDbField::getFormat"))
		return AcString(L"");
	return sFormat;
}

bool PyDbField::setFormat(const AcString& format)
{
	if (!impObj()) return false;
	Acad::ErrorStatus es = impObj()->setFormat(format);
	return PyWarnIfError(es, L"PyDbField::setFormat");
}

AcString PyDbField::getValue() const
{
	if (!impObj()) return AcString(L"");
	AcString sValue;
	if (!PyWarnIfError(impObj()->getValue(sValue), L"PyDbField::getValue"))
		return AcString(L"");
	return sValue;
}

py::tuple PyDbField::evaluate(int context, PyDbDatabase* db)
{
	if (!impObj()) return py::make_tuple(0, 0);
	int nFound = 0;
	int nEvaluated = 0;
	AcDbDatabase* pDb = (db != nullptr) ? db->impObj() : nullptr;
	if (!PyWarnIfError(impObj()->evaluate(context, pDb, &nFound, &nEvaluated), L"AcDbField::evaluate")) return py::make_tuple(0, 0);
	py::tuple t(2);
	t[0] = nFound;
	t[1] = nEvaluated;
	return t;
}

PyDbField* PyDbField::getChild(int index, AcDb::OpenMode mode)
{
	if (!impObj()) return nullptr;
	AcDbField* pChild = nullptr;
	Acad::ErrorStatus es = impObj()->getChild(index, pChild, mode);
	if (es != Acad::eOk || pChild == nullptr)
		throw py::index_error("getChild failed");
	return new PyDbField(pChild);
}

bool PyDbField::setInObject(PyDbObject& obj, const AcString& propName)
{
	if (!impObj()) return false;
	Acad::ErrorStatus es = impObj()->setInObject(obj.impObj(), propName);
	return PyWarnIfError(es, L"PyDbField::setInObject");
}

bool PyDbField::postInDatabase(PyDbDatabase& db)
{
	if (!impObj()) return false;
	Acad::ErrorStatus es = impObj()->postInDatabase(db.impObj());
	return PyWarnIfError(es, L"PyDbField::postInDatabase");
}

void bind_PyDbField(pym& m)
{
	// ---- 枚举：AcDbField 内部五组（PyRx 同名）----
	if (!py::hasattr(m, "FieldState"))
	{
		py::enum_<AcDbField::State>(m, "FieldState")
			.value("kInitialized", AcDbField::kInitialized)
			.value("kCompiled", AcDbField::kCompiled)
			.value("kModified", AcDbField::kModified)
			.value("kEvaluated", AcDbField::kEvaluated)
			.value("kHasCache", AcDbField::kHasCache)
			.value("kHasFormattedString", AcDbField::kHasFormattedString);
	}
	if (!py::hasattr(m, "FieldEvalOption"))
	{
		py::enum_<AcDbField::EvalOption>(m, "FieldEvalOption")
			.value("kDisable", AcDbField::kDisable)
			.value("kOnOpen", AcDbField::kOnOpen)
			.value("kOnSave", AcDbField::kOnSave)
			.value("kOnPlot", AcDbField::kOnPlot)
			.value("kOnEtransmit", AcDbField::kOnEtransmit)
			.value("kOnRegen", AcDbField::kOnRegen)
			.value("kOnDemand", AcDbField::kOnDemand)
			.value("kAutomatic", AcDbField::kAutomatic);
	}
	if (!py::hasattr(m, "FieldEvalContext"))
	{
		py::enum_<AcDbField::EvalContext>(m, "FieldEvalContext")
			.value("kOpen", AcDbField::kOpen)
			.value("kSave", AcDbField::kSave)
			.value("kPlot", AcDbField::kPlot)
			.value("kEtransmit", AcDbField::kEtransmit)
			.value("kRegen", AcDbField::kRegen)
			.value("kDemand", AcDbField::kDemand)
			.value("kPreview", AcDbField::kPreview)
			.value("kPlotPreview", AcDbField::kPlotPreview);
	}
	if (!py::hasattr(m, "FieldEvalStatus"))
	{
		py::enum_<AcDbField::EvalStatus>(m, "FieldEvalStatus")
			.value("kNotYetEvaluated", AcDbField::kNotYetEvaluated)
			.value("kSuccess", AcDbField::kSuccess)
			.value("kEvaluatorNotFound", AcDbField::kEvaluatorNotFound)
			.value("kSyntaxError", AcDbField::kSyntaxError)
			.value("kInvalidCode", AcDbField::kInvalidCode)
			.value("kInvalidContext", AcDbField::kInvalidContext)
			.value("kOtherError", AcDbField::kOtherError);
	}
	if (!py::hasattr(m, "FieldCodeFlag"))
	{
		py::enum_<AcDbField::FieldCodeFlag>(m, "FieldCodeFlag")
			.value("kFieldCode", AcDbField::kFieldCode)
			.value("kEvaluatedText", AcDbField::kEvaluatedText)
			.value("kEvaluatedChildren", AcDbField::kEvaluatedChildren)
			.value("kObjectReference", AcDbField::kObjectReference)
			.value("kAddMarkers", AcDbField::kAddMarkers)
			.value("kEscapeBackslash", AcDbField::kEscapeBackslash)
			.value("kStripOptions", AcDbField::kStripOptions)
			.value("kPreserveFields", AcDbField::kPreserveFields)
			.value("kTextField", AcDbField::kTextField)
			.value("kPreserveOptions", AcDbField::kPreserveOptions)
			.value("kDetachChildren", AcDbField::kDetachChildren)
			.value("kChildObjectReference", AcDbField::kChildObjectReference)
			.value("kForExpression", AcDbField::kForExpression);
	}

	if (py::hasattr(m, "Field"))
		return;

	py::class_<PyDbField, PyDbObject>(m, "Field", "AcDbField 包装（对齐 PyRx Db.Field）")
		.def(py::init<>())
		.def(py::init([]() { return new PyDbField(new AcDbField()); }))
		.def(py::init([](const AcString& fieldCode) { return PyDbField::create(fieldCode, false); }),
			py::arg("fieldCode"))
		.def(py::init([](const AcString& fieldCode, bool bTextField) { return PyDbField::create(fieldCode, bTextField); }),
			py::arg("fieldCode"), py::arg("bTextField"))
		.def(py::init([](const AcDbObjectId& id, AcDb::OpenMode mode, bool erased) {
			AcDbField* pField = nullptr;
			Acad::ErrorStatus es = acdbOpenObject(pField, id, mode, erased);
			if (es != Acad::eOk || pField == nullptr)
				throw py::value_error("AcDbField open failed");
			return new PyDbField(pField);
		}), py::arg("id"), py::arg("mode") = AcDb::kForRead, py::arg("erased") = false)
		.def_static("className", []() { return PyDbField::className(); })
		.def_static("desc", []() { return PyDbField::desc(); })
		.def("state", &PyDbField::state)
		.def("evaluationStatus", &PyDbField::evaluationStatus)
		.def("evaluationOption", &PyDbField::evaluationOption)
		.def("setEvaluationOption", &PyDbField::setEvaluationOption, py::arg("option"))
		.def("filingOption", &PyDbField::filingOption)
		.def("setFilingOption", &PyDbField::setFilingOption, py::arg("option"))
		.def("evaluatorId", &PyDbField::evaluatorId)
		.def("setEvaluatorId", &PyDbField::setEvaluatorId, py::arg("evaluatorId"))
		.def("isTextField", &PyDbField::isTextField)
		.def("convertToTextField", &PyDbField::convertToTextField)
		.def("dataType", &PyDbField::dataType)
		.def("childCount", &PyDbField::childCount)
		.def("getFieldCode", &PyDbField::getFieldCode, py::arg("flag"))
		.def("setFieldCode", &PyDbField::setFieldCode, py::arg("fieldCode"), py::arg("flag") = 0)
		.def("getFormat", &PyDbField::getFormat)
		.def("setFormat", &PyDbField::setFormat, py::arg("format"))
		.def("getValue", &PyDbField::getValue)
		.def("evaluate", &PyDbField::evaluate, py::arg("context") = 0, py::arg("db") = py::none())
		.def("getChild", &PyDbField::getChild, py::arg("index"), py::arg("mode"))
		.def("setInObject", &PyDbField::setInObject, py::arg("obj"), py::arg("propName"))
		.def("postInDatabase", &PyDbField::postInDatabase, py::arg("db"));
}
