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
}

namespace SDK {

    void InitStyleMap(OWPML::CStyles* styles) {
        if (!styles) return;

        g_styleMap.clear();

        const unsigned int cnt = styles->GetItemCnt();
        for (unsigned int i = 0; i < cnt; ++i) {
            auto* st = styles->Getstyle((int)i);
            if (!st) continue;

            g_styleMap[st->GetId()] = st->GetEngName();
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
