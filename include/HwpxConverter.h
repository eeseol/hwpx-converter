#pragma once

// 1. Windows 및 표준 라이브러리 (SDK 내부 헤더 의존성 포함)
#include <windows.h>
#include <Shlwapi.h>
#include <stack>      // 필수: Handler.h 등에서 사용
#include <deque>      // 필수: PartNameSpaceInfo.h 등에서 사용
#include <map>        // 필수: Document.h 등에서 사용
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>

#pragma comment(lib, "shlwapi.lib")

// 2. 한컴 엔진 의존성 사슬 (순서 고정)
#include "OWPMLApi/OWPMLApi.h"
#include "OWPMLUtil/HncDefine.h"
#include "OWPML/OWPML.h"
#include "OWPML/OwpmlForDec.h"

// 3. 텍스트 추출 함수 정의 (inline으로 헤더 포함 허용)
inline void ExtractText(OWPML::CObject* pObject, std::wstring& out) {
    if (!pObject) return;

    UINT id = pObject->GetID();
    if (id == ID_PARA_T) {
        OWPML::CT* pText = (OWPML::CT*)pObject;
        int count = pText->GetChildCount();
        for (int i = 0; i < count; i++) {
            OWPML::CObject* pChild = pText->GetObjectByIndex(i);
            if (pChild && pChild->GetID() == ID_PARA_Char) {
                out.append(((OWPML::CChar*)pChild)->Getval());
            }
        }
    }
    else if (id == ID_PARA_LineSeg) {
        out += L"\n";
    }
    else {
        OWPML::Objectlist* pChildList = pObject->GetObjectList();
        if (pChildList) {
            for (auto child : *pChildList) ExtractText(child, out);
        }
    }
}