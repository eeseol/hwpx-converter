#pragma once

#include <string>

namespace OWPML {
    class CObject;
}

namespace WalkerDebug
{
    // SCAN: 처음 보는 ID만 1회 찍는 용도
    void ScanLogOnce(int depth, OWPML::CObject* obj);

    // 덤프: 서브트리 구조 출력
    void DumpSubtree(OWPML::CObject* root, int absDepth, int relDepth);

    // 덤프: cellWrapper 내부 child ID들 출력 + cellAddr/span 확인
    void DumpCellWrapperChildren(OWPML::CObject* cellWrapper, int r, int c, int depth);

    void DumpSubtreeSafe(OWPML::CObject* root, int absDepth, int maxNodes);

}
