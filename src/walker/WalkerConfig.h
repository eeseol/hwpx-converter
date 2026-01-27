#pragma once

#include <cstdint>

namespace WalkerConfig
{
    // =========================================================
    // Global toggles (개발/디버그 모드)
    // - 릴리즈에선 false 유지 추천
    // =========================================================
    inline constexpr bool SCAN_MODE = false;   // "처음 보는 ID"만 1회 찍는 스캔
    inline constexpr bool DUMP_MODE = false;   // 덤프 출력 활성화
    inline constexpr bool TABLE_LOG = false;  // 테이블 렌더링 로그

    // 덤프 세부 옵션
    inline constexpr bool DUMP_TABLE_SUBTREE = true;
    inline constexpr bool DUMP_CELL_WRAPPER_CHILDREN = true;

    // 특정 ID만 서브트리 덤프하고 싶을 때
    inline constexpr bool DUMP_TARGET_SUBTREE = false;
    inline constexpr std::uint32_t TARGET_ID = 0;

    // =========================================================
    // Safety limits
    // =========================================================
    inline constexpr int MAX_DEPTH = 5000;          // 워커 재귀 안전장치
    inline constexpr int DUMP_MAX_REL_DEPTH = 20;   // 덤프할 때 상대 깊이 제한

    // =========================================================
    // Table / Cell structure IDs (네 로그로 "확정"된 값)
    // =========================================================
    inline constexpr std::uint32_t TABLE_ROOT_ID = 805306373;  // table root
    inline constexpr std::uint32_t ROW_GROUP_ID = 805306463;  // row group wrapper
    inline constexpr std::uint32_t CELL_WRAPPER_ID = 805306465;  // cell wrapper
    inline constexpr std::uint32_t CELL_CONTENT_ID = 805306406;  // cell content
    inline constexpr std::uint32_t CELL_ADDR_ID = 805306466;  // cellAddr
    inline constexpr std::uint32_t CELL_SPAN_ID = 805306467;  // cellSpan
    inline constexpr std::uint32_t CELL_SZ_ID = 805306468;  // cellSz
    inline constexpr std::uint32_t CELL_MARGIN_ID = 805306469;  // cellMargin

    // =========================================================
    // Optional: HTML output 정책 (향후 확장용)
    // =========================================================

    // 셀 내 "진짜 빈칸"을 찍을지
    // - covered(병합으로 덮인 셀)는 skip
    // - hole(데이터 자체가 없는 셀)은 empty <td>로 유지해야 레이아웃이 맞음
    inline constexpr bool EMIT_EMPTY_TD_FOR_HOLES = true;

    // hole로 찍는 td에 표시를 남길지 (LLM 파싱/디버깅에 도움)
    // 예: <td data-hwpx-empty="1"></td>
    inline constexpr bool TAG_EMPTY_TD = true;

    // =========================================================
    // Helper predicates
    // =========================================================
    inline constexpr bool IsTableRoot(std::uint32_t id) { return id == TABLE_ROOT_ID; }
    inline constexpr bool IsRowGroup(std::uint32_t id) { return id == ROW_GROUP_ID; }
    inline constexpr bool IsCellWrapper(std::uint32_t id) { return id == CELL_WRAPPER_ID; }

    // =========================================================
    // LIST PARA DUMP 
    // =========================================================
    inline constexpr bool DUMP_LIST_PARA_SUBTREE = false;   // 리스트 문단만 subtree 덤프
    inline constexpr int  DUMP_LIST_PARA_LIMIT = 50;       // 최대 N개 문단만
    inline constexpr int  DUMP_LIST_MAX_NODES = 400;       // dump safe 노드 제한

    // paraPrIDRef로 필터링 (너 xml에서 paraPrIDRef="20")
    inline constexpr bool DUMP_LIST_FILTER_BY_PARAPR = true;
    inline constexpr std::uint32_t LIST_PARA_PR_ID = 20;

} // namespace WalkerConfig
