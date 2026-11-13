#pragma once
#include "stdafx.h"
#include "dbdate.h"

// AcDbDate value wrapper (by-value member; AcDbDate has no AcRxObject base,
// unlike the pointer-managed PyRxObject-derived wrappers). Mirrors dbdate.h.
class PyDbDate {
public:
	PyDbDate();
	explicit PyDbDate(const AcDbDate& d);

	// Static factories matching AcDbDate::InitialValue without exposing the
	// SDK enum to Python (kInitZero / kInitLocalTime / kInitUniversalTime).
	static PyDbDate zero();
	static PyDbDate localTime();
	static PyDbDate universalTime();

	AcDbDate& impObj();
	const AcDbDate& impObj() const;

	// ---- date (month/day/year) ----
	short month() const;
	short day() const;
	short year() const;
	void setMonth(int v);
	void setDay(int v);
	void setYear(int v);
	void setDate(int month, int day, int year);

	// ---- time of day ----
	short hour() const;
	short minute() const;
	short second() const;
	short millisecond() const;
	void setHour(int v);
	void setMinute(int v);
	void setSecond(int v);
	void setMillisecond(int v);
	void setTime(int hour, int min, int sec, int msec);

	// ---- approximate time (15-minute units) ----
	int getApproximateTime() const;
	void setApproximateTime(int time);

	// ---- misc ----
	void setToZero();
	void getLocalTime();
	void getUniversalTime();
	void localToUniversal();
	void universalToLocal();

	// ---- Julian representation ----
	Adesk::Int32 julianDay() const;
	Adesk::Int32 msecsPastMidnight() const;
	void setJulianDay(Adesk::Int32 julianDay);
	void setMsecsPastMidnight(Adesk::Int32 msec);
	void setJulianDate(Adesk::Int32 julianDay, Adesk::Int32 msec);
	double julianFraction() const;
	void setJulianFraction(double fraction);

	AcString getLocalDisplayString() const;

	// ---- comparison ----
	bool operator==(const PyDbDate& o) const;
	bool operator>(const PyDbDate& o) const;
	bool operator>=(const PyDbDate& o) const;
	bool operator<(const PyDbDate& o) const;
	bool operator<=(const PyDbDate& o) const;

	// ---- arithmetic (returns a new PyDbDate) ----
	PyDbDate operator+(const PyDbDate& o) const;
	PyDbDate operator-(const PyDbDate& o) const;

	AcString toString() const;

private:
	AcDbDate m_date;
};

void bind_PyDbDate(pym& m);
