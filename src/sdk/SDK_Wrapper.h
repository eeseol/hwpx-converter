#pragma once
#include <string>
#include <cstdint>

namespace OWPML {
    class CObject;

    class CStyles;
    class CPType;
    class CT;
    class CChar;

    // Head쪽 타입들
    class CRefList;
    class CParaProperties;
    class CBullets;
    class CNumberings;
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

    // ===== paraPrIDRef =====
    unsigned int GetParaPrIDRef(OWPML::CPType* para);

    // ============================================================
    // LIST INFO
    // ============================================================

    enum class ListKind : std::uint8_t {
        None = 0,
        Numbering = 1,
        Bullet = 2
    };

    struct ListInfo {
        ListKind kind = ListKind::None;

        // heading이 가리키는 idRef (numbering id 또는 bullet id)
        std::uint32_t idRef = 0;

        // 문서의 level (정보만 보존)
        std::uint32_t level = 0;

        // bullet일 때 원본 글머리 문자 (렌더링은 점으로 통일하더라도 보존)
        std::wstring bulletChar;

        // 체크형(checkbox) 여부 (정책상 렌더링은 점으로 통일)
        bool checkable = false;
    };

    // Head에서 paraPr/numberings/bullets 맵 구성
    void InitParaProperties(OWPML::CParaProperties* paraProps);
    void InitBullets(OWPML::CBullets* bullets);
    void InitNumberings(OWPML::CNumberings* numberings);

    // 문단이 리스트인지 판별 + bullet char 추출
    ListInfo GetListInfoFromParagraph(OWPML::CPType* para);

} // namespace SDK
