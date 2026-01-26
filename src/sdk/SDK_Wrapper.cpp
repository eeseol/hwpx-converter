#include <cwctype>
#include <map>
#include <string>
#include <cstdint>

#include "sdk/OwpmSDKPrelude.h"
#include "sdk/SDK_Wrapper.h"

namespace {
    // =========================
    // Style map
    // =========================
    std::map<unsigned int, std::wstring> g_styleMap;

    static std::wstring Trim(const std::wstring& s)
    {
        size_t start = 0;
        while (start < s.size() && iswspace(s[start])) start++;

        size_t end = s.size();
        while (end > start && iswspace(s[end - 1])) end--;

        return s.substr(start, end - start);
    }

    static std::wstring NormalizeStyleEngName(std::wstring eng)
    {
        eng = Trim(eng);
        if (eng.empty()) return L"Normal";

        const std::wstring marker = L" Copy";
        size_t pos = eng.rfind(marker);
        if (pos != std::wstring::npos)
        {
            size_t numStart = pos + marker.size();
            if (numStart < eng.size())
            {
                bool allDigits = true;
                for (size_t i = numStart; i < eng.size(); ++i)
                {
                    if (eng[i] < L'0' || eng[i] > L'9') { allDigits = false; break; }
                }

                if (allDigits)
                {
                    eng = Trim(eng.substr(0, pos));
                    if (eng.empty()) return L"Normal";
                }
            }
        }

        return eng;
    }

    // =========================
    // List maps
    // =========================
    struct ParaPrListMeta {
        SDK::ListKind kind = SDK::ListKind::None;
        std::uint32_t idRef = 0;
        std::uint32_t level = 0;
    };

    std::map<std::uint32_t, ParaPrListMeta> g_paraPrListMeta;

    struct BulletMeta {
        std::wstring ch;
        std::wstring checkedCh;
        bool checkable = false;
    };
    std::map<std::uint32_t, BulletMeta> g_bulletMetaById;

    // numbering은 존재 여부만
    std::map<std::uint32_t, bool> g_numberingExistsById;
}

namespace SDK {

    // =========================
    // Style
    // =========================
    void InitStyleMap(OWPML::CStyles* pStyles) {
        if (!pStyles) return;
        g_styleMap.clear();

        const unsigned int count = pStyles->GetItemCnt();
        for (unsigned int i = 0; i < count; ++i) {
            auto* pStyle = pStyles->Getstyle((int)i);
            if (!pStyle) continue;

            std::wstring raw = pStyle->GetEngName();
            std::wstring norm = NormalizeStyleEngName(raw);
            g_styleMap[pStyle->GetId()] = norm;
        }
    }

    std::wstring GetStyleEngName(unsigned int styleID) {
        auto it = g_styleMap.find(styleID);
        if (it != g_styleMap.end()) return it->second;
        return L"Body";
    }

    // =========================
    // Tree helpers
    // =========================
    unsigned int GetID(OWPML::CObject* obj) {
        return obj ? obj->GetID() : 0;
    }

    int GetChildCountObj(OWPML::CObject* obj) {
        if (!obj) return 0;
        auto* list = obj->GetObjectList();
        if (!list) return 0;
        return (int)list->size();
    }

    OWPML::CObject* GetChildAtObj(OWPML::CObject* obj, int index) {
        if (!obj || index < 0) return nullptr;
        auto* list = obj->GetObjectList();
        if (!list) return nullptr;
        if (index >= (int)list->size()) return nullptr;

        auto it = list->begin();
        std::advance(it, index);
        return *it;
    }

    // =========================
    // Paragraph / Text
    // =========================
    unsigned int GetParaStyleID(OWPML::CPType* para) {
        return para ? para->GetStyleIDRef() : 0;
    }

    int GetChildCount(OWPML::CT* text) {
        return text ? text->GetChildCount() : 0;
    }

    OWPML::CObject* GetChildByIndex(OWPML::CT* text, int index) {
        return text ? text->GetObjectByIndex(index) : nullptr;
    }

    std::wstring GetCharValue(OWPML::CChar* ch) {
        return ch ? ch->Getval() : L"";
    }

    unsigned int GetParaPrIDRef(OWPML::CPType* para)
    {
        if (!para) return 0;
        return (unsigned int)para->GetParaPrIDRef();
    }

    // ============================================================
    // LIST INIT
    // ============================================================

    void InitNumberings(OWPML::CNumberings* numberings)
    {
        g_numberingExistsById.clear();
        if (!numberings) return;

        const unsigned int count = numberings->GetItemCnt();
        for (unsigned int i = 0; i < count; ++i)
        {
            auto* n = numberings->Getnumbering((int)i);
            if (!n) continue;
            g_numberingExistsById[(std::uint32_t)n->GetId()] = true;
        }
    }

    void InitBullets(OWPML::CBullets* bullets)
    {
        g_bulletMetaById.clear();
        if (!bullets) return;

        const unsigned int count = bullets->GetItemCnt();
        for (unsigned int i = 0; i < count; ++i)
        {
            auto* b = bullets->Getbullet((int)i);
            if (!b) continue;

            BulletMeta meta;
            meta.ch = b->GetChar() ? b->GetChar() : L"";
            meta.checkedCh = b->GetCheckedChar() ? b->GetCheckedChar() : L"";

            // checkable은 paraHead에 있음
            auto* ph = b->GetparaHead(0);
            if (ph)
            {
                // GetCheckable()이 int/UINT 형태일 확률 큼
                meta.checkable = (ph->GetCheckable() != 0);
            }

            g_bulletMetaById[(std::uint32_t)b->GetId()] = meta;
        }
    }

    void InitParaProperties(OWPML::CParaProperties* paraProps)
    {
        g_paraPrListMeta.clear();
        if (!paraProps) return;

        const unsigned int count = paraProps->GetItemCnt();

        for (unsigned int i = 0; i < count; ++i)
        {
            auto* paraPr = paraProps->GetparaPr((int)i);
            if (!paraPr) continue;

            const std::uint32_t paraPrId = (std::uint32_t)paraPr->GetId();
            ParaPrListMeta meta;

            auto* heading = paraPr->Getheading();
            if (heading)
            {
                const std::uint32_t idRef = (std::uint32_t)heading->GetIdRef();
                const std::uint32_t level = (std::uint32_t)heading->GetLevel();

                // ★ 핵심: idRef가 0이면 "리스트 아님"
                if (idRef != 0)
                {
                    const bool isNumbering = (g_numberingExistsById.find(idRef) != g_numberingExistsById.end());
                    const bool isBullet = (g_bulletMetaById.find(idRef) != g_bulletMetaById.end());

                    // ★ 핵심: 실제 bullets/numberings 중 하나에 존재할 때만 리스트 인정
                    if (isNumbering || isBullet)
                    {
                        meta.kind = isNumbering ? ListKind::Numbering : ListKind::Bullet;
                        meta.idRef = idRef;
                        meta.level = level;
                    }
                }
            }

            g_paraPrListMeta[paraPrId] = meta;
        }
    }

    // ============================================================
    // LIST INFO EXTRACT
    // ============================================================
    ListInfo GetListInfoFromParagraph(OWPML::CPType* para)
    {
        ListInfo info;
        if (!para) return info;

        const std::uint32_t paraPrId = (std::uint32_t)GetParaPrIDRef(para);
        if (paraPrId == 0) return info;

        auto it = g_paraPrListMeta.find(paraPrId);
        if (it == g_paraPrListMeta.end()) return info;

        const ParaPrListMeta& meta = it->second;

        // meta가 None이면 그대로 리턴
        if (meta.kind == ListKind::None) return info;

        // ★ 안전장치: idRef==0이면 리스트로 치지 않음
        if (meta.idRef == 0) return info;

        info.kind = meta.kind;
        info.idRef = meta.idRef;
        info.level = meta.level;

        if (info.kind == ListKind::Bullet)
        {
            auto bit = g_bulletMetaById.find(info.idRef);
            if (bit != g_bulletMetaById.end())
            {
                info.bulletChar = bit->second.ch;
                info.checkable = bit->second.checkable;
            }
        }

        return info;
    }

} // namespace SDK
