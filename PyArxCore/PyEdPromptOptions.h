// PyEdPromptOptions.h - PromptOptions value class for the interactive pyed*
// helper family. Pure inline header (trivial setters); no .cpp required.
//
// Moved verbatim from PyEdBinder.h during the PyEdFuncs split; no logic changed.
#pragma once

#include "stdafx.h"
#include "PyEdResult.h"

class PromptOptions {
public:
    OptionFlags Flags = OptionFlags::None;
    AcString Message;
    AcString KeyWords;
    PromptOptions() = default;
    void setMessage(const AcString& m) { Message = AcString(L"\n") + m; }
    void setFlag(OptionFlags f) {
        Flags = static_cast<OptionFlags>(static_cast<int>(Flags) | static_cast<int>(f));
    }
};
