#define DEBUG_PARA_LOG 0

#include "sdk/OwpmSDKPrelude.h"
#include "app/HwpxConverter.h"

#include "render/HtmlRenderer.h"
#include "walker/DocumentWalker.h"
#include "sdk/SDK_Wrapper.h"

#include <fstream>
#include <string>
#include <Windows.h>

static bool WriteUtf8File(const std::wstring& path, const std::wstring& content)
{
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
)
{
    if (!opt.outputHtml) return false;

    // 1) 문서 열기
    OWPML::COwpmlDocumnet* doc = OWPML::COwpmlDocumnet::OpenDocument(inputPath.c_str());
    if (!doc) return false;

    // 2) Head(refList) 초기화
    auto* head = doc->GetHead();
    if (head)
    {
        auto* refList = head->GetrefList();
        if (refList)
        {
            // (1) styles
            if (auto* styles = refList->Getstyles())
            {
                SDK::InitStyleMap(styles);
            }

            // (2) list meta
            // 중요: numberings/bullets 먼저 -> paraProperties가 heading idRef를 보고 kind 추정
            if (auto* numberings = refList->Getnumberings())
            {
                SDK::InitNumberings(numberings);
            }

            if (auto* bullets = refList->Getbullets())
            {
                SDK::InitBullets(bullets);
            }

            if (auto* paraProps = refList->GetparaProperties())
            {
                SDK::InitParaProperties(paraProps);
            }
        }
    }

    // 3) 변환 시작
    std::wstring out;
    auto* sections = doc->GetSections();
    if (sections)
    {
        for (auto* sec : *sections)
        {
            ExtractText(sec, out);
        }
    }

#if DEBUG_PARA_LOG
    Html::DumpStyleLogToConsole();
#endif

    // 문서 끝에서 열려있을 수 있는 리스트 닫기
    Html::FlushList(out);

    // 4) HTML 문서 래핑
    std::wstring html;
    Html::BeginHtmlDocument(html);
    html += out;
    Html::EndHtmlDocument(html);

    // 5) 저장
    const bool ok = WriteUtf8File(outputPath, html);

    delete doc;
    return ok;
}
