#include "MarkdownEngine.h"

int wmain(int argc, wchar_t* argv[]) {
    // 인자 체크: 프로그램명, 입력파일(hwpx), 출력파일(md)
    if (argc < 3) {
        std::wcout << L"사용법: " << argv[0] << L" <input.hwpx> <output.md>" << std::endl;
        return -1;
    }

    // 1. 문서 열기
    OWPML::COwpmlDocumnet* pDoc = OWPML::COwpmlDocumnet::OpenDocument(argv[1]);
    if (!pDoc) {
        std::wcout << L"파일을 열 수 없습니다: " << argv[1] << std::endl;
        return -1;
    }

    // 2. 텍스트 추출 (재귀 시작)
    std::wstring resultText;
    auto* sections = pDoc->GetSections();
    if (sections) {
        for (auto* section : *sections) {
            // 우리가 만든 Layer 2 엔진 호출
            ExtractText(section, resultText);
        }
    }

    // 3. 파일 저장 (UTF-8 인코딩 설정)
    std::wofstream f(argv[2]);
    // 마크다운 표준인 UTF-8로 한글을 저장하기 위한 설정
    f.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

    if (f.is_open()) {
        f << resultText;
        f.close();
        std::wcout << L"변환 완료: " << argv[2] << std::endl;
    }
    else {
        std::wcout << L"출력 파일을 생성할 수 없습니다." << std::endl;
    }

    // 4. 정리
    // OpenDocument로 생성된 객체 해제
    delete pDoc;

    return 0;
}