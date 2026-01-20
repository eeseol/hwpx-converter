#pragma once
#include <string>

struct ConvertOptions {
    bool outputHtml = true; // 지금은 true만 지원해도 됨
};

bool ConvertHwpxToHtml(
    const std::wstring& inputPath,
    const std::wstring& outputPath,
    const ConvertOptions& opt = {}
);
