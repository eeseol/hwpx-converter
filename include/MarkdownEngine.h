#pragma once
#include "MarkdownRules.h" // 2단계(Rules)를 포함하면 1단계(Wrapper)까지 자동으로 연결됨

// 최종 엔진: 메인에서 이 함수를 호출하면 전체 변환이 시작됩니다.
inline void ExtractText(OWPML::CObject* pObject, std::wstring& out) {
    if (!pObject) return;

    // [Step 1] 진입 규칙 실행 (Rules.h의 OnPreProcess)
    // 여기서 스타일을 보고 제목(#)을 붙일지 말지 결정합니다.
    Rules::OnPreProcess(pObject, out);

    // [Step 2] 데이터 추출 실행 (Rules.h의 OnProcess)
    // 여기서 실제 텍스트 글자들을 수집합니다.
    Rules::OnProcess(pObject, out);

    // [Step 3] 재귀 탐색 (Wrapper.h의 탐색 도구 활용)
    // 현재 객체(부모) 밑에 자식들이 있다면 하나씩 꺼내서 다시 이 함수를 호출합니다.
    OWPML::Objectlist* pChildList = SDK::GetChildList(pObject);
    if (pChildList) {
        for (auto child : *pChildList) {
            ExtractText(child, out); // <--- 재귀가 돌아가는 핵심 지점
        }
    }

    // [Step 4] 이탈 규칙 실행 (Rules.h의 OnPostProcess)
    // 자식들까지 다 훑고 올라오면서 문단 끝 줄바꿈(\n\n) 등을 처리합니다.
    Rules::OnPostProcess(pObject, out);
}