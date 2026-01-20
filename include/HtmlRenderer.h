#pragma once

#include <string>
#include <cwctype>
#include <map>
#include <iostream>

#include "SDK_Wrapper.h"

namespace Html {

    // ===========================
    // Cell mode + LineBreak policy
    // ===========================
    enum class CellBreakMode
    {
        Space,    // " "
        Newline,  // "\n"  (HTML에서는 그대로는 안 보일 수 있음. <pre>나 CSS 필요)
        BrTag     // "<br/>"
    };

    inline bool& CellMode()
    {
        static bool cellMode = false;
        return cellMode;
    }

    inline CellBreakMode& CellBreakPolicy()
    {
        // 기본값: 셀 안에서도 줄바꿈을 "보이게" 하려면 BrTag가 가장 안정적
        static CellBreakMode mode = CellBreakMode::BrTag;
        return mode;
    }

    // 셀 안에서 "문단 경계"를 어떻게 처리할지
    // - BrTag: 문단 끝마다 <br/>
    // - Space: 문단 끝마다 공백
    // - Newline: 문단 끝마다 \n
    inline CellBreakMode& CellParagraphPolicy()
    {
        static CellBreakMode mode = CellBreakMode::BrTag;
        return mode;
    }

    inline bool& CellHasWrittenText()
    {
        static bool written = false;
        return written;
    }

    inline void SetCellMode(bool on)
    {
        CellMode() = on;
        if (on)
        {
            // td 들어갈 때 상태 초기화
            CellHasWrittenText() = false;
        }
    }

    inline bool IsCellMode()
    {
        return CellMode();
    }

    inline void SetCellBreakMode(CellBreakMode mode)
    {
        CellBreakPolicy() = mode;
    }

    inline void SetCellParagraphMode(CellBreakMode mode)
    {
        CellParagraphPolicy() = mode;
    }

    // ===========================
    // Helpers
    // ===========================
    inline void AppendBreak(std::wstring& buf, CellBreakMode mode)
    {
        switch (mode)
        {
        case CellBreakMode::Space:
            buf += L" ";
            break;
        case CellBreakMode::Newline:
            buf += L"\n";
            break;
        case CellBreakMode::BrTag:
        default:
            buf += L"<br/>";
            break;
        }
    }

    // Extract level from styles like "Outline 4" -> 4, otherwise 0
    inline int ExtractOutlineLevel(const std::wstring& engName)
    {
        const std::wstring prefix = L"Outline ";
        if (engName.rfind(prefix, 0) != 0) return 0;

        int level = 0;
        for (size_t i = prefix.size(); i < engName.size(); ++i)
        {
            if (!iswdigit(engName[i])) break;
            level = level * 10 + (engName[i] - L'0');
        }
        return level;
    }

    // Outline -> HTML tag mapping
    inline std::wstring MapEngNameToTag(const std::wstring& engName)
    {
        const int level = ExtractOutlineLevel(engName);

        // Outline 1~6 => h1~h6
        if (level >= 1 && level <= 6) {
            return L"h" + std::to_wstring(level);
        }

        // Outline 7~10 (or anything else) => paragraph
        return L"p";
    }

    // ===========================
    // Style log (optional)
    // ===========================
    inline std::map<std::wstring, int>& StyleSeenAll()
    {
        static std::map<std::wstring, int> m;
        return m;
    }

    inline std::map<std::wstring, int>& StyleSeenMapped()
    {
        static std::map<std::wstring, int> m;
        return m;
    }

    inline std::map<std::wstring, int>& StyleSeenUnmapped()
    {
        static std::map<std::wstring, int> m;
        return m;
    }

    inline bool IsMappedEngName(const std::wstring& engName)
    {
        if (engName.rfind(L"Outline ", 0) == 0) return true;
        if (engName == L"Normal") return true;
        if (engName == L"Body") return true;
        return false;
    }

    inline void LogParaStyle(const std::wstring& engName)
    {
        StyleSeenAll()[engName]++;

        if (IsMappedEngName(engName))
            StyleSeenMapped()[engName]++;
        else
            StyleSeenUnmapped()[engName]++;
    }

    inline void DumpStyleLogToConsole()
    {
        std::wcout << L"\n================== STYLE LOG SUMMARY ==================\n";
        std::wcout << L"[ALL] count=" << StyleSeenAll().size() << L"\n";
        std::wcout << L"[MAPPED] count=" << StyleSeenMapped().size() << L"\n";
        std::wcout << L"[UNMAPPED] count=" << StyleSeenUnmapped().size() << L"\n\n";

        std::wcout << L"------------------ UNMAPPED STYLES ------------------\n";
        for (auto& kv : StyleSeenUnmapped())
        {
            std::wcout << L"- " << kv.first << L" : " << kv.second << L"\n";
        }
        std::wcout << L"======================================================\n";
    }

    // class name normalize
    // Example: "Outline 4" -> "outline-4"
    inline std::wstring NormalizeClassName(const std::wstring& engName)
    {
        const int level = ExtractOutlineLevel(engName);
        if (level >= 1 && level <= 10) {
            return L"outline-" + std::to_wstring(level);
        }

        // Keep original for now (Normal, Body, etc.)
        return engName;
    }

    // Check if a string has any non-whitespace
    inline bool HasMeaningfulText(const std::wstring& s)
    {
        for (wchar_t ch : s) {
            if (!iswspace(ch)) return true;
        }
        return false;
    }

    // ===========================
    // Paragraph state (temporary)
    // ===========================
    inline bool& InPara()
    {
        static bool inPara = false;
        return inPara;
    }

    inline std::wstring& ParaTag()
    {
        static std::wstring tag;
        return tag;
    }

    inline std::wstring& ParaClass()
    {
        static std::wstring cls;
        return cls;
    }

    inline std::wstring& ParaBuffer()
    {
        static std::wstring buf;
        return buf;
    }

    // Called when entering a paragraph
    inline void BeginParagraph(OWPML::CPType* para)
    {
        if (!para) return;

        const unsigned int styleID = SDK::GetParaStyleID(para);
        const std::wstring engName = SDK::GetStyleEngName(styleID);

        LogParaStyle(engName);

        ParaTag() = MapEngNameToTag(engName);
        ParaClass() = NormalizeClassName(engName);

        InPara() = true;
        ParaBuffer().clear();
    }

    // Handle text run (CT) inside a paragraph
    inline void ProcessText(OWPML::CT* text)
    {
        if (!text || !InPara()) return;

        const int count = SDK::GetChildCount(text);
        for (int i = 0; i < count; ++i)
        {
            OWPML::CObject* child = SDK::GetChildByIndex(text, i);
            if (!child) continue;

            const unsigned int childID = SDK::GetID(child);

            if (childID == ID_PARA_Char)
            {
                ParaBuffer() += SDK::GetCharValue((OWPML::CChar*)child);
            }
            else if (childID == ID_PARA_LineBreak)
            {
                // 줄바꿈(명시적 LineBreak)은 셀 모드든 아니든 정책에 따라 처리
                if (!IsCellMode())
                {
                    ParaBuffer() += L"<br/>";
                }
                else
                {
                    AppendBreak(ParaBuffer(), CellBreakPolicy());
                }
            }
        }
    }

    // Some documents use LineSeg as a logical line boundary
    // 실제 문서에서 줄바꿈이 LineBreak가 아니라 LineSeg로 더 많이 나올 수 있음
    inline void ProcessLineSeg()
    {
        if (!InPara()) return;
    }

    // Called when leaving a paragraph
    inline void EndParagraph(std::wstring& out)
    {
        if (!InPara()) return;

        if (HasMeaningfulText(ParaBuffer()))
        {
            if (IsCellMode())
            {
                // 셀 내부: 문단 여러 개가 "붙어버리는 문제" 방지
                // - 이전 문단 텍스트가 이미 찍혔다면, 문단 경계 구분자를 먼저 넣어준다.
                if (CellHasWrittenText())
                {
                    std::wstring sep;
                    AppendBreak(sep, CellParagraphPolicy());
                    out += sep;
                }

                out += ParaBuffer();
                CellHasWrittenText() = true;
            }
            else
            {
                // 셀 밖: 기존대로 <p/h1..> 태그 생성
                out += L"<" + ParaTag() + L" class=\"" + ParaClass() + L"\">";
                out += ParaBuffer();
                out += L"</" + ParaTag() + L">\n";
            }
        }

        InPara() = false;
        ParaTag().clear();
        ParaClass().clear();
        ParaBuffer().clear();
    }


    inline void BeginHtmlDocument(std::wstring& out)
    {
        out += LR"(<!doctype html>
<html>
<head>
<meta charset="utf-8"/>
<style>
table { border-collapse: collapse; }
th, td {
    border: 1px solid black;
    padding: 6px 10px;
    text-align: left;
    vertical-align: top;
}
</style>
</head>
<body>
)";
    }

    inline void EndHtmlDocument(std::wstring& out)
    {
        out += LR"(
</body>
</html>
)";
    }


} // namespace Html
