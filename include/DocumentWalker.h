// DocumentWalker.h (UPDATED)
// - span 적용 + covered 스킵 + (필요시) empty 셀 표시
// - 점유표(occupancy grid)는 "테이블 루트 단위 로컬"로만 유지 (중첩표 안전)
// - CellMode 저장/복원 포함
//
// NOTE:
// 1) 기존 maxColCount 기반 렌더링은 cellAddr 기반 그리드 렌더링으로 교체됨
// 2) covered(병합 덮임) 칸은 td를 생성하지 않음(정석)
// 3) 빈칸 셀은 SDK가 wrapper를 내려주는 편이라(네 로그 기준) 대부분 그대로 <td></td>로 처리됨
//    다만 혹시 wrapper 자체가 없는 "진짜 empty hole"이 있으면 <td data-hwpx-empty="1"></td>로 보정함.

#pragma once

#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <algorithm>

#include "SDK_Wrapper.h"
#include "HtmlRenderer.h"

#include "OWPML/Class/Para/tc.h"
#include "OWPML/Class/Para/cellSpan.h"
#include "OWPML/Class/Para/cellAddr.h"

// forward decl
inline void ExtractTextImpl(OWPML::CObject* object, std::wstring& out, int depth);

// =====================
// Config & Utilities
// =====================
namespace WalkConfig
{
    // ====== 개발/디버그 토글 ======
    static constexpr bool SCAN_MODE = false;
    static constexpr bool DUMP_MODE = false;
    static constexpr bool TABLE_LOG = false;

    // ====== Dump 옵션 ======
    static constexpr bool DUMP_TABLE_SUBTREE = true;
    static constexpr bool DUMP_CELL_WRAPPER_CHILDREN = true;

    // (선택) 특정 ID 서브트리 덤프하고 싶으면 사용
    static constexpr bool DUMP_TARGET_SUBTREE = false;
    static constexpr unsigned int TARGET_ID = 0;

    // ====== 표 ID (네 로그로 확정된 값들) ======
    static constexpr unsigned int TABLE_ROOT_ID = 805306373;     // table root
    static constexpr unsigned int ROW_GROUP_ID = 805306463;      // row group
    static constexpr unsigned int CELL_WRAPPER_ID = 805306465;   // cell wrapper
    static constexpr unsigned int CELL_CONTENT_ID = 805306406;   // cell content
    static constexpr unsigned int CELL_SPAN_ID = 805306467;      // cellSpan
    static constexpr unsigned int CELL_ADDR_ID = 805306466;      // cellAddr
    static constexpr unsigned int CELL_SZ_ID = 805306468;        // cellSz
    static constexpr unsigned int CELL_MARGIN_ID = 805306469;    // cellMargin

    // ====== dump depth 제한 ======
    static constexpr int DUMP_MAX_REL_DEPTH = 20;

    // ====== 안전장치 ======
    static constexpr int MAX_DEPTH = 5000;

    inline std::map<unsigned int, int>& SeenIdOnce()
    {
        static std::map<unsigned int, int> m;
        return m;
    }

    inline int CountChildrenByObjectList(OWPML::CObject* obj)
    {
        if (!obj) return 0;
        int childCount = 0;
        if (auto list = obj->GetObjectList())
        {
            for (auto* _ : *list) { (void)_; childCount++; }
        }
        return childCount;
    }

    inline void ScanLogOnce(int depth, OWPML::CObject* obj)
    {
        if (!SCAN_MODE || !obj) return;

        const unsigned int id = SDK::GetID(obj);
        int& cnt = SeenIdOnce()[id];
        cnt++;

        if (cnt == 1)
        {
            const int childCount = CountChildrenByObjectList(obj);
            std::wcout
                << L"[SCAN] depth=" << depth
                << L" id=" << id
                << L" childCount=" << childCount
                << L"\n";
        }
    }

    inline const wchar_t* KnownIdName(unsigned int id)
    {
        switch (id)
        {
        case ID_PARA_PType:      return L"ID_PARA_PType(Paragraph)";
        case ID_PARA_T:          return L"ID_PARA_T(TextRun)";
        case ID_PARA_Char:       return L"ID_PARA_Char(Char)";
        case ID_PARA_LineSeg:    return L"ID_PARA_LineSeg(LineSeg)";
        case ID_PARA_LineBreak:  return L"ID_PARA_LineBreak(LineBreak)";
        default:                 return L"";
        }
    }

    inline void DumpSubtree(OWPML::CObject* root, int absDepth, int relDepth)
    {
        if (!root) return;
        if (relDepth > DUMP_MAX_REL_DEPTH) return;

        const unsigned int id = SDK::GetID(root);
        const int childCount = CountChildrenByObjectList(root);
        const wchar_t* name = KnownIdName(id);

        for (int i = 0; i < relDepth; ++i) std::wcout << L"  ";

        std::wcout
            << L"[DUMP] absDepth=" << absDepth
            << L" rel=" << relDepth
            << L" id=" << id
            << L" childCount=" << childCount;

        if (name && name[0] != 0)
            std::wcout << L"  (" << name << L")";

        std::wcout << L"\n";

        auto list = root->GetObjectList();
        if (!list) return;

        for (auto* child : *list)
        {
            if (!child) continue;
            DumpSubtree(child, absDepth + 1, relDepth + 1);
        }
    }

    inline OWPML::CObject* FindFirstChildById(OWPML::CObject* parent, unsigned int targetId)
    {
        if (!parent) return nullptr;
        auto list = parent->GetObjectList();
        if (!list) return nullptr;

        for (auto* ch : *list)
        {
            if (!ch) continue;
            if (SDK::GetID(ch) == targetId) return ch;
        }
        return nullptr;
    }

    inline void CollectChildrenById(OWPML::CObject* parent, unsigned int targetId, std::vector<OWPML::CObject*>& outVec)
    {
        outVec.clear();
        if (!parent) return;

        auto list = parent->GetObjectList();
        if (!list) return;

        for (auto* ch : *list)
        {
            if (!ch) continue;
            if (SDK::GetID(ch) == targetId)
                outVec.push_back(ch);
        }
    }

    // ===========================
    // cellWrapper 내부 child ID dump + addr/span 확정 로그
    // ===========================
    inline void DumpCellWrapperChildren(OWPML::CObject* cellWrapper, int r, int c, int depth)
    {
        if (!DUMP_MODE || !DUMP_CELL_WRAPPER_CHILDREN) return;
        if (!cellWrapper) return;

        auto list = cellWrapper->GetObjectList();
        if (!list)
        {
            std::wcout << L"[CELL_WRAPPER] r=" << r << L" c=" << c << L" (no child list)\n";
            return;
        }

        const unsigned int wid = SDK::GetID(cellWrapper);
        const int childCount = CountChildrenByObjectList(cellWrapper);

        std::wcout
            << L"\n[CELL_WRAPPER] depth=" << depth
            << L" r=" << r
            << L" c=" << c
            << L" wrapperId=" << wid
            << L" childCount=" << childCount
            << L"\n";

        int idx = 0;
        for (auto* ch : *list)
        {
            if (!ch) continue;

            const unsigned int cid = SDK::GetID(ch);

            std::wcout
                << L"  - idx=" << idx++
                << L" id=" << cid;

            if (cid == CELL_CONTENT_ID)  std::wcout << L" (cellContent)";
            if (cid == CELL_ADDR_ID)     std::wcout << L" (cellAddr)";
            if (cid == CELL_SPAN_ID)     std::wcout << L" (cellSpan)";
            if (cid == CELL_SZ_ID)       std::wcout << L" (cellSz)";
            if (cid == CELL_MARGIN_ID)   std::wcout << L" (cellMargin)";

            std::wcout << L"\n";

            if (cid == CELL_ADDR_ID)
            {
                auto* addr = static_cast<OWPML::CCellAddr*>(ch);
                std::wcout
                    << L"    -> CCellAddr CONFIRMED! row="
                    << addr->GetRowAddr()
                    << L", col="
                    << addr->GetColAddr()
                    << L"\n";
            }

            if (cid == CELL_SPAN_ID)
            {
                auto* span = static_cast<OWPML::CCellSpan*>(ch);
                std::wcout
                    << L"    -> CCellSpan CONFIRMED! colSpan="
                    << span->GetColSpan()
                    << L", rowSpan="
                    << span->GetRowSpan()
                    << L"\n";
            }
        }

        std::wcout << L"[CELL_WRAPPER] END\n";
        std::wcout.flush();
    }
}

// =====================
// Helpers for table rendering
// =====================
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

inline bool IsHtmlEffectivelyEmpty(const std::wstring& html)
{
    // "<br/>" 같은 태그만 있는 경우를 empty로 보려는 목적
    std::wstring s;
    s.reserve(html.size());

    // 태그 제거는 과하니, 최소한의 관용 처리:
    // - 공백/개행 제거
    // - "<br/>" 텍스트 제거 후 남는 게 있는지 확인
    for (size_t i = 0; i < html.size(); ++i)
    {
        wchar_t ch = html[i];
        if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n')
            continue;
        s.push_back(ch);
    }

    // "<br/>" 제거(여러 번)
    const std::wstring br = L"<br/>";
    size_t pos = 0;
    while ((pos = s.find(br, pos)) != std::wstring::npos)
    {
        s.erase(pos, br.size());
    }

    // 남은 문자가 있으면 empty 아님
    return s.empty();
}

// =====================
// Table renderer (NEW)
// - TABLE_ROOT(373) -> ROW_GROUP(463) -> CELL_WRAPPER(465)
// - cellAddr 기반으로 (row,col)에 anchor 배치
// - cellSpan 기반으로 occupied 마킹
// - covered는 td 스킵
// - wrapper가 없는 비covered 칸만 예외적으로 empty td 생성
// =====================
inline void RenderTableFromRoot373(OWPML::CObject* tableRoot, std::wstring& out, int depth)
{
    if (!tableRoot) return;

    const unsigned int rid = SDK::GetID(tableRoot);
    if (rid != WalkConfig::TABLE_ROOT_ID) return;

    // (중첩표 안전) CellMode 저장/복원
    const bool prevCellMode = Html::IsCellMode();

    // 0) 덤프: 표 전체 subtree
    if (WalkConfig::DUMP_MODE && WalkConfig::DUMP_TABLE_SUBTREE)
    {
        std::wcout << L"\n========== TABLE SUBTREE DUMP START ==========\n";
        std::wcout << L"TABLE_ROOT_ID=" << WalkConfig::TABLE_ROOT_ID << L"\n\n";
        WalkConfig::DumpSubtree(tableRoot, depth, 0);
        std::wcout << L"========== TABLE SUBTREE DUMP END ==========\n\n";
    }

    // 1) rowGroups 모으기
    std::vector<OWPML::CObject*> rowGroups;
    WalkConfig::CollectChildrenById(tableRoot, WalkConfig::ROW_GROUP_ID, rowGroups);
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
        WalkConfig::CollectChildrenById(rowGroups[rg], WalkConfig::CELL_WRAPPER_ID, wrappers);

        for (auto* cellWrapper : wrappers)
        {
            if (!cellWrapper) continue;

            // addr 필수 (없으면 이 방식으로 표 렌더가 어려움)
            auto* addrObj = WalkConfig::FindFirstChildById(cellWrapper, WalkConfig::CELL_ADDR_ID);
            if (!addrObj) continue;

            auto* addr = static_cast<OWPML::CCellAddr*>(addrObj);
            const int r = (int)addr->GetRowAddr();
            const int c = (int)addr->GetColAddr();

            // span
            int colSpan = 1;
            int rowSpan = 1;
            if (auto* spanObj = WalkConfig::FindFirstChildById(cellWrapper, WalkConfig::CELL_SPAN_ID))
            {
                auto* span = static_cast<OWPML::CCellSpan*>(spanObj);
                colSpan = (int)span->GetColSpan();
                rowSpan = (int)span->GetRowSpan();
                if (colSpan <= 0) colSpan = 1;
                if (rowSpan <= 0) rowSpan = 1;
            }

            // content
            auto* content = WalkConfig::FindFirstChildById(cellWrapper, WalkConfig::CELL_CONTENT_ID);

            // 디버그: cellWrapper child dump
            WalkConfig::DumpCellWrapperChildren(cellWrapper, r, c, depth);

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
    // rowGroups.size()는 "행 그룹 개수" 기준(대부분 실제 행 수와 동일)
    // addr 기반 inferredRowCount가 더 크면 그걸 우선 반영
    const int rowCount = std::max((int)rowGroups.size(), inferredRowCount);
    const int colCount = std::max(1, inferredColCount);

    if (WalkConfig::TABLE_LOG)
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

                // span 점유 마킹 (anchor 포함해서 전부 true)
                for (int rr = r; rr < r + cell.rowSpan && rr < rowCount; ++rr)
                {
                    for (int cc = c; cc < c + cell.colSpan && cc < colCount; ++cc)
                    {
                        occupied[rr][cc] = true;
                    }
                }

                // 셀 내용은 temp에 뽑아서 "진짜 빈칸인지" 판정 후 attribute 추가 가능
                std::wstring cellBuf;
                cellBuf.reserve(256);

                // td 내부 렌더링 모드
                Html::SetCellMode(true);

                if (cell.content)
                    ExtractTextImpl(cell.content, cellBuf, depth + 1);
                else if (cell.wrapper)
                    ExtractTextImpl(cell.wrapper, cellBuf, depth + 1);

                Html::SetCellMode(false);

                // SDK 기준 빈칸 셀은 wrapper는 존재하고 텍스트만 없는 케이스가 많음
                // 그때를 명시하고 싶으면 data-hwpx-empty 부여
                const bool isEmpty = IsHtmlEffectivelyEmpty(cellBuf);

                if (isEmpty)
                {
                    tdOpen += L" data-hwpx-empty=\"1\"";
                }

                tdOpen += L">";
                out += tdOpen;

                // 실제 내용
                out += cellBuf;

                out += L"</td>\n";
            }
            else
            {
                // occupied도 아니고 wrapper도 없으면 "비정상 hole" 또는 SDK 생략 케이스
                // 구조 유지용으로 빈 td를 생성하되, 명시적으로 표시
                out += L"<td data-hwpx-empty=\"1\"></td>\n";
            }
        }

        out += L"</tr>\n";
    }

    out += L"</table>\n";

    // (중첩표 안전) CellMode 복원
    Html::SetCellMode(prevCellMode);
}

// =====================
// Main recursive walker
// =====================
inline void ExtractTextImpl(OWPML::CObject* object, std::wstring& out, int depth)
{
    if (!object) return;

    if (depth > WalkConfig::MAX_DEPTH)
    {
        std::wcout << L"[WARN] Max depth exceeded. Stop recursion.\n";
        return;
    }

    OWPML::Objectlist* childList = object->GetObjectList();
    if (!childList) return;

    for (OWPML::CObject* child : *childList)
    {
        if (!child) continue;

        WalkConfig::ScanLogOnce(depth, child);

        const unsigned int id = SDK::GetID(child);

        // (선택) TARGET 서브트리 덤프
        if (WalkConfig::DUMP_MODE && WalkConfig::DUMP_TARGET_SUBTREE && id == WalkConfig::TARGET_ID)
        {
            std::wcout << L"\n========== TARGET SUBTREE DUMP START ==========\n";
            std::wcout << L"TARGET_ID=" << WalkConfig::TARGET_ID << L"\n\n";
            WalkConfig::DumpSubtree(child, depth, 0);
            std::wcout << L"========== TARGET SUBTREE DUMP END ==========\n\n";
            return;
        }

        switch (id)
        {
            // TABLE ROOT는 전용 렌더러로 처리
        case WalkConfig::TABLE_ROOT_ID:
        {
            RenderTableFromRoot373(child, out, depth);
            break;
        }

        // Paragraph (표 밖 텍스트)
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

        // 줄바꿈 세그먼트
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

// 기존 호출부 유지
inline void ExtractText(OWPML::CObject* object, std::wstring& out)
{
    ExtractTextImpl(object, out, 0);
}
