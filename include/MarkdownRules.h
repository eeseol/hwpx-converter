#pragma once
#include "SDK_Wrapper.h"

namespace Rules {

    // 1. [진입 규칙] 객체에 처음 들어갈 때 실행 (예: 제목 기호 # 추가)
    inline void OnPreProcess(OWPML::CObject* pObj, std::wstring& out) {
        UINT id = SDK::GetID(pObj);

        // CT(텍스트 뭉치)를 만났을 때 스타일 확인
        if (id == ID_PARA_T) {
            OWPML::CT* pText = (OWPML::CT*)pObj;
            UINT styleID = SDK::GetTextStyleID(pText);

            // 스타일 ID가 1~3번이면 마크다운 제목(#)으로 간주
            if (styleID >= 2 && styleID <= 4) {
                out += std::wstring(styleID-1, L'#') + L" ";
            }
        }
    }

    // 2. [데이터 규칙] 실제 글자 및 텍스트 내부의 줄바꿈 추출
    inline void OnProcess(OWPML::CObject* pObj, std::wstring& out) {
        UINT id = SDK::GetID(pObj);

        // <hp:t> 태그를 처리할 때 그 안의 자식들을 순서대로 검사
        if (id == ID_PARA_T) {
            OWPML::CT* pText = (OWPML::CT*)pObj;
            int count = SDK::GetChildCount(pText);

            for (int i = 0; i < count; i++) {
                OWPML::CObject* pChild = SDK::GetChildByIndex(pText, i);
                if (!pChild) continue;

                UINT childID = SDK::GetID(pChild);

                // 자식이 일반 글자(<hp:char>)인 경우
                if (childID == ID_PARA_Char) {
                    out.append(SDK::GetValue((OWPML::CChar*)pChild));
                }
                // 자식이 줄바꿈(<hp:lineBreak/>)인 경우
                else if (childID == ID_PARA_LineBreak) {
                    out += L"  \n"; // 마크다운 강제 줄바꿈 (스페이스 2개 + \n)
                }
            }
        }
    }

    // 3. 이탈 규칙
    inline void OnPostProcess(OWPML::CObject* pObj, std::wstring& out) {
        UINT id = SDK::GetID(pObj);

        // 헤더에 정의된 ID_PARA_PType 사용
        if (id == ID_PARA_PType) {
            out += L"\n\n"; // 문단 간격 띄우기
        }
    }
}