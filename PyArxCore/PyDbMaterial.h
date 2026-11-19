#pragma once
#include "stdafx.h"
#include <dbMaterial.h>
#include "PyDbObject.h"

class PyDbMaterial : public PyDbObject {
public:
	PyWrapper_H(DbMaterial)
	PyDbMaterial(AcDbMaterial* ptr) : PyDbObject(ptr) {}
	PyDbMaterial() : PyDbMaterial(new AcDbMaterial()) {}

	int channelFlags() const;
	void clearAdskMaterialCache();
	double colorBleedScale() const;
	int finalGather() const;
	int getAdskMaterialFlags() const;
	int globalIllumination() const;
	int illuminationModel() const;
	double indirectBumpScale() const;
	bool isAnonymous() const;
	// isRenamable: SDK2024 未导出该符号，暂不封装
	double luminance() const;
	int luminanceMode() const;
	int mode() const;
	double reflectanceScale() const;
	double reflectivity() const;
	double selfIllumination() const;
	double shininess() const;
	double translucence() const;
	double transmittanceScale() const;
	bool twoSided() const;
	bool setChannelFlags(int v);
	bool setColorBleedScale(double v);
	bool setFinalGather(int v);
	bool setAdskMaterialFlags(int v);
	bool setGlobalIllumination(int v);
	bool setIlluminationModel(int v);
	bool setIndirectBumpScale(double v);
	bool setAnonymous(bool v);
	bool setLuminance(double v);
	bool setLuminanceMode(int v);
	bool setMode(int v);
	bool setName(const AcString& v);
	bool setDescription(const AcString& v);
	bool setReflectanceScale(double v);
	bool setReflectivity(double v);
	bool setSelfIllumination(double v);
	bool setShininess(double v);
	bool setTranslucence(double v);
	bool setTransmittanceScale(double v);
	bool setTwoSided(bool v);
};

void bind_PyDbMaterial(pym& m);
