#include <iostream>
#include <string>
#include <cwctype>

#include <fcntl.h>
#include <io.h>

#include "app/HwpxConverter.h"

static bool EndsWithIgnoreCase(const std::wstring& s, const std::wstring& suffix)
{
    if (s.size() < suffix.size()) return false;
    size_t off = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i) {
        wchar_t a = towlower(s[off + i]);
        wchar_t b = towlower(suffix[i]);
        if (a != b) return false;
    }
    return true;
}

int wmain(int argc, wchar_t* argv[])
{
    // 콘솔 wide 출력 안정화 (한글/경로 깨짐 방지)
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    if (argc < 3) {
        std::wcout << L"사용법: " << argv[0] << L" <input.hwpx> <output.html>\n";
        return -1;
    }

    const std::wstring inputPath = argv[1];
    const std::wstring outputPath = argv[2];

    // 1) 확장자 검사
    if (!EndsWithIgnoreCase(inputPath, L".hwpx")) {
        std::wcout << L"[ERROR] 입력 파일은 .hwpx만 지원합니다.\n"
            << std::flush;
        return -1;
    }

    ConvertOptions opt;
    opt.outputHtml = true;

    // 2) 변환 시도
    if (!ConvertHwpxToHtml(inputPath, outputPath, opt)) {
        std::wcout << L"[ERROR] 변환 실패: 표준 HWPX 파일이 아니거나 손상된 파일일 수 있습니다.\n"
            << std::flush;
        return -1;
    }

    std::wcout << L"변환 완료: " << outputPath << L"\n";
    return 0;
}
