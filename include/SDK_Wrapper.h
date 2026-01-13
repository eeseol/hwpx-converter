#pragma once
#include "HwpxConverter.h"

namespace SDK {

    // 트리 탐색 도구. 재귀 돌아갑니다.===============================
    
    // 현재 노드가 어떤 타입(문단, 텍스트, 그림 등)인지 식별 번호를 가져옵니다.
    inline UINT GetID(OWPML::CObject* pObj) { return pObj->GetID(); }

    // 현재 노드 밑에 달린 자식 객체들의 목록(리스트)을 가져옵니다. (재귀의 핵심)
    inline OWPML::Objectlist* GetChildList(OWPML::CObject* pObj) { return pObj->GetObjectList(); }


    // 텍스트 뭉치(CT) 분석 도구.  =============================

    // CT(텍스트 런) 객체 안에 실제 글자(CChar)가 몇 개 들어있는지 확인합니다.
    inline int GetChildCount(OWPML::CT* pText) {
        return pText ? pText->GetChildCount() : 0;
    }

    // CT 객체 내부에서 n번째 자식 객체(보통 CChar)를 콕 집어서 가져옵니다.
    inline OWPML::CObject* GetChildByIndex(OWPML::CT* pText, int index) {
        return pText ? pText->GetObjectByIndex(index) : nullptr;
    }

    // 이 텍스트 뭉치에 적용된 스타일 번호를 가져옵니다.
    inline UINT GetTextStyleID(OWPML::CT* pText) {
        return pText ? pText->GetCharStyleIDRef() : 0;
    }


    // [실제 글자(CChar) 데이터 추출] -------------------------------------------

    // CChar 객체에 담긴 실제 문자열(wstring) 값을 뽑아냅니다.
    inline std::wstring GetCharValue(OWPML::CChar* pChar) {
        return pChar ? pChar->Getval() : L"";
    }

    // 기존 GetValue와 중복될 수 있어 하나로 통일하거나 별칭으로 사용합니다.
    inline std::wstring GetValue(OWPML::CChar* pChar) {
        return GetCharValue(pChar);
    }
}