#pragma once

// 1. Windows 및 표준 라이브러리 (SDK 내부 헤더 의존성 포함)
#include <windows.h>
#include <Shlwapi.h>
#include <stack>
#include <deque>
#include <map>
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