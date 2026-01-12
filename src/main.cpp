#include "HwpxConverter.h"

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) return -1;

    // 1. 읽기
    OWPML::COwpmlDocumnet* pDoc = OWPML::COwpmlDocumnet::OpenDocument(argv[1]);
    if (!pDoc) return -1;

    // 2. 뽑기
    std::wstring resultText;
    auto* sections = pDoc->GetSections();
    if (sections) {
        for (auto* section : *sections) ExtractText(section, resultText);
    }

    // 3. 저장
    std::wofstream f(argv[2]);
    f.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));
    if (f.is_open()) {
        f << resultText;
        f.close();
    }

    delete pDoc;
    return 0;
}