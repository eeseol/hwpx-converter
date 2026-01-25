#include "walker/TableRenderer.h"
#include "walker/WalkerConfig.h"
#include "walker/WalkerDebug.h"
#include "walker/WalkerUtils.h"

#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>

#include "sdk/OwpmSDKPrelude.h"
#include "sdk/SDK_Wrapper.h"
#include "render/HtmlRenderer.h"

namespace
{
    struct CellPos
    {
        int r = 0;
        int c = 0;

        bool operator<(const CellPos& o) const
        {
            if (r != o.r) return r < o.r;
            return c < o.c;
        }
    };

    struct CellInfo
    {
        OWPML::CObject* wrapper = nullptr;
        OWPML::CObject* content = nullptr;
        int rowSpan = 1;
        int colSpan = 1;
    };

    static bool IsHtmlEffectivelyEmpty(const std::wstring& html)
    {
        // "<br/>" 같은 태그만 있는 경우를 empty로 보는 목적
        std::wstring s;
        s.reserve(html.size());

        for (size_t i = 0; i < html.size(); ++i)
        {
            wchar_t ch = html[i];
            if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n')
                continue;
            s.push_back(ch);
        }

        const std::wstring br = L"<br/>";
        size_t pos = 0;
        while ((pos = s.find(br, pos)) != std::wstring::npos)
        {
            s.erase(pos, br.size());
        }

        return s.empty();
    }
}

namespace TableRenderer
{
    void RenderTableFromRoot373(
        OWPML::CObject* tableRoot,
        std::wstring& out,
        int depth,
        RenderChildFn renderChild
    )
    {
        if (!tableRoot) return;
        if (!renderChild) return;

        const unsigned int rid = SDK::GetID(tableRoot);
        if (rid != WalkerConfig::TABLE_ROOT_ID) return;

        // (중첩표 안전) CellMode 저장/복원
        const bool prevCellMode = Html::IsCellMode();

        // 0) 덤프: 표 전체 subtree
        if (WalkerConfig::DUMP_MODE && WalkerConfig::DUMP_TABLE_SUBTREE)
        {
            std::wcout << L"\n========== TABLE SUBTREE DUMP START ==========\n";
            std::wcout << L"TABLE_ROOT_ID=" << WalkerConfig::TABLE_ROOT_ID << L"\n\n";
            WalkerDebug::DumpSubtree(tableRoot, depth, 0);
            std::wcout << L"========== TABLE SUBTREE DUMP END ==========\n\n";
        }

        // 1) rowGroups 모으기
        std::vector<OWPML::CObject*> rowGroups;
        WalkerUtils::CollectChildrenById(tableRoot, WalkerConfig::ROW_GROUP_ID, rowGroups);
        if (rowGroups.empty())
        {
            Html::SetCellMode(prevCellMode);
            return;
        }

        // 2) cellWrapper 수집 + (row,col) 맵 구성
        std::map<CellPos, CellInfo> cellMap;

        int inferredRowCount = 0;
        int inferredColCount = 0;

        for (size_t rg = 0; rg < rowGroups.size(); ++rg)
        {
            std::vector<OWPML::CObject*> wrappers;
            WalkerUtils::CollectChildrenById(rowGroups[rg], WalkerConfig::CELL_WRAPPER_ID, wrappers);

            for (auto* cellWrapper : wrappers)
            {
                if (!cellWrapper) continue;

                // addr 필수
                auto* addrObj = WalkerUtils::FindFirstChildById(cellWrapper, WalkerConfig::CELL_ADDR_ID);
                if (!addrObj) continue;

                auto* addr = static_cast<OWPML::CCellAddr*>(addrObj);
                const int r = (int)addr->GetRowAddr();
                const int c = (int)addr->GetColAddr();

                // span
                int colSpan = 1;
                int rowSpan = 1;
                if (auto* spanObj = WalkerUtils::FindFirstChildById(cellWrapper, WalkerConfig::CELL_SPAN_ID))
                {
                    auto* span = static_cast<OWPML::CCellSpan*>(spanObj);
                    colSpan = (int)span->GetColSpan();
                    rowSpan = (int)span->GetRowSpan();
                    if (colSpan <= 0) colSpan = 1;
                    if (rowSpan <= 0) rowSpan = 1;
                }

                // content
                auto* content = WalkerUtils::FindFirstChildById(cellWrapper, WalkerConfig::CELL_CONTENT_ID);

                // 디버그: cellWrapper child dump
                WalkerDebug::DumpCellWrapperChildren(cellWrapper, r, c, depth);

                // map 저장
                CellPos pos{ r, c };
                CellInfo info;
                info.wrapper = cellWrapper;
                info.content = content;
                info.colSpan = colSpan;
                info.rowSpan = rowSpan;

                cellMap[pos] = info;

                inferredRowCount = std::max(inferredRowCount, r + rowSpan);
                inferredColCount = std::max(inferredColCount, c + colSpan);
            }
        }

        if (cellMap.empty())
        {
            Html::SetCellMode(prevCellMode);
            return;
        }

        // 3) rowCount / colCount 확정
        const int rowCount = std::max((int)rowGroups.size(), inferredRowCount);
        const int colCount = std::max(1, inferredColCount);

        if (WalkerConfig::TABLE_LOG)
        {
            std::wcout << L"\n[TABLE] ===== Render Start (GRID) =====\n";
            std::wcout << L"[TABLE] depth=" << depth
                << L" rowGroups=" << (int)rowGroups.size()
                << L" inferredRows=" << inferredRowCount
                << L" inferredCols=" << inferredColCount
                << L" => rows=" << rowCount
                << L" cols=" << colCount
                << L"\n";
            std::wcout << L"[TABLE] ================================\n\n";
        }

        // 4) occupancy grid (테이블 루트 로컬)
        std::vector<std::vector<bool>> occupied(rowCount, std::vector<bool>(colCount, false));

        // 5) HTML 출력
        out += L"<table>\n";

        for (int r = 0; r < rowCount; ++r)
        {
            out += L"<tr>\n";

            for (int c = 0; c < colCount; ++c)
            {
                // covered 스킵
                if (occupied[r][c])
                {
                    continue;
                }

                const CellPos pos{ r, c };
                auto it = cellMap.find(pos);

                if (it != cellMap.end())
                {
                    const CellInfo& cell = it->second;

                    // span attribute 구성
                    std::wstring tdOpen = L"<td";
                    if (cell.colSpan > 1)
                    {
                        tdOpen += L" colspan=\"";
                        tdOpen += std::to_wstring(cell.colSpan);
                        tdOpen += L"\"";
                    }
                    if (cell.rowSpan > 1)
                    {
                        tdOpen += L" rowspan=\"";
                        tdOpen += std::to_wstring(cell.rowSpan);
                        tdOpen += L"\"";
                    }

                    // span 점유 마킹
                    for (int rr = r; rr < r + cell.rowSpan && rr < rowCount; ++rr)
                    {
                        for (int cc = c; cc < c + cell.colSpan && cc < colCount; ++cc)
                        {
                            occupied[rr][cc] = true;
                        }
                    }

                    // 셀 내부 렌더링(임시 버퍼)
                    std::wstring cellBuf;
                    cellBuf.reserve(256);

                    Html::SetCellMode(true);

                    if (cell.content)
                        renderChild(cell.content, cellBuf, depth + 1);
                    else if (cell.wrapper)
                        renderChild(cell.wrapper, cellBuf, depth + 1);

                    Html::SetCellMode(false);

                    const bool isEmpty = IsHtmlEffectivelyEmpty(cellBuf);

                    if (WalkerConfig::TAG_EMPTY_TD && isEmpty)
                    {
                        tdOpen += L" data-hwpx-empty=\"1\"";
                    }

                    tdOpen += L">";
                    out += tdOpen;

                    out += cellBuf;

                    out += L"</td>\n";
                }
                else
                {
                    // hole: wrapper도 없고 occupied도 아닌 진짜 빈 칸
                    if (WalkerConfig::EMIT_EMPTY_TD_FOR_HOLES)
                    {
                        if (WalkerConfig::TAG_EMPTY_TD)
                            out += L"<td data-hwpx-empty=\"1\"></td>\n";
                        else
                            out += L"<td></td>\n";
                    }
                }
            }

            out += L"</tr>\n";
        }

        out += L"</table>\n";

        // CellMode 복원
        Html::SetCellMode(prevCellMode);
    }
}
