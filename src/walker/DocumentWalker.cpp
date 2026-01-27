#include "walker/DocumentWalker.h"
#include "walker/WalkerConfig.h"
#include "walker/WalkerDebug.h"
#include "walker/TableRenderer.h"
#include "sdk/OwpmSDKPrelude.h"

#include <string>
#include <iostream>

#include "sdk/SDK_Wrapper.h"
#include "render/HtmlRenderer.h"

// forward decl
static void ExtractTextImpl(OWPML::CObject* object, std::wstring& out, int depth);

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

        WalkerDebug::ScanLogOnce(depth, child);

        const unsigned int id = SDK::GetID(child);

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
        case WalkerConfig::TABLE_ROOT_ID:
        {
            Html::FlushList(out);
            TableRenderer::RenderTableFromRoot373(child, out, depth, ExtractTextImpl);
            break;
        }

        case ID_PARA_PType:
        {
            auto* para = (OWPML::CPType*)child;

            // 리스트 판별 (셀 모드면 무시)
            SDK::ListInfo li;
            if (!Html::IsCellMode())
            {
                li = SDK::GetListInfoFromParagraph(para);
            }

            if (li.kind != SDK::ListKind::None && li.idRef != 0)
            {
                Html::EnsureListOpen(out, li);
                Html::BeginListItemMode(li);
            }
            else
            {
                Html::FlushList(out);
            }

            Html::BeginParagraph(para);
            ExtractTextImpl(child, out, depth + 1);
            Html::EndParagraph(out);
            break;
        }

        case ID_PARA_T:
        {
            Html::ProcessText((OWPML::CT*)child);
            break;
        }

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

void ExtractText(OWPML::CObject* object, std::wstring& out)
{
    ExtractTextImpl(object, out, 0);
    Html::FlushList(out); // 섹션 끝나면 정리
}
