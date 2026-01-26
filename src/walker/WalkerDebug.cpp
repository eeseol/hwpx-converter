#include "walker/WalkerDebug.h"
#include "walker/WalkerConfig.h"

#include <map>
#include <iostream>
#include <vector>

#include "sdk/OwpmSDKPrelude.h"
#include "sdk/SDK_Wrapper.h"

namespace
{
    std::map<unsigned int, int>& SeenIdOnce()
    {
        static std::map<unsigned int, int> m;
        return m;
    }

    int CountChildrenByObjectList(OWPML::CObject* obj)
    {
        if (!obj) return 0;
        int childCount = 0;
        if (auto list = obj->GetObjectList())
        {
            for (auto* _ : *list) { (void)_; childCount++; }
        }
        return childCount;
    }

    const wchar_t* KnownIdName(unsigned int id)
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
}

namespace WalkerDebug
{
    void ScanLogOnce(int depth, OWPML::CObject* obj)
    {
        if (!WalkerConfig::SCAN_MODE || !obj) return;

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

    void DumpSubtree(OWPML::CObject* root, int absDepth, int relDepth)
    {
        if (!root) return;
        if (relDepth > WalkerConfig::DUMP_MAX_REL_DEPTH) return;

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

    void DumpCellWrapperChildren(OWPML::CObject* cellWrapper, int r, int c, int depth)
    {
        if (!WalkerConfig::DUMP_MODE || !WalkerConfig::DUMP_CELL_WRAPPER_CHILDREN) return;
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

            if (cid == WalkerConfig::CELL_CONTENT_ID)  std::wcout << L" (cellContent)";
            if (cid == WalkerConfig::CELL_ADDR_ID)     std::wcout << L" (cellAddr)";
            if (cid == WalkerConfig::CELL_SPAN_ID)     std::wcout << L" (cellSpan)";
            if (cid == WalkerConfig::CELL_SZ_ID)       std::wcout << L" (cellSz)";
            if (cid == WalkerConfig::CELL_MARGIN_ID)   std::wcout << L" (cellMargin)";

            std::wcout << L"\n";

            if (cid == WalkerConfig::CELL_ADDR_ID)
            {
                auto* addr = static_cast<OWPML::CCellAddr*>(ch);
                std::wcout
                    << L"    -> CCellAddr CONFIRMED! row="
                    << addr->GetRowAddr()
                    << L", col="
                    << addr->GetColAddr()
                    << L"\n";
            }

            if (cid == WalkerConfig::CELL_SPAN_ID)
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

    void DumpSubtreeSafe(OWPML::CObject* root, int absDepth, int maxNodes)
    {
        if (!root) return;
        if (maxNodes <= 0) maxNodes = 300;

        struct Frame
        {
            OWPML::CObject* node;
            int abs;
            int rel;
        };

        std::vector<Frame> st;
        st.reserve(1024);
        st.push_back({ root, absDepth, 0 });

        int printed = 0;

        while (!st.empty() && printed < maxNodes)
        {
            Frame cur = st.back();
            st.pop_back();

            if (!cur.node) continue;
            if (cur.rel > WalkerConfig::DUMP_MAX_REL_DEPTH) continue;

            const unsigned int id = SDK::GetID(cur.node);
            const int childCount = CountChildrenByObjectList(cur.node);

            for (int i = 0; i < cur.rel; ++i) std::wcout << L"  ";

            std::wcout
                << L"[DUMP_SAFE] absDepth=" << cur.abs
                << L" rel=" << cur.rel
                << L" id=" << id
                << L" childCount=" << childCount
                << L" ptr=0x" << (void*)cur.node
                << L"\n";

            printed++;

            auto list = cur.node->GetObjectList();
            if (!list) continue;

            // 출력 순서 유지하려고 역순 push
            std::vector<OWPML::CObject*> children;
            children.reserve(list->size());
            for (auto* ch : *list) children.push_back(ch);

            for (int i = (int)children.size() - 1; i >= 0; --i)
                st.push_back({ children[i], cur.abs + 1, cur.rel + 1 });
        }

        std::wcout << L"[DUMP_SAFE] printed=" << printed << L"/" << maxNodes << L"\n";
        std::wcout.flush();
    }

} // namespace WalkerDebug
