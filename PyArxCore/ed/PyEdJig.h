// PyEdJig.h - AcEdJig trampoline + AcEdDragStyle value-class registration
// for the embedded PyEd module. JIG enums (DragStatus / UserInputControls /
// CursorType / StyleType) are registered centrally in the PyEnum module
// (PyEnum.cpp) and are NOT re-defined here.
//
// NOTE: drag()/acquire* drive the CAD command context and block for user
// input; they are bound but MUST NOT be called from self-tests.
#pragma once

#include "stdafx.h"
#include "dbjig.h"

// Trampoline: lets a Python class subclass AcEdJig and override the virtual
// sampler()/update()/entity() hooks. dimData()/setDimValue() (dynamic
// dimensions) keep the base implementation and are not exposed to Python.
class PyEdJig : public AcEdJig {
public:
    using AcEdJig::AcEdJig;

    AcEdJig::DragStatus sampler() override {
        PYBIND11_OVERRIDE(AcEdJig::DragStatus, AcEdJig, sampler);
    }

    Adesk::Boolean update() override {
        PYBIND11_OVERRIDE(Adesk::Boolean, AcEdJig, update);
    }

    AcDbEntity* entity() const override {
        PYBIND11_OVERRIDE(AcDbEntity*, AcEdJig, entity);
    }
};

// Registered from inside the existing PYBIND11_EMBEDDED_MODULE(PyEd) block.
void registerPyEdJig(py::module& m);
