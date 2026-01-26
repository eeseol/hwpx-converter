#include "render/HtmlRenderer.h"

#include <cwctype>
#include <map>
#include <iostream>

#include "sdk/SDK_Wrapper.h"
#include "sdk/OwpmSDKPrelude.h"

namespace Html {

    // ===========================
    // Cell mode + LineBreak policy
    // ===========================
    static bool& CellMode()
    {
        static bool cellMode = false;
        return cellMode;
    }

    static CellBreakMode& CellBreakPolicy()
    {
        static CellBreakMode mode = CellBreakMode::BrTag;
        return mode;
    }

    static CellBreakMode& CellParagraphPolicy()
    {
        static CellBreakMode mode = CellBreakMode::BrTag;
        return mode;
    }

    static bool& CellHasWrittenText()
    {
        static bool written = false;
        return written;
    }

    void SetCellMode(bool on)
    {
        CellMode() = on;
        if (on) CellHasWrittenText() = false;
    }

    bool IsCellMode()
    {
        return CellMode();
    }

    void SetCellBreakMode(CellBreakMode mode)
    {
        CellBreakPolicy() = mode;
    }

    void SetCellParagraphMode(CellBreakMode mode)
    {
        CellParagraphPolicy() = mode;
    }

    // ===========================
    // Helpers
    // ===========================
    static void AppendBreak(std::wstring& buf, CellBreakMode mode)
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

    static bool HasMeaningfulText(const std::wstring& s)
    {
        for (wchar_t ch : s)
        {
            if (!iswspace(ch)) return true;
        }
        return false;
    }

    // ===========================
    // Outline style mapping
    // ===========================
    static int ExtractOutlineLevel(const std::wstring& engName)
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

    static std::wstring MapEngNameToTag(const std::wstring& engName)
    {
        const int level = ExtractOutlineLevel(engName);
        if (level >= 1 && level <= 6)
            return L"h" + std::to_wstring(level);

        return L"p";
    }

    static std::wstring NormalizeClassName(const std::wstring& engName)
    {
        const int level = ExtractOutlineLevel(engName);
        if (level >= 1 && level <= 10)
            return L"outline-" + std::to_wstring(level);

        return engName;
    }

    // ===========================
    // Style log (optional)
    // ===========================
    static std::map<std::wstring, int>& StyleSeenAll()
    {
        static std::map<std::wstring, int> m;
        return m;
    }

    static std::map<std::wstring, int>& StyleSeenMapped()
    {
        static std::map<std::wstring, int> m;
        return m;
    }

    static std::map<std::wstring, int>& StyleSeenUnmapped()
    {
        static std::map<std::wstring, int> m;
        return m;
    }

    static bool IsMappedEngName(const std::wstring& engName)
    {
        if (engName.rfind(L"Outline ", 0) == 0) return true;
        if (engName == L"Normal") return true;
        if (engName == L"Body") return true;
        return false;
    }

    static void LogParaStyle(const std::wstring& engName)
    {
        StyleSeenAll()[engName]++;
        if (IsMappedEngName(engName))
            StyleSeenMapped()[engName]++;
        else
            StyleSeenUnmapped()[engName]++;
    }

    void DumpStyleLogToConsole()
    {
        std::wcout << L"\n================== STYLE LOG SUMMARY ==================\n";
        std::wcout << L"[ALL] count=" << StyleSeenAll().size() << L"\n";
        std::wcout << L"[MAPPED] count=" << StyleSeenMapped().size() << L"\n";
        std::wcout << L"[UNMAPPED] count=" << StyleSeenUnmapped().size() << L"\n\n";

        std::wcout << L"------------------ UNMAPPED STYLES ------------------\n";
        for (auto& kv : StyleSeenUnmapped())
            std::wcout << L"- " << kv.first << L" : " << kv.second << L"\n";

        std::wcout << L"======================================================\n";
    }

    // ===========================
    // Paragraph state
    // ===========================
    static bool& InPara()
    {
        static bool inPara = false;
        return inPara;
    }

    static std::wstring& ParaTag()
    {
        static std::wstring tag;
        return tag;
    }

    static std::wstring& ParaClass()
    {
        static std::wstring cls;
        return cls;
    }

    static std::wstring& ParaBuffer()
    {
        static std::wstring buf;
        return buf;
    }

    // ===========================
    // List state machine
    // ===========================
    static bool& InList()
    {
        static bool inList = false;
        return inList;
    }

    static SDK::ListKind& CurListKind()
    {
        static SDK::ListKind k = SDK::ListKind::None;
        return k;
    }

    static std::uint32_t& CurListIdRef()
    {
        static std::uint32_t v = 0;
        return v;
    }

    static bool& ParaIsListItem()
    {
        static bool v = false;
        return v;
    }

    void EnsureListOpen(std::wstring& out, const SDK::ListInfo& info)
    {
        if (IsCellMode()) return;                 // 테이블 셀은 일단 무시
        if (info.kind == SDK::ListKind::None) return;
        if (info.idRef == 0) return;              // ★ 핵심: idRef==0이면 열지 않음

        // 다른 리스트로 바뀌면 닫고 다시 열기
        if (InList())
        {
            if (CurListKind() != info.kind || CurListIdRef() != info.idRef)
            {
                out += L"</ol>\n";
                InList() = false;
                CurListKind() = SDK::ListKind::None;
                CurListIdRef() = 0;
            }
        }

        if (!InList())
        {
            // 정책: numbering/bullet 상관없이 ol로만 열고 CSS로 점 처리
            out += L"<ol class=\"hwpx-ol-dot\">\n";
            InList() = true;
            CurListKind() = info.kind;
            CurListIdRef() = info.idRef;
        }
    }

    void FlushList(std::wstring& out)
    {
        if (!InList()) return;
        out += L"</ol>\n";
        InList() = false;
        CurListKind() = SDK::ListKind::None;
        CurListIdRef() = 0;
    }

    void BeginListItemMode(const SDK::ListInfo& info)
    {
        if (IsCellMode()) { ParaIsListItem() = false; return; }
        if (info.kind == SDK::ListKind::None) { ParaIsListItem() = false; return; }
        if (info.idRef == 0) { ParaIsListItem() = false; return; } // ★ 핵심
        ParaIsListItem() = true;
    }

    // ===========================
    // Paragraph lifecycle
    // ===========================
    void BeginParagraph(OWPML::CPType* para)
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

    void ProcessText(OWPML::CT* text)
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

    void ProcessLineSeg()
    {
        if (!InPara()) return;
    }

    void EndParagraph(std::wstring& out)
    {
        if (!InPara()) return;

        const bool hasText = HasMeaningfulText(ParaBuffer());

        if (hasText)
        {
            if (IsCellMode())
            {
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
                // ★ 리스트 문단이면 li로만 출력
                if (ParaIsListItem())
                {
                    out += L"<li>";
                    out += ParaBuffer();
                    out += L"</li>\n";
                }
                else
                {
                    out += L"<" + ParaTag() + L" class=\"" + ParaClass() + L"\">";
                    out += ParaBuffer();
                    out += L"</" + ParaTag() + L">\n";
                }
            }
        }
        // ★ 빈 문단이면 아무 것도 출력하지 않음 (빈 li 방지)

        InPara() = false;
        ParaIsListItem() = false;

        ParaTag().clear();
        ParaClass().clear();
        ParaBuffer().clear();
    }

    void BeginHtmlDocument(std::wstring& out)
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

/* 핵심 정책:
   - numbering/bullet 모두 <ol>로 열되
   - 화면은 ul처럼 점만 보이게(번호 계산 X)
*/
.hwpx-ol-dot {
    list-style-type: disc;
    margin: 0;
    padding-left: 1.4em;
}
.hwpx-ol-dot > li {
    margin: 0.2em 0;
}
</style>
</head>
<body>
)";
    }

    void EndHtmlDocument(std::wstring& out)
    {
        out += LR"(
</body>
</html>
)";
    }

} // namespace Html
