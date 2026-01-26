#pragma once

#include <string>
#include <cstdint>

namespace OWPML {
    class CPType;
    class CT;
}

namespace SDK {
    struct ListInfo;
    enum class ListKind : std::uint8_t;
}

namespace Html {

    // ===========================
    // Cell mode + LineBreak policy
    // ===========================
    enum class CellBreakMode
    {
        Space,
        Newline,
        BrTag
    };

    void SetCellMode(bool on);
    bool IsCellMode();

    void SetCellBreakMode(CellBreakMode mode);
    void SetCellParagraphMode(CellBreakMode mode);

    // ===========================
    // List state machine
    // ===========================
    void EnsureListOpen(std::wstring& out, const SDK::ListInfo& info);
    void FlushList(std::wstring& out);
    void BeginListItemMode(const SDK::ListInfo& info);

    // Paragraph lifecycle
    void BeginParagraph(OWPML::CPType* para);
    void ProcessText(OWPML::CT* text);
    void ProcessLineSeg();
    void EndParagraph(std::wstring& out);

    // Document wrapper
    void BeginHtmlDocument(std::wstring& out);
    void EndHtmlDocument(std::wstring& out);

    // Style log
    void DumpStyleLogToConsole();

} // namespace Html
