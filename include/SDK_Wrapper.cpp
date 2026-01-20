#include <cwctype>
#include "OwpmSDKPrelude.h"
#include "SDK_Wrapper.h"

// 세부 클래스 헤더 (스타일 파싱용)
#include "OWPML/Class/Head/styles.h"
#include "OWPML/Class/Head/StyleType.h"
#include "OWPML/Class/Para/PType.h"
#include "OWPML/Class/Para/t.h"
#include "OWPML/Class/Para/text.h"

namespace {
    std::map<unsigned int, std::wstring> g_styleMap;

    // 앞/뒤 공백 제거
    static std::wstring Trim(const std::wstring& s)
    {
        size_t start = 0;
        while (start < s.size() && iswspace(s[start])) start++;

        size_t end = s.size();
        while (end > start && iswspace(s[end - 1])) end--;

        return s.substr(start, end - start);
    }

    // "Normal Copy6" 같은 복제 스타일 흡수
    static std::wstring NormalizeStyleEngName(std::wstring eng)
    {
        eng = Trim(eng);

        // engName이 비어있으면 Normal로 fallback
        if (eng.empty()) return L"Normal";

        // 규칙: " Copy" + 숫자로 끝나면 Copy 부분 제거
        // 예: "Normal Copy6" -> "Normal"
        const std::wstring marker = L" Copy";
        size_t pos = eng.rfind(marker);
        if (pos != std::wstring::npos)
        {
            // marker 뒤쪽이 숫자로만 구성되면 Copy suffix로 판단
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
                    eng = Trim(eng.substr(0, pos)); // " CopyN" 제거
                    if (eng.empty()) return L"Normal";
                }
            }
        }

        return eng;
    }
}

namespace SDK {

    void InitStyleMap(OWPML::CStyles* pStyles) {
        if (!pStyles) return;
        g_styleMap.clear();

        const unsigned int count = pStyles->GetItemCnt();
        for (unsigned int i = 0; i < count; ++i) {
            auto* pStyle = pStyles->Getstyle((int)i);
            if (!pStyle) continue;

            std::wstring raw = pStyle->GetEngName();              // 원본 engName
            std::wstring norm = NormalizeStyleEngName(raw);       // 정규화 engName

            g_styleMap[pStyle->GetId()] = norm;
        }
    }

    std::wstring GetStyleEngName(unsigned int styleID) {
        auto it = g_styleMap.find(styleID);
        if (it != g_styleMap.end()) return it->second;
        return L"Body";
    }

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

} // namespace SDK
