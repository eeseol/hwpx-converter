// main.cpp
#include <iostream>
#include <string>
#include <cwctype>
#include <algorithm>
#include <filesystem>

#include <fcntl.h>
#include <io.h>

#include "app/HwpxConverter.h"

namespace fs = std::filesystem;

static std::wstring Trim(const std::wstring& s)
{
    size_t b = 0;
    while (b < s.size() && iswspace(s[b])) b++;
    size_t e = s.size();
    while (e > b && iswspace(s[e - 1])) e--;
    return s.substr(b, e - b);
}

static std::wstring StripQuotes(std::wstring s)
{
    s = Trim(s);
    if (s.size() >= 2 && ((s.front() == L'"' && s.back() == L'"') || (s.front() == L'\'' && s.back() == L'\'')))
        return s.substr(1, s.size() - 2);
    return s;
}

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

static bool IsHwpxPath(const std::wstring& path)
{
    return EndsWithIgnoreCase(path, L".hwpx");
}

static bool ContainsWhitespaceOrParen(const std::wstring& s)
{
    for (wchar_t ch : s) {
        if (iswspace(ch) || ch == L'(' || ch == L')') return true;
    }
    return false;
}

// Windows 파일명에서 금지 문자 제거(출력 "파일명"만 정리)
static std::wstring SanitizeFileName(std::wstring name)
{
    // 금지 문자: < > : " / \ | ? *  + 제어문자(0~31)
    for (auto& ch : name)
    {
        if (ch < 32) { ch = L'_'; continue; }
        switch (ch)
        {
        case L'<': case L'>': case L':': case L'"':
        case L'/': case L'\\': case L'|': case L'?': case L'*':
            ch = L'_';
            break;
        default:
            break;
        }
    }

    // 끝의 공백/점 제거 (Windows 규칙)
    while (!name.empty() && (name.back() == L' ' || name.back() == L'.'))
        name.pop_back();

    name = Trim(name);
    if (name.empty()) name = L"output";

    // 예약 장치명(CON, PRN, AUX, NUL, COM1~9, LPT1~9) 방어
    auto upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](wchar_t c) { return (wchar_t)towupper(c); });

    auto IsReserved = [&](const std::wstring& u) -> bool {
        if (u == L"CON" || u == L"PRN" || u == L"AUX" || u == L"NUL") return true;
        if (u.rfind(L"COM", 0) == 0 && u.size() == 4 && (u[3] >= L'1' && u[3] <= L'9')) return true;
        if (u.rfind(L"LPT", 0) == 0 && u.size() == 4 && (u[3] >= L'1' && u[3] <= L'9')) return true;
        return false;
        };

    if (IsReserved(upper))
        name = L"_" + name;

    return name;
}

static fs::path MakeUniquePath(const fs::path& desired)
{
    if (!fs::exists(desired)) return desired;

    fs::path dir = desired.parent_path();
    std::wstring stem = desired.stem().wstring();
    std::wstring ext = desired.extension().wstring();
    if (ext.empty()) ext = L".html";

    for (int i = 1; i < 10000; ++i)
    {
        std::wstring candidateName = stem + L" (" + std::to_wstring(i) + L")" + ext;
        fs::path cand = dir.empty() ? fs::path(candidateName) : (dir / candidateName);
        if (!fs::exists(cand)) return cand;
    }
    // 비정상적으로 많이 겹치면 마지막 fallback
    return desired;
}

// 출력 경로에서 "파일명만" 정리하고, 확장자 .html 보장
static fs::path NormalizeOutputPath(const fs::path& outRaw)
{
    fs::path dir = outRaw.parent_path();
    std::wstring filename = outRaw.filename().wstring();

    if (filename.empty()) filename = L"output.html";

    // 확장자 처리
    fs::path tmp(filename);
    std::wstring stem = tmp.stem().wstring();
    std::wstring ext = tmp.extension().wstring();

    if (!EndsWithIgnoreCase(ext, L".html")) {
        // 사용자가 .htm / 다른 확장자 / 확장자 없음 -> .html로 고정
        ext = L".html";
    }

    stem = SanitizeFileName(stem);
    fs::path finalName = fs::path(stem + ext);

    if (dir.empty()) return finalName;
    return dir / finalName;
}

static void PrintUsage(const wchar_t* argv0)
{
    std::wcout << L"사용법:\n"
        << L"  " << argv0 << L" <input.hwpx> [output.html]\n\n"
        << L"규칙:\n"
        << L"  - output 생략 시: input과 같은 폴더에 <입력파일명>.html 자동 생성\n"
        << L"  - 출력 파일명만 안전 문자로 정리(입력 파일명은 변경하지 않음)\n"
        << L"  - 동일 파일 존재 시: (1), (2)... 붙여서 덮어쓰기 방지\n";
}

int wmain(int argc, wchar_t* argv[])
{
    // 콘솔 wide 출력 안정화 (한글/경로 깨짐 방지)
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    if (argc < 2) {
        PrintUsage(argv[0]);
        return -1;
    }

    // 공백 경로를 따옴표로 안 감싸면 argv가 쪼개져 argc가 커짐
    // 이번 버전은 옵션 파싱 없이 "2개 또는 3개 인자"만 지원
    if (argc > 3) {
        std::wcout << L"[ERROR] 인자 개수가 너무 많습니다. 입력 경로에 공백이 있으면 따옴표로 감싸주세요.\n";
        std::wcout << L"예) " << argv[0] << L" \"C:\\path with space\\in.hwpx\" out.html\n\n";
        PrintUsage(argv[0]);
        return -1;
    }

    const std::wstring inputRaw = argv[1];
    const std::wstring inputPathW = StripQuotes(inputRaw);
    const fs::path inputPath(inputPathW);

    // 1) 확장자 검사
    if (!IsHwpxPath(inputPathW)) {
        std::wcout << L"[ERROR] 입력 파일은 .hwpx만 지원합니다.\n";
        std::wcout << L"        입력: " << inputPathW << L"\n";
        return -1;
    }

    // 2) 입력 파일 존재 검사
    std::error_code ec;
    if (!fs::exists(inputPath, ec)) {
        std::wcout << L"[ERROR] 입력 파일을 찾을 수 없습니다.\n";
        std::wcout << L"        입력: " << inputPathW << L"\n";
        if (ContainsWhitespaceOrParen(inputPathW)) {
            std::wcout << L"        힌트: 경로에 공백/괄호가 있으면 따옴표로 감싸서 실행해 주세요.\n";
        }
        return -1;
    }

    // 3) 출력 경로 결정(옵션)
    fs::path outputPath;
    if (argc == 3)
    {
        const std::wstring outRaw = StripQuotes(argv[2]);
        fs::path outPathRaw(outRaw);

        // 파일명만 sanitize + .html 보장
        outputPath = NormalizeOutputPath(outPathRaw);

        // 출력 디렉토리 존재 검사(명시된 경우)
        fs::path outDir = outputPath.parent_path();
        if (!outDir.empty() && !fs::exists(outDir, ec)) {
            std::wcout << L"[ERROR] 출력 폴더가 존재하지 않습니다.\n";
            std::wcout << L"        출력: " << outputPath.wstring() << L"\n";
            return -1;
        }
    }
    else
    {
        // 자동 생성: 입력과 같은 폴더 + <입력 stem>.html
        fs::path dir = inputPath.parent_path();
        std::wstring stem = inputPath.stem().wstring();
        stem = SanitizeFileName(stem);

        outputPath = dir / fs::path(stem + L".html");
    }

    // 4) 덮어쓰기 방지: (1)(2)...
    outputPath = MakeUniquePath(outputPath);

    ConvertOptions opt;
    opt.outputHtml = true;

    // 5) 변환
    if (!ConvertHwpxToHtml(inputPathW, outputPath.wstring(), opt)) {
        std::wcout << L"[ERROR] 변환 실패: 표준 HWPX 파일이 아니거나 손상된 파일일 수 있습니다.\n";
        std::wcout << L"        입력: " << inputPathW << L"\n";
        return -1;
    }

    std::wcout << L"변환 완료: " << outputPath.wstring() << L"\n";
    return 0;
}
