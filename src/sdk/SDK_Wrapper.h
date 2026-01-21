#pragma once
#include <string>

namespace OWPML {
    class CObject;
    class CStyles;
    class CStyleType;
    class CPType;
    class CT;
    class CChar;
}

namespace SDK {

    // ===== 스타일 맵 =====
    void InitStyleMap(OWPML::CStyles* styles);
    std::wstring GetStyleEngName(unsigned int styleID);

    // ===== 트리 탐색(재귀용) =====
    unsigned int GetID(OWPML::CObject* obj);

    int GetChildCountObj(OWPML::CObject* obj);
    OWPML::CObject* GetChildAtObj(OWPML::CObject* obj, int index);

    // ===== 문단 스타일 =====
    unsigned int GetParaStyleID(OWPML::CPType* para);

    // ===== CT 텍스트 런 =====
    int GetChildCount(OWPML::CT* text);
    OWPML::CObject* GetChildByIndex(OWPML::CT* text, int index);

    // ===== 글자 =====
    std::wstring GetCharValue(OWPML::CChar* ch);
}
