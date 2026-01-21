#pragma once

// ===== 0) Windows 매크로 지뢰 제거 =====
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef interface
#undef interface
#endif
#ifdef byte
#undef byte
#endif

// ===== 1) Windows 타입(LPCWSTR, BOOL 등) 보장 =====
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

// ===== 2) STL 보장 (SDK 내부가 몰래 의존함) =====
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <deque>
#include <iostream>
#include <fstream>

// ===== 3) 한컴 SDK 핵심 헤더 =====
#include "OWPMLApi/OWPMLApi.h"
#include "OWPMLUtil/HncDefine.h"
#include "OWPML/OWPML.h"
#include "OWPML/OwpmlForDec.h"
