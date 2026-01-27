#define DEBUG_PARA_LOG 0

#include "sdk/OwpmSDKPrelude.h"
#include "app/HwpxConverter.h"

#include "render/HtmlRenderer.h"
#include "walker/DocumentWalker.h"
#include "sdk/SDK_Wrapper.h"

#include <fstream>
#include <string>
#include <Windows.h>
#include <algorithm>
#include <cwctype>

static std::wstring Trim(const std::wstring& s)
{
    size_t b = 0;
    while (b < s.size() && iswspace(s[b])) b++;
    size_t e = s.size();
    while (e > b && iswspace(s[e - 1])) e--;
    return s.substr(b, e - b);
}

static std::wstring StripQuotes(std::wstring s)
{
    s = Trim(s);
    if (s.size() >= 2 && ((s.front() == L'"' && s.back() == L'"') || (s.front() == L'\'' && s.back() == L'\'')))
        return s.substr(1, s.size() - 2);
    return s;
}

static bool EndsWithCaseInsensitive(const std::wstring& s, const std::wstring& suffix)
{
    if (s.size() < suffix.size()) return false;

    const size_t start = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i)
    {
        wchar_t a = towlower(s[start + i]);
        wchar_t b = towlower(suffix[i]);
        if (a != b) return false;
    }
    return true;
}

static bool IsHwpxPath(std::wstring path)
{
    path = StripQuotes(path);
    return EndsWithCaseInsensitive(path, L".hwpx");
}

static bool WriteUtf8File(const std::wstring& path, const std::wstring& content)
{
    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0,
        content.c_str(), (int)content.size(),
        nullptr, 0,
        nullptr, nullptr
    );
    if (sizeNeeded <= 0) return false;

    std::string utf8(sizeNeeded, '\0');
    WideCharToMultiByte(
        CP_UTF8, 0,
        content.c_str(), (int)content.size(),
        utf8.data(), sizeNeeded,
        nullptr, nullptr
    );

    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open()) return false;

    f.write(utf8.data(), (std::streamsize)utf8.size());
    f.close();
    return true;
}

bool ConvertHwpxToHtml(
    const std::wstring& inputPathRaw,
    const std::wstring& outputPathRaw,
    const ConvertOptions& opt
)
{
    if (!opt.outputHtml) return false;

    const std::wstring inputPath = StripQuotes(inputPathRaw);
    const std::wstring outputPath = StripQuotes(outputPathRaw);

    // 방어적 체크(엔트리 포인트가 main이 아니어도 안전)
    if (!IsHwpxPath(inputPath)) return false;

    OWPML::COwpmlDocumnet* doc = OWPML::COwpmlDocumnet::OpenDocument(inputPath.c_str());
    if (!doc) return false;

    // ===== Head(refList) 초기화 =====
    auto* head = doc->GetHead();
    if (head) {
        auto* refList = head->GetrefList();
        if (refList) {
            // 1) 스타일
            if (auto* styles = refList->Getstyles()) {
                SDK::InitStyleMap(styles);
            }

            // 2) 리스트 관련
            if (auto* numberings = refList->Getnumberings()) {
                SDK::InitNumberings(numberings);
            }

            if (auto* bullets = refList->Getbullets()) {
                SDK::InitBullets(bullets);
            }

            if (auto* paraProps = refList->GetparaProperties()) {
                SDK::InitParaProperties(paraProps);
            }
        }
    }

    // ===== 변환 시작 =====
    std::wstring out;
    auto* sections = doc->GetSections();
    if (sections) {
        for (auto* sec : *sections) {
            ExtractText(sec, out);
        }
    }

#if DEBUG_PARA_LOG
    Html::DumpStyleLogToConsole();
#endif

    Html::FlushList(out);

    std::wstring html;
    Html::BeginHtmlDocument(html);
    html += out;
    Html::EndHtmlDocument(html);

    const bool ok = WriteUtf8File(outputPath, html);

    delete doc;
    return ok;
}
