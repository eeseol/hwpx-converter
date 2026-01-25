#include "walker/DocumentWalker.h"
#include "walker/WalkerConfig.h"
#include "walker/WalkerDebug.h"
#include "walker/TableRenderer.h"
#include "sdk/OwpmSDKPrelude.h"

#include <string>
#include <iostream>

#include "sdk/SDK_Wrapper.h"
#include "render/HtmlRenderer.h"

// forward decl: TableRenderer에서 콜백으로 쓰려고 필요
static void ExtractTextImpl(OWPML::CObject* object, std::wstring& out, int depth);

// =====================
// Main recursive walker
// =====================
static void ExtractTextImpl(OWPML::CObject* object, std::wstring& out, int depth)
{
    if (!object) return;

    if (depth > WalkerConfig::MAX_DEPTH)
    {
        std::wcout << L"[WARN] Max depth exceeded. Stop recursion.\n";
        return;
    }

    OWPML::Objectlist* childList = object->GetObjectList();
    if (!childList) return;

    for (OWPML::CObject* child : *childList)
    {
        if (!child) continue;

        // SCAN MODE (처음 발견하는 ID 로그)
        WalkerDebug::ScanLogOnce(depth, child);

        const unsigned int id = SDK::GetID(child);

        // 특정 ID 서브트리 덤프(원하면)
        if (WalkerConfig::DUMP_MODE && WalkerConfig::DUMP_TARGET_SUBTREE && id == WalkerConfig::TARGET_ID)
        {
            std::wcout << L"\n========== TARGET SUBTREE DUMP START ==========\n";
            std::wcout << L"TARGET_ID=" << WalkerConfig::TARGET_ID << L"\n\n";
            WalkerDebug::DumpSubtree(child, depth, 0);
            std::wcout << L"========== TARGET SUBTREE DUMP END ==========\n\n";
            return;
        }

        switch (id)
        {
            // Table 전용 렌더러로 처리
        case WalkerConfig::TABLE_ROOT_ID:
        {
            TableRenderer::RenderTableFromRoot373(child, out, depth, ExtractTextImpl);
            break;
        }

        // Paragraph
        case ID_PARA_PType:
        {
            Html::BeginParagraph((OWPML::CPType*)child);
            ExtractTextImpl(child, out, depth + 1);
            Html::EndParagraph(out);
            break;
        }

        // TextRun
        case ID_PARA_T:
        {
            Html::ProcessText((OWPML::CT*)child);
            break;
        }

        // LineSeg
        case ID_PARA_LineSeg:
        {
            Html::ProcessLineSeg();
            break;
        }

        default:
            ExtractTextImpl(child, out, depth + 1);
            break;
        }
    }
}

// 기존 외부 인터페이스 유지
void ExtractText(OWPML::CObject* object, std::wstring& out)
{
    ExtractTextImpl(object, out, 0);
}
