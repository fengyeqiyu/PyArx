#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDate.h"

namespace {
	std::string acsToNarrow(const AcString& s) {
		const char* u = s.utf8Ptr();
		return u ? std::string(u) : std::string();
	}
}

PyDbDate::PyDbDate() : m_date() {}
PyDbDate::PyDbDate(const AcDbDate& d) : m_date(d) {}

PyDbDate PyDbDate::zero() { return PyDbDate(AcDbDate(AcDbDate::kInitZero)); }
PyDbDate PyDbDate::localTime() { return PyDbDate(AcDbDate(AcDbDate::kInitLocalTime)); }
PyDbDate PyDbDate::universalTime() { return PyDbDate(AcDbDate(AcDbDate::kInitUniversalTime)); }

AcDbDate& PyDbDate::impObj() { return m_date; }
const AcDbDate& PyDbDate::impObj() const { return m_date; }

short PyDbDate::month() const { return m_date.month(); }
short PyDbDate::day() const { return m_date.day(); }
short PyDbDate::year() const { return m_date.year(); }
void PyDbDate::setMonth(int v) { m_date.setMonth(v); }
void PyDbDate::setDay(int v) { m_date.setDay(v); }
void PyDbDate::setYear(int v) { m_date.setYear(v); }
void PyDbDate::setDate(int month, int day, int year) { m_date.setDate(month, day, year); }

short PyDbDate::hour() const { return m_date.hour(); }
short PyDbDate::minute() const { return m_date.minute(); }
short PyDbDate::second() const { return m_date.second(); }
short PyDbDate::millisecond() const { return m_date.millisecond(); }
void PyDbDate::setHour(int v) { m_date.setHour(v); }
void PyDbDate::setMinute(int v) { m_date.setMinute(v); }
void PyDbDate::setSecond(int v) { m_date.setSecond(v); }
void PyDbDate::setMillisecond(int v) { m_date.setMillisecond(v); }
void PyDbDate::setTime(int hour, int min, int sec, int msec) { m_date.setTime(hour, min, sec, msec); }

// Skeleton: AcDbDate::getApproximateTime/setApproximateTime are declared in
// dbdate.h but not exported by the runtime lib; left unbound and unreferenced.
int PyDbDate::getApproximateTime() const { return 0; }
void PyDbDate::setApproximateTime(int) { /* not exported; no-op */ }

void PyDbDate::setToZero() { m_date.setToZero(); }
void PyDbDate::getLocalTime() { m_date.getLocalTime(); }
void PyDbDate::getUniversalTime() { m_date.getUniversalTime(); }
void PyDbDate::localToUniversal() { m_date.localToUniversal(); }
void PyDbDate::universalToLocal() { m_date.universalToLocal(); }

Adesk::Int32 PyDbDate::julianDay() const { return m_date.julianDay(); }
Adesk::Int32 PyDbDate::msecsPastMidnight() const { return m_date.msecsPastMidnight(); }
void PyDbDate::setJulianDay(Adesk::Int32 j) { m_date.setJulianDay(j); }
void PyDbDate::setMsecsPastMidnight(Adesk::Int32 m) { m_date.setMsecsPastMidnight(m); }
void PyDbDate::setJulianDate(Adesk::Int32 j, Adesk::Int32 m) { m_date.setJulianDate(j, m); }
double PyDbDate::julianFraction() const { return m_date.julianFraction(); }
void PyDbDate::setJulianFraction(double f) { m_date.setJulianFraction(f); }

AcString PyDbDate::getLocalDisplayString() const {
	AcString s = m_date.getLocalDisplayString();
	const ACHAR* p = s.constPtr();
	return p ? AcString(p) : AcString();
}

bool PyDbDate::operator==(const PyDbDate& o) const { return m_date == o.m_date; }
bool PyDbDate::operator>(const PyDbDate& o) const { return m_date > o.m_date; }
bool PyDbDate::operator>=(const PyDbDate& o) const { return m_date >= o.m_date; }
bool PyDbDate::operator<(const PyDbDate& o) const { return m_date < o.m_date; }
bool PyDbDate::operator<=(const PyDbDate& o) const { return m_date <= o.m_date; }

PyDbDate PyDbDate::operator+(const PyDbDate& o) const { return PyDbDate(m_date + o.m_date); }
PyDbDate PyDbDate::operator-(const PyDbDate& o) const { return PyDbDate(m_date - o.m_date); }

AcString PyDbDate::toString() const {
	return AcString(std::format("PyDbDate[jd={},ms={}]", static_cast<long long>(julianDay()),
		static_cast<long long>(msecsPastMidnight())).c_str(), AcString::Utf8);
}

void bind_PyDbDate(pym& m) {
	py::class_<PyDbDate>(m, "PyDbDate")
		.def(py::init<>())
		.def_static("zero", &PyDbDate::zero)
		.def_static("localTime", &PyDbDate::localTime)
		.def_static("universalTime", &PyDbDate::universalTime)
		// date
		.def("month", &PyDbDate::month)
		.def("day", &PyDbDate::day)
		.def("year", &PyDbDate::year)
		.def("setMonth", &PyDbDate::setMonth, py::arg("v"))
		.def("setDay", &PyDbDate::setDay, py::arg("v"))
		.def("setYear", &PyDbDate::setYear, py::arg("v"))
		.def("setDate", &PyDbDate::setDate, py::arg("month"), py::arg("day"), py::arg("year"))
		// time
		.def("hour", &PyDbDate::hour)
		.def("minute", &PyDbDate::minute)
		.def("second", &PyDbDate::second)
		.def("millisecond", &PyDbDate::millisecond)
		.def("setHour", &PyDbDate::setHour, py::arg("v"))
		.def("setMinute", &PyDbDate::setMinute, py::arg("v"))
		.def("setSecond", &PyDbDate::setSecond, py::arg("v"))
		.def("setMillisecond", &PyDbDate::setMillisecond, py::arg("v"))
		.def("setTime", &PyDbDate::setTime, py::arg("hour"), py::arg("min"), py::arg("sec"), py::arg("msec"))
		// getApproximateTime/setApproximateTime removed: SDK methods not exported
		// ---- misc ----
		.def("setToZero", &PyDbDate::setToZero)
		.def("getLocalTime", &PyDbDate::getLocalTime)
		.def("getUniversalTime", &PyDbDate::getUniversalTime)
		.def("localToUniversal", &PyDbDate::localToUniversal)
		.def("universalToLocal", &PyDbDate::universalToLocal)
		// julian
		.def("julianDay", &PyDbDate::julianDay)
		.def("msecsPastMidnight", &PyDbDate::msecsPastMidnight)
		.def("setJulianDay", &PyDbDate::setJulianDay, py::arg("julianDay"))
		.def("setMsecsPastMidnight", &PyDbDate::setMsecsPastMidnight, py::arg("msec"))
		.def("setJulianDate", &PyDbDate::setJulianDate, py::arg("julianDay"), py::arg("msec"))
		.def("julianFraction", &PyDbDate::julianFraction)
		.def("setJulianFraction", &PyDbDate::setJulianFraction, py::arg("fraction"))
		.def("getLocalDisplayString", &PyDbDate::getLocalDisplayString)
		// operators
		.def("__eq__", &PyDbDate::operator==)
		.def("__gt__", &PyDbDate::operator>)
		.def("__ge__", &PyDbDate::operator>=)
		.def("__lt__", &PyDbDate::operator<)
		.def("__le__", &PyDbDate::operator<=)
		.def("__add__", &PyDbDate::operator+)
		.def("__sub__", &PyDbDate::operator-)
		.def("__repr__", &PyDbDate::toString);
}
