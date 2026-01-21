#include <iostream>
#include "app/HwpxConverter.h"

int wmain(int argc, wchar_t* argv[])
{
    if (argc < 3) {
        std::wcout << L"사용법: " << argv[0] << L" <input.hwpx> <output.html>\n";
        return -1;
    }

    ConvertOptions opt;
    opt.outputHtml = true;

    if (!ConvertHwpxToHtml(argv[1], argv[2], opt)) {
        std::wcout << L"변환 실패\n";
        return -1;
    }

    std::wcout << L"변환 완료: " << argv[2] << L"\n";
    return 0;
}
