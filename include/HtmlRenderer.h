#pragma once
#include "OwpmSDKPrelude.h"
#include "SDK_Wrapper.h"

namespace Rules {

    inline std::wstring EscapeHtml(const std::wstring& s) {
        std::wstring out;
        out.reserve(s.size());

        for (wchar_t c : s) {
            switch (c) {
            case L'&': out += L"&amp;"; break;
            case L'<': out += L"&lt;";  break;
            case L'>': out += L"&gt;";  break;
            case L'"': out += L"&quot;"; break;
            default:   out += c; break;
            }
        }
        return out;
    }

    inline std::wstring MapEngNameToTag(const std::wstring& engName) {
        if (engName == L"Outline 1") return L"h1";
        if (engName == L"Outline 2") return L"h2";
        if (engName == L"Outline 3") return L"h3";
        if (engName == L"Outline 4") return L"h4";
        if (engName == L"Outline 5") return L"h5";
        return L"p";
    }

    // 문단 들어갈 때 <p> or <h1> 오픈
    inline void OnPreProcess(OWPML::CObject* obj, std::wstring& out) {
        const unsigned int id = SDK::GetID(obj);

        if (id == ID_PARA_PType) {
            auto* para = (OWPML::CPType*)obj;

            const unsigned int styleID = SDK::GetParaStyleID(para);
            const std::wstring engName = SDK::GetStyleEngName(styleID);
            const std::wstring tag = MapEngNameToTag(engName);

            out += L"<" + tag + L" class=\"" + engName + L"\">";
        }
    }

    // CT에서 글자/줄바꿈 처리
    inline void OnProcess(OWPML::CObject* obj, std::wstring& out) {
        const unsigned int id = SDK::GetID(obj);

        if (id == ID_PARA_T) {
            auto* text = (OWPML::CT*)obj;

            const int cnt = SDK::GetChildCount(text);
            for (int i = 0; i < cnt; ++i) {
                auto* child = SDK::GetChildByIndex(text, i);
                if (!child) continue;

                const unsigned int cid = SDK::GetID(child);

                if (cid == ID_PARA_Char) {
                    auto* ch = (OWPML::CChar*)child;
                    out += EscapeHtml(SDK::GetCharValue(ch));
                }
                else if (cid == ID_PARA_LineBreak) {
                    out += L"<br/>";
                }
            }
        }
    }

    // 문단 끝나면 </p> 닫기
    inline void OnPostProcess(OWPML::CObject* obj, std::wstring& out) {
        const unsigned int id = SDK::GetID(obj);

        if (id == ID_PARA_PType) {
            auto* para = (OWPML::CPType*)obj;

            const unsigned int styleID = SDK::GetParaStyleID(para);
            const std::wstring engName = SDK::GetStyleEngName(styleID);
            const std::wstring tag = MapEngNameToTag(engName);

            out += L"</" + tag + L">\n";
        }
    }

} // namespace Rules
