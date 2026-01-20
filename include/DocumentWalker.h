#pragma once
#include "HtmlRenderer.h"

inline void ExtractText(OWPML::CObject* obj, std::wstring& out) {
    if (!obj) return;

    Rules::OnPreProcess(obj, out);
    Rules::OnProcess(obj, out);

    const int n = SDK::GetChildCountObj(obj);
    for (int i = 0; i < n; ++i) {
        ExtractText(SDK::GetChildAtObj(obj, i), out);
    }

    Rules::OnPostProcess(obj, out);
}
