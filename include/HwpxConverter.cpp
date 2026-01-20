#define DEBUG_PARA_LOG 0

#include "OwpmSDKPrelude.h"
#include "HwpxConverter.h"

#include "DocumentWalker.h"
#include "SDK_Wrapper.h"

// 스타일 접근용
#include "OWPML/Class/Head/HWPMLHeadType.h"
#include "OWPML/Class/Head/MappingTableType.h"
#include "OWPML/Class/Head/styles.h"

static bool WriteUtf8File(const std::wstring& path, const std::wstring& content) {
    // wchar -> utf8
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
    const std::wstring& inputPath,
    const std::wstring& outputPath,
    const ConvertOptions& opt
) {
    if (!opt.outputHtml) {
        // 지금은 HTML만 지원
        return false;
    }

    // 1) 문서 열기
    OWPML::COwpmlDocumnet* doc = OWPML::COwpmlDocumnet::OpenDocument(inputPath.c_str());
    if (!doc) return false;

    // 2) 스타일 맵 초기화 (head -> refList -> styles)
    auto* head = doc->GetHead();
    if (head) {
        auto* refList = head->GetrefList();
        if (refList) {
            auto* styles = refList->Getstyles();
            if (styles) {
                SDK::InitStyleMap(styles);
            }
        }
    }

    // 3) 변환 시작
    std::wstring out;
    auto* sections = doc->GetSections();
    if (sections) {
        for (auto* sec : *sections) {
            ExtractText(sec, out);
        }
    }

    //PARA 디버깅 코드
    #if DEBUG_PARA_LOG

    Html::DumpStyleLogToConsole();

    #endif // DEBUG_PARA_LOG


    // HTML 문서 래핑
    std::wstring html;
    Html::BeginHtmlDocument(html);   // 여기서 head + body 시작까지
    html += out;                     // 본문(테이블/문단들)
    Html::EndHtmlDocument(html);   // body/html 닫기


    // 4) 저장
    const bool ok = WriteUtf8File(outputPath, html);

    delete doc;
    return ok;
}
