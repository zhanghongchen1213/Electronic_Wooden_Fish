#!/usr/bin/env python3
"""阶段三 SquareLine 导出、binding、CMake 与符号静态闭合验证。"""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

from validate_font_coverage import validate_font_coverage


ROOT = Path(__file__).resolve().parents[3]
GENERATED = ROOT / "components/ui/generated"
BINDINGS = ROOT / "components/ui/bindings"
MANIFEST = ROOT / "lvgl-design/squareline_studio/project_manifest.json"
PROJECTION = BINDINGS / "ui_manifest_projection.c"

TEXT_SUFFIXES = {
    ".c",
    ".cmake",
    ".ecomp",
    ".h",
    ".json",
    ".md",
    ".py",
    ".spj",
    ".txt",
    ".yaml",
    ".yml",
}


def fail(message: str) -> None:
    print(f"ERROR: {message}")
    raise SystemExit(1)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def validate_firmware_version_contract() -> None:
    project_cmake = read(ROOT / "CMakeLists.txt")
    version_matches = list(re.finditer(
        r'^\s*set\s*\(\s*LEGBOT_DEFAULT_PROJECT_VER\s+"([^"]+)"\s*\)\s*$',
        project_cmake,
        flags=re.M,
    ))
    versions = [match.group(1) for match in version_matches]
    if len(versions) != 1 or re.fullmatch(r"v\d+\.\d+\.\d+", versions[0]) is None:
        fail(
            "顶层 LEGBOT_DEFAULT_PROJECT_VER 必须唯一且为带 v 的三段语义版本，"
            f"实际为 {versions}"
        )
    if re.search(
        r'set\s*\(\s*PROJECT_VER\s+"\$\{_legbot_effective_project_ver\}"\s+'
        r'CACHE\s+STRING\s+"[^"]*"\s+FORCE\s*\)',
        project_cmake,
        flags=re.S,
    ) is None:
        fail("顶层 PROJECT_VER 必须由有效版本强制刷新 ESP-IDF 缓存")
    version_position = version_matches[0].start()
    project_position = project_cmake.find("project(")
    if version_position < 0 or project_position < 0 or version_position > project_position:
        fail("LEGBOT_DEFAULT_PROJECT_VER 必须在顶层 project() 前定义")

    project_description_path = ROOT / "build/project_description.json"
    if project_description_path.exists():
        configured_version = json.loads(read(project_description_path)).get(
            "project_version"
        )
        expected_version = versions[0]
        cmake_cache_path = ROOT / "build/CMakeCache.txt"
        if cmake_cache_path.exists():
            override_match = re.search(
                r'^LEGBOT_PROJECT_VER_OVERRIDE:[^=]*=(.*)$',
                read(cmake_cache_path),
                flags=re.M,
            )
            if override_match is not None and override_match.group(1):
                expected_version = override_match.group(1)
        if configured_version != expected_version:
            fail(
                "构建目录中的固件版本已过期："
                f"期望为 {expected_version}，已配置值为 {configured_version}；"
                "请先执行 idf.py reconfigure"
            )

    services_cmake = read(ROOT / "components/services/CMakeLists.txt")
    if "esp_app_format" not in services_cmake:
        fail("services 组件缺少 esp_app_format 依赖")

    ui_service = normalize_c(
        strip_c_comments(read(ROOT / "components/services/ui_service/ui_service.c"))
    )
    for token in (
        "esp_app_get_description()",
        "app_description->version[0]!='\\0'",
        'app_description->version:"--"',
    ):
        if token not in ui_service:
            fail(f"ui_service 缺少运行镜像版本合同: {token}")


def validate_firmware_profile_contract() -> None:
    project_cmake = read(ROOT / "CMakeLists.txt")
    resolver_path = ROOT / "cmake/resolve_firmware_profile.cmake"
    test_path = ROOT / "cmake/profile_contract/test_firmware_profile.cmake"
    if not resolver_path.exists() or not test_path.exists():
        fail("固件编译模式缺少 resolver 或 CMake 合同测试")

    resolver = normalize_c(strip_c_comments(read(resolver_path)))
    for token in (
        'if(override_definedANDNOT"${override_value}"STREQUAL"")',
        'if(NOT"${override_value}"MATCHES"^[01]$")',
        'if(_project_majorSTREQUAL"1"AND"${override_value}"STREQUAL"1")',
        'PROJECT_VER=${project_version}禁止启用4G',
        'if(_project_majorSTREQUAL"1")set(_resolved_mode"0")',
        'elseif(_project_majorSTREQUAL"2")set(_resolved_mode"1")',
        'PROJECT_VER=${project_version}尚未定义景区管理模式默认值',
    ):
        if token not in resolver:
            fail(f"固件编译模式 resolver 缺少合同: {token}")

    compile_definition = (
        '"SCENIC_AREA_MANAGEMENT_DEBUG=${_legbot_scenic_mode}"'
    )
    resolver_position = project_cmake.find(
        "legbot_resolve_scenic_area_management_mode("
    )
    definition_position = project_cmake.find(compile_definition)
    project_position = project_cmake.find("project(legbot_watch)")
    if (
        min(resolver_position, definition_position, project_position) < 0
        or not resolver_position < definition_position < project_position
    ):
        fail("固件编译模式必须在 project() 前解析并发布全局 COMPILE_DEFINITIONS")

    profile_test = subprocess.run(
        ["cmake", "-P", str(test_path)],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if profile_test.returncode != 0:
        output = (profile_test.stdout + profile_test.stderr).strip()
        fail(f"固件编译模式 CMake 合同测试失败: {output}")

    app_main = read(ROOT / "main/app_main.c")
    for token in (
        "#if SCENIC_AREA_MANAGEMENT_DEBUG",
        "#if !LEGBOT_CAP_MODEM",
        "ml307r_bsp_hold_disabled();",
        "#if LEGBOT_CAP_MODEM",
        "return cloud_provision_apply();",
        "return at_core_init();",
        "蓝牙通信模式跳过 cloud 配置预置",
        "蓝牙通信模式跳过 ML307R AT 核心初始化",
    ):
        if token not in app_main:
            fail(f"启动流程缺少固件编译模式门禁: {token}")

    board_source = normalize_c(
        strip_c_comments(read(ROOT / "components/BSP/BOARD/bsp_board.c"))
    )
    for token in (
        "#ifLEGBOT_CAP_MODEM",
        "ml307r_bsp_enable()",
        "ml307r_bsp_hold_disabled()",
    ):
        if token not in board_source:
            fail(f"板级 4G 能力门禁缺少合同: {token}")

    modem_bsp_source = normalize_c(
        strip_c_comments(read(ROOT / "components/BSP/ML307R/ml307r_bsp.c"))
    )
    for token in (
        "configure_enable_level(LEGBOT_BSP_ML307R_EN_INACTIVE_LEVEL)",
        "returnESP_ERR_NOT_SUPPORTED;",
    ):
        if token not in modem_bsp_source:
            fail(f"ML307R BSP 缺少禁用 profile 低电平门禁: {token}")

    services_source = normalize_c(
        strip_c_comments(read(ROOT / "components/services/legbot_services.c"))
    )
    for token in (
        ".starts_by_default=LEGBOT_CAP_MODEM==1",
        ".starts_by_default=LEGBOT_CAP_GPS_TIME==1||"
        "LEGBOT_CAP_GPS_CONTINUOUS_LOCATION==1",
        ".starts_by_default=LEGBOT_CAP_CLOUD==1",
        "!service_allowed_by_profile(id)",
        "returnESP_ERR_NOT_SUPPORTED;",
        "if(id==LEGBOT_SERVICE_MODEM){returnLEGBOT_CAP_MODEM==1;}",
        "if(id==LEGBOT_SERVICE_GPS){returnLEGBOT_CAP_GPS_TIME==1||"
        "LEGBOT_CAP_GPS_CONTINUOUS_LOCATION==1;}",
        "if(id==LEGBOT_SERVICE_CLOUD){returnLEGBOT_CAP_CLOUD==1;}",
    ):
        if token not in services_source:
            fail(f"服务框架缺少固件编译模式资源门禁: {token}")

    selftest_source = strip_c_comments(
        read(ROOT / "components/services/selftest_service/selftest_service.c")
    )
    execute_item = normalize_c(c_function_body(selftest_source, "execute_item"))
    profile_skip = execute_item.find(
        "firmware_profile_skip_code(metadata->item_id)"
    )
    handler_lookup = execute_item.find(
        "find_registration(metadata->item_id)"
    )
    if profile_skip < 0 or handler_lookup < 0 or profile_skip > handler_lookup:
        fail("模式禁用自检项必须在查找和调用硬件 handler 前短路")
    for token in (
        "SELFTEST_OUTCOME_SKIP",
        "SELFTEST_REASON_PREREQUISITE",
        "GPS_DISABLED_BY_FIRMWARE_PROFILE",
        "MODEM_DISABLED_BY_FIRMWARE_PROFILE",
    ):
        if token not in selftest_source:
            fail(f"自检缺少固件编译模式跳过合同: {token}")

    runtime_source = normalize_c(
        strip_c_comments(read(BINDINGS / "ui_runtime_binding.c"))
    )
    for token in (
        "set_flag(cellular_icons[index],LV_OBJ_FLAG_HIDDEN,true)",
        "set_flag(gps_icons[index],LV_OBJ_FLAG_HIDDEN,true)",
        "set_x(ble_icons[index],82)",
        ":UI_STATUS_ICON_DISCONNECTED_COLOR)",
        "returnUI_RUNTIME_INTENT_SUBMIT_SERVICE;",
        'set_label(ui_ui_maintenance_row_cellular_value,"不可用")',
        "set_flag(ui_ui_maintenance_row_cellular,LV_OBJ_FLAG_CLICKABLE,"
        "LEGBOT_CAP_MODEM!=0)",
    ):
        if token not in runtime_source:
            fail(f"UI binding 缺少蓝牙固件模式投影: {token}")

    ui_service = read(ROOT / "components/services/ui_service/ui_service.c")
    normalized_ui_service = normalize_c(strip_c_comments(ui_service))
    for token in (
        "#if!LEGBOT_CAP_MODEM",
        "constboolcellular_connected=false;",
        "model->cellular_connecting=false;",
        'constchar*cellular_status="不可用";',
    ):
        if token not in normalized_ui_service:
            fail(f"v1 维护页 4G 灰态缺少能力门禁: {token}")
    alert_gate = re.search(
        r"#if\s+SCENIC_AREA_MANAGEMENT_DEBUG\s+"
        r"if\s*\(!cellular_connected\).*?"
        r"UI_RUNTIME_ALERT_CELLULAR.*?"
        r"UI_RUNTIME_ALERT_GPS.*?"
        r"#endif",
        ui_service,
        flags=re.S,
    )
    if alert_gate is None:
        fail("4G/GPS 提醒未受景区管理编译模式门禁")
    if (
        "#if !SCENIC_AREA_MANAGEMENT_DEBUG" not in ui_service
        or "maintenance_service_request_selftest()" not in ui_service
    ):
        fail("蓝牙固件模式缺少绕过 GPS 环境页的自检提交")

    ble_service = normalize_c(
        strip_c_comments(
            read(ROOT / "components/services/ble_service/ble_service.c")
        )
    )
    for token in (
        "try_start_automatic_authorization();",
        "ble_authorization_should_schedule(",
        "schedule_automatic_authorization_retry(",
        "BLE_AUTOMATIC_AUTHORIZATION_MAX_ATTEMPTS",
        ".result=WATCH_RENTAL_RESULT_PAID",
        "start_unlock_write(&unlock_action);",
        "s_automatic_authorization_attempt_generation=s_link_generation;",
        "atomic_fetch_add(&s_unlock_intent_sequence,1U)",
        ".unlock_intent_sequence=sequence",
        "output.intent_sequence=request->intent_sequence",
        ".intent_sequence=s_authorization_intent_sequence",
    ):
        if token not in ble_service:
            fail(f"BLE 缺少自动授权与解锁确认合同: {token}")
    automatic_owner_loop = (
        "#ifSCENIC_AREA_MANAGEMENT_DEBUG"
        "try_submit_pending_cloud_rental_request();"
        "handle_cloud_rental_results();"
        "#endif"
        "try_start_automatic_authorization();"
    )
    if automatic_owner_loop not in ble_service:
        fail("景区与蓝牙模式未共用每链路自动授权 owner 调度")
    invalidate_link = normalize_c(
        c_function_body(
            strip_c_comments(
                read(ROOT / "components/services/ble_service/ble_service.c")
            ),
            "invalidate_link_generation",
        )
    )
    for token in (
        "s_automatic_authorization_attempt_count=0U;",
        "s_automatic_authorization_retry_at_ms=0U;",
        "clear_pending_cloud_rental_submission();",
    ):
        if token not in invalidate_link:
            fail(f"新物理链路未重置自动授权重试状态: {token}")

    cloud_result = normalize_c(
        c_function_body(
            strip_c_comments(
                read(ROOT / "components/services/ble_service/ble_service.c")
            ),
            "handle_cloud_rental_result",
        )
    )
    paid_publish = cloud_result.find("publish_control_update_confirmed(&update)")
    paid_write = cloud_result.find("start_unlock_write(&action)")
    if paid_publish < 0 or paid_write < 0 or paid_publish > paid_write:
        fail("云端 PAID 未在控制状态确认发布成功后再发起 BLE 解锁")

    cloud_submission_retry = normalize_c(
        c_function_body(
            strip_c_comments(
                read(ROOT / "components/services/ble_service/ble_service.c")
            ),
            "try_submit_pending_cloud_rental_request",
        )
    )
    for token in (
        "cloud_service_rental_submit(&s_cloud_rental_submission,0)",
        "cloud_submission_retry_delay_ms(",
        "s_cloud_rental_submission_retry_at_ms=now_ms+retry_delay_ms;",
    ):
        if token not in cloud_submission_retry:
            fail(f"云租赁请求缺少同会话有界退避入队重试合同: {token}")
    for token in (
        "BLE_CLOUD_RENTAL_SUBMISSION_RETRY_DELAY_MS",
        "BLE_CLOUD_RENTAL_SUBMISSION_RETRY_MAX_DELAY_MS",
    ):
        if token not in ble_service:
            fail(f"云租赁请求缺少同会话退避上下界: {token}")

    normalized_ui_service = normalize_c(strip_c_comments(ui_service))
    for token in (
        "s_payment_link_generation!=watch.ble_link_generation",
        "watch.control_link_generation==watch.ble_link_generation",
        "ui_payment_result_matches(",
        "result.intent_sequence",
        "result.link_generation",
        "ble_service_request_first_unlock(&intent_sequence,0)",
        "s_payment_intent_sequence=intent_sequence",
        "ui_payment_projection_build(",
        "model->authorization_pending=",
        "model->payment_required=payment.payment_required",
        "model->payment_failed=payment.payment_failed",
        "model->payment_action_enabled=payment.payment_action_enabled",
    ):
        if token not in normalized_ui_service:
            fail(f"UI 缺少自动授权静默与失败降级合同: {token}")
    if "s_payment_request_id" in normalized_ui_service:
        fail("UI 仍使用 request_id 关联支付重试，可能被旧自动终态误清理")
    if "s_payment_submission_sequence" in normalized_ui_service:
        fail("UI 仍允许无 intent 关联的控制状态序号清理支付重试")

    payment_correlation = normalize_c(
        c_function_body(
            strip_c_comments(
                read(
                    ROOT
                    / "components/services/ui_service/ui_payment_correlation.c"
                )
            ),
            "ui_payment_result_matches",
        )
    )
    for token in (
        "pending&&",
        "pending_intent_sequence!=0U",
        "pending_link_generation!=0U",
        "result_intent_sequence==pending_intent_sequence",
        "result_link_generation==pending_link_generation",
    ):
        if token not in payment_correlation:
            fail(f"UI 支付终态缺少精确 intent/链路关联: {token}")

    payment_projection = normalize_c(
        c_function_body(
            strip_c_comments(
                read(
                    ROOT
                    / "components/services/ui_service/ui_payment_correlation.c"
                )
            ),
            "ui_payment_projection_build",
        )
    )
    for token in (
        "phase==WATCH_RENTAL_PHASE_WAITING_PAYMENT",
        "phase==WATCH_RENTAL_PHASE_RETRY_WAIT",
        "phase==WATCH_RENTAL_PHASE_BLOCKED",
        "last_result==WATCH_RENTAL_RESULT_UNPAID",
        "projection->payment_failed=",
        "projection->payment_action_enabled=",
        "(!blocked||legacy_unpaid)",
    ):
        if token not in payment_projection:
            fail(f"UI 支付中间态缺少既有 04-03/04-04 投影合同: {token}")

    payment_disabled = re.search(
        r"#else\s+"
        r"s_payment_verification_attempted\s*=\s*false\s*;.*?"
        r"model->payment_required\s*=\s*false\s*;.*?"
        r"model->payment_action_enabled\s*=\s*false\s*;"
        r"\s*#endif",
        ui_service,
        flags=re.S,
    )
    if payment_disabled is None:
        fail("蓝牙固件模式未在 UI 模型层关闭支付验证页与提交入口")

    flash_manual = read(ROOT / "docs/guides/Flash_watch.md")
    if (
        "蓝牙通信模式不启动 cloud，也不显示支付验证页" not in flash_manual
        or "只有确认成功后普通控制才会开放" not in flash_manual
    ):
        fail("固件打包文档缺少蓝牙模式本地授权与解锁确认说明")


def generated_object_symbols() -> set[str]:
    pattern = re.compile(r"(?:extern\s+)?lv_obj_t\s*\*\s*(ui_[A-Za-z0-9_]+)")
    symbols: set[str] = set()
    for path in GENERATED.rglob("*.[ch]"):
        symbols.update(pattern.findall(read(path)))
    return symbols


def validate_generated_display_geometry(manifest: dict) -> None:
    canvas = manifest.get("canvas", {})
    expected_canvas = {
        "width": 410,
        "height": 502,
        "shape": "rounded_rect",
        "corner_radius": 110,
        "critical_content_clearance": 8,
    }
    for key, value in expected_canvas.items():
        if canvas.get(key) != value:
            fail(f"manifest canvas.{key}={canvas.get(key)!r}，预期 {value!r}")

    generated = normalize_c(
        "\n".join(read(path) for path in (GENERATED / "screens").glob("*.c"))
    )
    screen_symbols = {
        "ui_" + screen
        for state in manifest["state_registry"]
        if (screen := state.get("screen") or state.get("authoring_template_host_screen"))
    }
    root_symbols = {
        "ui_" + state["page_state_root_object"]
        for state in manifest["state_registry"]
    }
    if len(screen_symbols) != 9 or len(root_symbols) != 13:
        fail(
            "manifest 显示根数量不闭合: "
            f"screens={len(screen_symbols)}, page_roots={len(root_symbols)}"
        )

    for symbol in sorted(screen_symbols | root_symbols):
        required = (
            f"lv_obj_set_style_radius({symbol},110,LV_PART_MAIN|LV_STATE_DEFAULT);",
            f"lv_obj_set_style_clip_corner({symbol},true,LV_PART_MAIN|LV_STATE_DEFAULT);",
        )
        for call in required:
            if call not in generated:
                fail(f"SquareLine 导出根缺少圆角显示调用: {call}")

    for symbol in sorted(root_symbols):
        for call in (
            f"lv_obj_set_width({symbol},410);",
            f"lv_obj_set_height({symbol},502);",
        ):
            if call not in generated:
                fail(f"SquareLine 导出页面根未保持 410×502: {call}")

    retry_root = "ui_ui_page_selftest_retry_detail_root"
    if f"lv_obj_set_scroll_dir({retry_root},LV_DIR_VER);" not in generated:
        fail("08-06 SquareLine 导出页面根未保持纵向滚动能力")

    result_list = "ui_ui_selftest_result_result_list"
    if f"lv_obj_set_scroll_dir({result_list},LV_DIR_VER);" not in generated:
        fail("08-04 SquareLine 导出结果列表未保持纵向滚动能力")
    if f"lv_obj_set_scrollbar_mode({result_list},LV_SCROLLBAR_MODE_OFF);" not in generated:
        fail("08-04 SquareLine 导出结果列表叠加了非设计滚动条")
    if re.search(
        rf"lv_obj_clear_flag\({result_list},[^;]*LV_OBJ_FLAG_SCROLLABLE",
        generated,
    ):
        fail("08-04 SquareLine 导出结果列表错误清除了 SCROLLABLE")

    for token in (
        "lv_obj_set_width(ui_ui_home_normal_icon_assist_state,42);",
        "lv_obj_set_height(ui_ui_home_normal_icon_assist_state,42);",
        "lv_obj_set_x(ui_ui_home_normal_icon_assist_state,16);",
        "lv_obj_set_y(ui_ui_home_normal_icon_assist_state,134);",
        "lv_obj_set_style_text_font(ui_ui_home_normal_txt_assist_state,&ui_font_ns700_31,LV_PART_MAIN|LV_STATE_DEFAULT);",
    ):
        if token not in generated:
            fail(f"首页助力状态导出几何或字体不符: {token}")

    device_info = normalize_c(
        read(GENERATED / "screens/ui_ui_scr_device_info.c")
    )
    for token in (
        'lv_label_set_text(ui_ui_device_info_row_firmware_value,"1.0.0");',
        "lv_obj_set_width(ui_ui_device_info_row_watch_id_label,160);",
        "lv_obj_set_x(ui_ui_device_info_row_watch_id_value,220);",
        "lv_obj_set_width(ui_ui_device_info_row_watch_id_value,142);",
        "lv_label_set_long_mode(ui_ui_device_info_row_watch_id_value,LV_LABEL_LONG_CLIP);",
        "lv_obj_set_width(ui_ui_device_info_row_bound_mac_label,130);",
        "lv_obj_set_x(ui_ui_device_info_row_bound_mac_value,198);",
        "lv_obj_set_width(ui_ui_device_info_row_bound_mac_value,164);",
        "lv_label_set_long_mode(ui_ui_device_info_row_bound_mac_value,LV_LABEL_LONG_CLIP);",
        'lv_label_set_text(ui_ui_device_info_row_left_leg_position_label,"左腿位置");',
        'lv_label_set_text(ui_ui_device_info_row_right_leg_position_label,"右腿位置");',
    ):
        if token not in device_info:
            fail(f"05-02 生成代码缺少设备标识、版本或腿位合同: {token}")


def resolver_fields(node: object) -> list[tuple[str, str]]:
    found: list[tuple[str, str]] = []
    if isinstance(node, dict):
        resolver = node.get("required_phase3_resolver")
        if isinstance(resolver, dict):
            found.append((resolver["root"], resolver["field"]))
        for value in node.values():
            found.extend(resolver_fields(value))
    elif isinstance(node, list):
        for value in node:
            found.extend(resolver_fields(value))
    return found


def enum_name(state_key: str) -> str:
    return "UI_MANIFEST_STATE_" + re.sub(r"[^A-Z0-9]+", "_", state_key.upper())


def target_expression(
    descriptor: dict,
    root_symbol: str,
    component_by_guid: dict[str, str],
) -> str:
    target = descriptor["target"]
    component_path = target.get("component_path")
    if component_path and len(component_path.get("coid_path", [])) > 1:
        component_name = component_by_guid.get(component_path["cid"])
        if component_name is None:
            fail(f"manifest 引用了未知组件 GUID: {component_path['cid']}")
        macro = "UI_COMP_" + component_name.upper() + "_" + target["logical_name"].upper()
        return f"ui_comp_get_child({root_symbol}, {macro})"
    return "ui_" + target["logical_name"]


def walk_descriptor_targets(
    descriptor: dict,
    component_by_guid: dict[str, str],
    component_root_symbol: str = "",
):
    target = descriptor["target"]
    component_path = target.get("component_path")
    next_component_root = component_root_symbol
    if component_path and len(component_path.get("coid_path", [])) == 1:
        next_component_root = "ui_" + target["logical_name"]
    elif not component_path:
        next_component_root = ""
    expression = target_expression(
        descriptor,
        next_component_root,
        component_by_guid,
    )
    yield descriptor, expression
    for child in descriptor.get("children", []):
        yield from walk_descriptor_targets(
            child,
            component_by_guid,
            next_component_root,
        )


def manifest_operation_call(expression: str, operation: dict) -> str:
    op = operation.get("op")
    args = operation.get("args", [])
    if op == "lv_obj_set_pos" and len(args) == 2:
        return f"{op}({expression},{args[0]},{args[1]});"
    if op == "lv_obj_set_size" and len(args) == 2:
        return f"{op}({expression},{args[0]},{args[1]});"
    if op in {"lv_obj_add_flag", "lv_obj_clear_flag"} and args == ["HIDDEN"]:
        return f"{op}({expression},LV_OBJ_FLAG_HIDDEN);"
    if op in {"lv_obj_add_state", "lv_obj_clear_state"} and args in (
        ["DISABLED"],
        ["CHECKED"],
    ):
        return f"{op}({expression},LV_STATE_{args[0]});"
    if op == "lv_label_set_text" and len(args) == 1:
        text = args[0].replace("\\n", "\n")
        return f"{op}({expression},{json.dumps(text, ensure_ascii=False)});"
    if op in {
        "lv_obj_set_style_text_color",
        "lv_obj_set_style_bg_color",
        "lv_obj_set_style_border_color",
    } and len(args) >= 3:
        color = f"0x{args[0]:02X}{args[1]:02X}{args[2]:02X}"
        return (
            f"{op}({expression},lv_color_hex({color}),"
            "LV_PART_MAIN|LV_STATE_DEFAULT);"
        )
    if op == "lv_obj_set_style_text_font" and len(args) == 1:
        return (
            f"{op}({expression},&ui_font_{args[0]},"
            "LV_PART_MAIN|LV_STATE_DEFAULT);"
        )
    if op == "lv_obj_set_style_text_align" and len(args) == 1:
        return (
            f"{op}({expression},LV_TEXT_ALIGN_{args[0]},"
            "LV_PART_MAIN|LV_STATE_DEFAULT);"
        )
    if op in {
        "lv_obj_set_style_bg_opa",
        "lv_obj_set_style_radius",
        "lv_obj_set_style_border_width",
    } and len(args) == 1:
        return (
            f"{op}({expression},{args[0]},"
            "LV_PART_MAIN|LV_STATE_DEFAULT);"
        )
    fail(f"manifest 包含验证器不支持的投影操作: {operation}")
    return ""


def normalize_c(text: str) -> str:
    return re.sub(r"\s+", "", text)


def strip_c_comments(text: str) -> str:
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S)


def projection_case(source: str, state_key: str) -> str:
    enum = enum_name(state_key)
    match = re.search(
        rf"\bcase\s+{re.escape(enum)}\s*:(.*?)(?=\n\s*(?:case\s+|default\s*:))",
        source,
        flags=re.S,
    )
    if match is None:
        fail(f"projection 缺少状态 case: {state_key}")
    body = match.group(1)
    if "return true;" not in body:
        fail(f"projection 状态未成功闭合: {state_key}")
    return normalize_c(body)


def validate_projection_semantics(manifest: dict) -> None:
    source = read(PROJECTION)
    if "ui_projection_target_t" in source or "switch (state)" not in source:
        fail("projection 仍是 baseline-only 表结构，未生成逐状态完整 switch/calls")
    if "ui_comp_get_child(ui_ui_selftest_result_result_list," in normalize_c(source):
        fail("projection 把 08-04 普通列表容器误当成组件实例")

    component_by_guid = {
        item["component_guid"]: item["squareline_component"]
        for item in manifest["component_registry"]
    }
    component_root_symbols: set[str] = set()

    def collect_component_roots(descriptor: dict) -> None:
        target = descriptor["target"]
        component_path = target.get("component_path")
        if component_path and len(component_path.get("coid_path", [])) == 1:
            component_root_symbols.add("ui_" + target["logical_name"])
        for child in descriptor.get("children", []):
            collect_component_roots(child)

    for state in manifest["state_registry"]:
        for override in state["state_patch"]["overrides"].values():
            collect_component_roots(override)
    projection_component_roots = set(
        re.findall(r"ui_comp_get_child\((ui_[A-Za-z0-9_]+),", normalize_c(source))
    )
    unexpected_component_roots = projection_component_roots - component_root_symbols
    if unexpected_component_roots:
        fail(
            "projection 的组件子对象使用了非组件根: "
            + ", ".join(sorted(unexpected_component_roots))
        )

    total_show = 0
    for state in manifest["state_registry"]:
        state_key = state["state_key"]
        patch = state.get("state_patch", {})
        required = {"baseline_reset", "show", "hide", "overrides"}
        missing = required - set(patch)
        if missing:
            fail(f"manifest 状态 {state_key} 缺少 patch 字段: {sorted(missing)}")
        body = projection_case(source, state_key)
        show_names = {item["logical_name"] for item in patch["show"]}
        hide_names = {item["logical_name"] for item in patch["hide"]}
        if not show_names:
            fail(f"manifest 状态没有任何可见对象，疑似 all-hidden: {state_key}")
        total_show += len(show_names)

        for logical_name in show_names:
            expected = normalize_c(
                f"lv_obj_clear_flag(ui_{logical_name}, LV_OBJ_FLAG_HIDDEN);"
            )
            if expected not in body:
                fail(f"projection 未落实 {state_key}.show: {logical_name}")
        for logical_name in hide_names:
            expected = normalize_c(
                f"lv_obj_add_flag(ui_{logical_name}, LV_OBJ_FLAG_HIDDEN);"
            )
            if expected not in body:
                fail(f"projection 未落实 {state_key}.hide: {logical_name}")

        for descriptor in patch["baseline_reset"]:
            expression = target_expression(descriptor, "", component_by_guid)
            for operation in descriptor["operations"]:
                expected = normalize_c(manifest_operation_call(expression, operation))
                if expected not in body:
                    fail(
                        f"projection 缺少 {state_key}.baseline_reset 操作: "
                        f"{operation.get('op')}({expression})"
                    )
        for override in patch["overrides"].values():
            for descriptor, expression in walk_descriptor_targets(
                override,
                component_by_guid,
            ):
                for operation in descriptor["operations"]:
                    expected = normalize_c(manifest_operation_call(expression, operation))
                    if expected not in body:
                        fail(
                            f"projection 缺少 {state_key}.overrides 操作: "
                            f"{operation.get('op')}({expression})"
                        )
    if total_show == 0 or "lv_obj_clear_flag" not in source:
        fail("projection 没有任何 show 操作，疑似 all-hidden")


def resolver_initializer(source: str) -> tuple[str, str]:
    match = re.search(
        r"static\s+const\s+ui_phase3_resolver_t\s+"
        r"s_phase3_resolvers\s*\[[^]]+\]\s*=\s*\{(.*?)\n\s*\};",
        source,
        flags=re.S,
    )
    if match is None:
        fail("binding 缺少 s_phase3_resolvers 初始化块")
    actual_source = source[: match.start()] + source[match.end() :]
    return match.group(1), actual_source


def c_function_body(source: str, name: str) -> str:
    match = re.search(
        rf"^[ \t]*(?:static[ \t]+)?(?:const[ \t]+)?"
        rf"[A-Za-z_][A-Za-z0-9_]*[ \t]+(?:\*[ \t]*)?"
        rf"{re.escape(name)}\s*"
        rf"\([^;{{}}]*\)\s*\{{",
        source,
        flags=re.M | re.S,
    )
    if match is None:
        fail(f"找不到 resolver consumer 函数定义: {name}")
    opening = source.find("{", match.start())
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : index]
    fail(f"resolver consumer 函数未闭合: {name}")
    return ""


def validate_resolver_evidence(manifest: dict, runtime_source: str) -> None:
    initializer, actual_source = resolver_initializer(runtime_source)
    actual_source = strip_c_comments(actual_source)
    source_resolvers = re.findall(
        r'\{"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*'
        r'"([^"]+)",\s*"([^"]+)"\}',
        initializer,
    )
    manifest_resolvers = resolver_fields(manifest)
    if len(manifest_resolvers) != 37 or len(source_resolvers) != 37:
        fail(
            f"resolver 数量不闭合: manifest={len(manifest_resolvers)}, "
            f"binding={len(source_resolvers)}"
        )
    if sorted(manifest_resolvers) != sorted(
        (root, field) for root, field, _, _, _ in source_resolvers
    ):
        fail("37 个 resolver 的 root/field 与 manifest 不一致")

    derived_field_evidence = {
        ("ui_page_selftest_gps_context_root", "gps_context"):
            r"\bs_gps_return_page\b",
        ("ui_page_selftest_retry_detail_root", "failed_items"):
            r"\bfailed_category_at\s*\(\s*model\b",
        ("ui_page_selftest_retry_detail_root", "retry_state"):
            r"\bmodel\s*->\s*retry_running\b",
    }
    generated_symbols = generated_object_symbols()
    for root, field, selector, operation, consumer in source_resolvers:
        if f"ui_{selector}" not in generated_symbols:
            fail(
                f"resolver selector 不存在于 SquareLine 导出对象: "
                f"{root}.{field} -> {selector}"
            )
        consumer_body = c_function_body(actual_source, consumer)
        field_pattern = derived_field_evidence.get(
            (root, field),
            rf"\bmodel\s*->\s*{re.escape(field)}\b",
        )
        if re.search(field_pattern, consumer_body) is None:
            fail(
                f"resolver consumer 未在函数范围内消费字段: "
                f"{root}.{field} -> {consumer}"
            )
        if re.search(rf"\b{re.escape(operation)}\b", consumer_body) is None:
            fail(
                f"resolver consumer 未在同一函数范围执行声明操作: "
                f"{root}.{field} -> {consumer}.{operation}"
            )


def validate_route_semantics(runtime_source: str) -> None:
    source = strip_c_comments(runtime_source)
    render = c_function_body(source, "ui_runtime_binding_render")
    accept = c_function_body(source, "ui_runtime_binding_accept_intent")
    route_back = c_function_body(source, "route_back")
    selftest_page = c_function_body(source, "selftest_page_for_model")
    advance = c_function_body(source, "advance_selftest_route")
    resolve_state = c_function_body(source, "resolve_static_state")
    retry_detail = c_function_body(source, "set_retry_detail")
    screen_init = c_function_body(source, "screen_init")
    projection = c_function_body(source, "apply_model_projection")
    clear_modal = c_function_body(source, "show_clear_binding_modal")

    selftest_page_code = normalize_c(selftest_page)
    running_evidence = selftest_page_code.find(
        "model->selftest_running||model->retry_running"
    )
    finished_evidence = selftest_page_code.find(
        "model->selftest_results.state==SELFTEST_RUN_FINISHED"
    )
    if (
        running_evidence < 0
        or finished_evidence < 0
        or running_evidence > finished_evidence
    ):
        fail("自检入口必须让当前运行/重试事实优先于历史 FINISHED 摘要")

    resolve_state_code = normalize_c(resolve_state)
    for token in (
        "if(!model->selftest_running&&!model->retry_running){"
        "if(model->selftest_results.state==SELFTEST_RUN_FINISHED)",
        "caseUI_PAGE_SELFTEST_GPS_CONTEXT:"
        "if(model->selftest_running||model->retry_running)",
    ):
        if token not in resolve_state_code:
            fail(f"静态状态解析未让当前 retry_running 压过旧 FINISHED: {token}")

    for token in (
        "advance_selftest_route(model)",
        "s_last_ble_connected",
        "ui_navigation_is_shell_page(s_current_page)",
        "shell_page_for_slot(s_shell_slot)",
        "redraw_variant_transition",
        "request_full_redraw()",
    ):
        if normalize_c(token) not in normalize_c(render):
            fail(f"render 路由矩阵缺少事实边沿语义: {token}")

    open_selftest = re.search(
        r"case\s+UI_INTENT_OPEN_SELFTEST\s*:(.*?)case\s+UI_INTENT_OPEN_CLEAR_BINDING",
        accept,
        flags=re.S,
    )
    if open_selftest is None or "route_to(selftest_page_for_model(model))" not in normalize_c(
        open_selftest.group(1)
    ):
        fail("OPEN_SELFTEST 未按真实自检状态解析入口")

    retry_case = re.search(
        r"case\s+UI_INTENT_RETRY_FAILED_ITEM\s*:(.*?)default\s*:",
        accept,
        flags=re.S,
    )
    if retry_case is None:
        fail("缺少单项自检重试路由 case")
    retry_code = normalize_c(retry_case.group(1))
    for token in (
        "s_gps_return_page=UI_PAGE_SELFTEST_RESULT",
        "route_to(UI_PAGE_SELFTEST_GPS_CONTEXT)",
    ):
        if token not in retry_code:
            fail(f"GPS 单项重试缺少来源路由语义: {token}")
    if retry_code.count("route_to(") != 1:
        fail("非 GPS 单项重试不得在 owner 接受前预跳 08-02/08-03")

    gps_back = re.search(
        r"case\s+UI_PAGE_SELFTEST_GPS_CONTEXT\s*:(.*?)case\s+UI_PAGE_SELFTEST_RETRY_DETAIL",
        route_back,
        flags=re.S,
    )
    if gps_back is None or "route_to(s_gps_return_page)" not in normalize_c(
        gps_back.group(1)
    ):
        fail("08-05 返回未恢复 08-01/08-04 来源")

    for page in (
        "UI_PAGE_SELFTEST_IDLE",
        "UI_PAGE_SELFTEST_RUNNING",
        "UI_PAGE_SELFTEST_MANUAL",
        "UI_PAGE_SELFTEST_RESULT",
        "UI_PAGE_SELFTEST_RETRY_DETAIL",
        "UI_PAGE_SELFTEST_GPS_CONTEXT",
    ):
        if page not in advance:
            fail(f"自检内部事实推进矩阵缺少页面: {page}")

    for token in (
        "ui_selftest_category_name(category)",
        "ui_ui_selftest_retry_detail_lbl_retry_reason",
        "ui_ui_selftest_retry_detail_val_retry_reason",
        "LV_OBJ_FLAG_HIDDEN",
    ):
        if token not in retry_detail:
            fail(f"08-06 类别/内部字段隐藏语义缺失: {token}")

    for body_name, body in (("screen_init", screen_init),
                            ("apply_model_projection", projection)):
        for token in (
            "ui_ui_selftest_manual_val_manual_countdown",
            "ui_ui_selftest_manual_lbl_manual_seconds",
            "ui_ui_selftest_manual_txt_manual_action_hint",
        ):
            if token not in body:
                fail(f"{body_name} 缺少振动重放中心热区: {token}")

    for token in (
        "model->clear_binding_failed",
        '"清除失败"',
        '"重试清除"',
        '"正在清除"',
        '"清除中"',
    ):
        if token not in clear_modal:
            fail(f"09-01 缺少 idle/clearing/failed 视觉状态: {token}")

    if re.search(
        r'strcmp\s*\(\s*model\s*->\s*device_model\s*,\s*"Legbot Watch"\s*\)'
        r'\s*==\s*0',
        projection,
    ) is None:
        fail("05-02 device_model 未真实参与标准型号条件判断")
    for label in ('"手环版本"', '"设备版本"'):
        if label not in projection:
            fail(f"05-02 设备型号标签基线或回退缺失: {label}")


def validate_ui_service_semantics(ui_service_source: str) -> None:
    source = strip_c_comments(ui_service_source)
    build_model_source = c_function_body(source, "build_runtime_model")
    build_model = normalize_c(build_model_source)
    handle_intent = normalize_c(c_function_body(source, "handle_ui_intent"))
    owner_loop = normalize_c(c_function_body(source, "ui_service_run"))
    display_flush = normalize_c(c_function_body(source, "display_flush"))
    display_wait = normalize_c(c_function_body(source, "display_wait"))
    complete_flush = normalize_c(
        c_function_body(source, "complete_pending_flush")
    )
    wait_fast = normalize_c(
        c_function_body(source, "wait_for_fast_completion")
    )
    render_shell = normalize_c(
        c_function_body(source, "render_shell_fast_frame")
    )
    compose_shell = normalize_c(
        c_function_body(source, "compose_shell_fast_chunk")
    )
    log_shell = normalize_c(
        c_function_body(source, "log_shell_performance_report")
    )

    if re.search(
        r'copy_text\s*\(\s*model\s*->\s*device_model\s*,\s*'
        r'sizeof\s*\(\s*model\s*->\s*device_model\s*\)\s*,\s*'
        r'"Legbot Watch"\s*\)',
        build_model_source,
    ) is None:
        fail("ui_service 模型缺少 Legbot Watch 设备型号真实来源")

    for token in (
        "s_clear_binding_submission_refresh_pending",
        "watch.ble_transaction==WATCH_BLE_TRANSACTION_ERROR",
        "s_clear_binding_failed=true",
        "model->clear_binding_failed=s_clear_binding_failed",
    ):
        if token not in build_model:
            fail(f"ui_service 模型缺少型号来源或清绑 owner 终态锁存: {token}")

    for token in (
        "s_clear_binding_submitted=true",
        "s_clear_binding_submission_refresh_pending=true",
        "s_clear_binding_failed=false",
        "s_clear_binding_failed=true",
    ):
        if token not in handle_intent:
            fail(f"ui_service 清绑提交缺少 clearing/failed 状态转换: {token}")
    for token in (
        "constboollocal_shell_navigation="
        "is_local_shell_navigation_intent(intent->type)",
        "if(local_shell_navigation&&s_last_rendered_model_valid)",
        "if(local_shell_navigation){return;}",
    ):
        if token not in handle_intent:
            fail(f"常驻页纯本地导航仍可能重复争用状态快照: {token}")

    guard_begin = display_flush.find("ui_display_flush_guard_begin(")
    draw_submit = display_flush.find("co5300_bsp_draw_bitmap(")
    if guard_begin < 0 or draw_submit < 0 or guard_begin > draw_submit:
        fail("普通 LVGL flush 必须在颜色事务提交前绑定完成代次")
    completion_check = complete_flush.find(
        "ui_display_flush_guard_completed("
    )
    flush_ready = complete_flush.find("lv_disp_flush_ready(")
    if (
        completion_check < 0
        or flush_ready < 0
        or completion_check > flush_ready
    ):
        fail("普通 LVGL flush 必须先验证匹配代次再归还绘制缓冲")
    for token in (
        "ui_display_flush_guard_timed_out(",
        "UI_LVGL_FLUSH_TIMEOUT_US",
        "reset_display_touch(true)",
    ):
        if token not in display_wait:
            fail(f"LVGL flush 永久等待保护缺少合同: {token}")
    if "(void)ulTaskNotifyTake(pdTRUE,0)" not in wait_fast:
        fail("硬件快路完成后必须清除本笔通知")
    if "esp_rom_delay_us(" in wait_fast:
        fail("有界 panel IO 已负责事务回收，快路不得保留逐块固定延时")

    if "render_shell_fast_frame()" not in owner_loop:
        fail("ui_task owner 循环必须调度常驻横滑快照直传")
    for token in (
        "ui_runtime_binding_shell_fast_frame(&frame)",
        "compose_shell_fast_chunk(destination,&frame,first_y,line_count)",
        "wait_for_fast_completion(pending_generation)",
        "co5300_bsp_draw_bitmap(",
        "ui_performance_metrics_record_frame("
        "&s_shell_performance_metrics",
        "record_shell_fast_path_miss(",
        "abort_hardware_fast_path(true)",
    ):
        if token not in render_shell:
            fail(f"常驻横滑快照直传缺少 UI owner 合同: {token}")
    final_wait = render_shell.rfind(
        "wait_for_fast_completion(pending_generation)"
    )
    frame_record = render_shell.find(
        "ui_performance_metrics_record_frame("
        "&s_shell_performance_metrics"
    )
    if final_wait < 0 or frame_record < 0 or frame_record < final_wait:
        fail("常驻横滑只能在末块 DMA 完成后记录完整帧")
    for token in (
        "frame->current_pixels",
        "frame->left_pixels",
        "frame->right_pixels",
        "s_shell_round_insets[y]",
        "memcpy(",
        "memset(",
    ):
        if token not in compose_shell:
            fail(f"常驻横滑分块合成缺少合同: {token}")
    if "路径=常驻快照直传" not in log_shell:
        fail("常驻横滑性能汇总必须标识快照直传路径")


def validate_ble_control_priority(ble_service_source: str) -> None:
    source = strip_c_comments(ble_service_source)
    request_control = normalize_c(
        c_function_body(source, "ble_service_request_control")
    )
    owner_loop = normalize_c(c_function_body(source, "ble_service_run"))
    mailbox_handler = normalize_c(
        c_function_body(source, "handle_priority_poweroff_mailbox")
    )
    preempt = normalize_c(
        c_function_body(source, "preempt_ordinary_control_for_poweroff")
    )

    for token in (
        "intent->kind==WATCH_CONTROL_KIND_POWEROFF&&intent->target==1U",
        "!priority_poweroff&&atomic_load(&s_control_result_backpressured)",
        "atomic_compare_exchange_strong(&s_poweroff_mailbox_sequence,"
        "&empty_sequence,(uint_fast32_t)intent_sequence)",
    ):
        if token not in request_control:
            fail(f"BLE 关机优先邮箱准入语义缺失: {token}")
    if "atomic_store(&s_poweroff_mailbox_sequence,intent_sequence)" in request_control:
        fail("BLE 关机优先邮箱不得覆盖已返回 ESP_OK 的未消费请求")

    if "handle_priority_poweroff_mailbox()" not in owner_loop:
        fail("BLE owner 循环未消费独立关机优先邮箱")
    for token in (
        "atomic_exchange(&s_poweroff_mailbox_sequence,0U)",
        "handle_control_intent(&event)",
    ):
        if token not in mailbox_handler:
            fail(f"BLE owner 关机优先邮箱消费语义缺失: {token}")
    if "publish_control_update_confirmed(&update)" in preempt:
        fail("关机抢占普通控制不得被普通状态 apply-ACK 门禁阻塞")
    for token in (
        "control_gate_abort_pending(&candidate_gate,&update)",
        "publish_control_update(&update)",
    ):
        if token not in preempt:
            fail(f"关机抢占普通控制状态清理语义缺失: {token}")


def validate_internal_selftest_privacy(runtime_source: str) -> None:
    runtime_code = strip_c_comments(runtime_source)
    for forbidden_field in ("reason", "detail_code", "elapsed_ms"):
        if re.search(rf"\b{forbidden_field}\b", runtime_code):
            fail(f"runtime binding 禁止投影内部自检字段: {forbidden_field}")


def validate_all_page_frame_rate_contract() -> None:
    runtime_source = strip_c_comments(
        read(ROOT / "components/ui/bindings/ui_runtime_binding.c")
    )
    runtime = normalize_c(runtime_source)
    service = normalize_c(
        strip_c_comments(
            read(ROOT / "components/services/ui_service/ui_service.c")
        )
    )
    for token in (
        "#defineUI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT2U",
        "transient_bytes>UI_SETTINGS_SNAPSHOT_TRANSIENT_BUDGET_BYTES",
        "LV_OBJ_FLAG_SCROLL_CHAIN|LV_OBJ_FLAG_SCROLL_ELASTIC",
        "ui_runtime_binding_shell_fast_frame",
        "ui_runtime_binding_settings_fast_frame",
        "ui_runtime_binding_selftest_result_fast_frame",
        "ui_runtime_binding_abort_fast_path",
        "update_touch_progress_local(",
        "switch(screen_for_page(s_current_page))",
    ):
        if token not in runtime:
            fail(f"全页面帧率合同缺少 binding 语义: {token}")
    for token in (
        "render_shell_fast_frame()",
        "render_settings_fast_frame()",
        "render_selftest_result_fast_frame()",
        "ui_runtime_binding_motion_active()",
        "ui_performance_metrics_record_frame("
        "&s_shell_performance_metrics",
        "ui_performance_metrics_record_frame("
        "&s_settings_performance_metrics",
        "ui_performance_metrics_record_frame("
        "&s_selftest_result_performance_metrics",
        "record_shell_fast_path_miss(",
        "record_settings_fast_path_miss(",
        "record_selftest_result_fast_path_miss(",
        "abort_hardware_fast_path(true)",
        "ui_runtime_binding_warm_settings_snapshot()",
        "ui_runtime_binding_warm_selftest_result_snapshot()",
    ):
        if token not in service:
            fail(f"全页面帧率合同缺少 UI 服务语义: {token}")

    scroll_owners = (
        (
            "ui_ui_main_shell_pager",
            "shell_scroll_end_event_cb",
            "常驻四页横滑",
        ),
        (
            "ui_ui_settings_settings_list_viewport",
            "settings_scroll_event_cb",
            "设置七栏纵滑",
        ),
        (
            "ui_ui_selftest_result_result_list",
            "selftest_result_scroll_event_cb",
            "自检结果纵滑",
        ),
    )
    for object_name, callback, description in scroll_owners:
        for event in (
            "LV_EVENT_SCROLL_BEGIN",
            "LV_EVENT_SCROLL",
            "LV_EVENT_SCROLL_END",
        ):
            token = (
                f"lv_obj_add_event_cb({object_name},{callback},"
                f"{event},NULL);"
            )
            if token not in runtime:
                fail(f"{description} 缺少运动事件闭环: {event}")

    for event in (
        "LV_EVENT_SCROLL_BEGIN",
        "LV_EVENT_SCROLL",
        "LV_EVENT_SCROLL_END",
    ):
        registrations = re.findall(
            rf"lv_obj_add_event_cb\([^;]*,{event},[^;]*\);",
            runtime,
        )
        if len(registrations) != len(scroll_owners):
            fail(
                f"{event} 注册对象不在三条连续滚动白名单内: "
                f"实际 {len(registrations)}，预期 {len(scroll_owners)}"
            )

    temporary_roots = (
        "ui_ui_page_settings_root",
        "ui_ui_page_device_info_root",
        "ui_ui_page_maintenance_root",
        "ui_ui_page_ble_root",
        "ui_ui_page_selftest_progress_root",
        "ui_ui_page_selftest_result_root",
        "ui_ui_page_selftest_gps_context_root",
        "ui_ui_page_selftest_retry_detail_root",
    )
    root_normalization = normalize_c(
        c_function_body(
            runtime_source,
            "remove_redundant_temporary_clipping",
        )
    )
    for root in temporary_roots:
        if root not in root_normalization:
            fail(f"按需页面根未纳入静态去滚动合同: {root}")
    for token in (
        "LV_OBJ_FLAG_SCROLLABLE|"
        "LV_OBJ_FLAG_SCROLL_ELASTIC|"
        "LV_OBJ_FLAG_SCROLL_MOMENTUM|"
        "LV_OBJ_FLAG_SCROLL_CHAIN",
        "lv_obj_set_style_clip_corner(root,false,"
        "LV_PART_MAIN|LV_STATE_DEFAULT)",
    ):
        if token not in root_normalization:
            fail(f"按需页面根静态化缺少合同: {token}")

    screen_init = normalize_c(
        c_function_body(runtime_source, "screen_init")
    )
    if (
        screen_init.count("remove_redundant_temporary_clipping(screen);")
        != len(temporary_roots)
    ):
        fail("9 个 Screen 中除主壳外的 8 个按需页必须逐一关闭整页滚动")
    if "lv_scr_load_anim" in runtime:
        fail("Screen 切换不得绕过性能审查引入整屏 LVGL 动画")


def validate_selftest_runtime_edges() -> None:
    config_header = normalize_c(
        read(ROOT / "components/platform/config_service/config_service.h")
    )
    config_source = strip_c_comments(
        read(ROOT / "components/platform/config_service/config_service.c")
    )
    selftest_source = strip_c_comments(
        read(ROOT / "components/services/selftest_service/selftest_service.c")
    )
    ui_source = strip_c_comments(
        read(ROOT / "components/services/ui_service/ui_service.c")
    )

    for token in (
        '#defineCONFIG_SERVICE_DIAG_NAMESPACE"diag"',
        "config_service_selftest_running_read(bool*running,",
        "config_service_selftest_running_write(boolrunning,",
    ):
        if token not in config_header:
            fail(f"config owner 缺少自检 dirty marker 合同: {token}")

    config_read = normalize_c(
        c_function_body(config_source, "config_service_selftest_running_read")
    )
    config_write = normalize_c(
        c_function_body(config_source, "config_service_selftest_running_write")
    )
    for token in (
        "nvs_open(CONFIG_SERVICE_DIAG_NAMESPACE,NVS_READONLY,&handle)",
        "nvs_get_u8(handle,CONFIG_SERVICE_SELFTEST_RUNNING_KEY,&stored_running)",
    ):
        if token not in config_read:
            fail(f"config owner 自检 dirty marker 读取语义缺失: {token}")
    for token in (
        "nvs_open(CONFIG_SERVICE_DIAG_NAMESPACE,NVS_READWRITE,&handle)",
        "nvs_set_u8(handle,CONFIG_SERVICE_SELFTEST_RUNNING_KEY,running?1U:0U)",
        "nvs_commit(handle)",
    ):
        if token not in config_write:
            fail(f"config owner 自检 dirty marker 写入语义缺失: {token}")

    execute_all = normalize_c(c_function_body(selftest_source, "execute_all"))
    set_dirty = execute_all.find(
        "config_service_selftest_running_write(true,pdMS_TO_TICKS(20))"
    )
    publish_begin = execute_all.find("state_service_publish_selftest_begin(")
    publish_finish = execute_all.find("state_service_publish_selftest_finish(")
    clear_dirty = execute_all.rfind(
        "config_service_selftest_running_write(false,pdMS_TO_TICKS(20))"
    )
    if min(set_dirty, publish_begin, publish_finish, clear_dirty) < 0 or not (
        set_dirty < publish_begin < publish_finish < clear_dirty
    ):
        fail("整机自检 dirty marker 未满足置位→begin→terminal→清理顺序")
    if "if(terminal_published){" not in execute_all:
        fail("整机自检 dirty marker 清理未受 terminal 发布成功门禁")

    ui_init = normalize_c(c_function_body(ui_source, "ui_service_init"))
    for token in (
        "config_service_selftest_running_read(&previous_running,pdMS_TO_TICKS(20))",
        "s_previous_selftest_incomplete=previous_running",
        "config_service_selftest_running_write(false,pdMS_TO_TICKS(20))",
    ):
        if token not in ui_init:
            fail(f"UI 启动消费自检 dirty marker 语义缺失: {token}")

    build_model = normalize_c(c_function_body(ui_source, "build_runtime_model"))
    for token in (
        'constchar*selftest_status="尚未运行"',
        'elseif(s_previous_selftest_incomplete){selftest_status="上次未完成";}',
    ):
        if token not in build_model:
            fail("UI 未将遗留 dirty marker 仅投影为“上次未完成”")

    handle_request = normalize_c(c_function_body(ui_source, "handle_request"))
    sleep_case = re.search(
        r"caseUI_SERVICE_REQUEST_SLEEP:(.*?)caseUI_SERVICE_REQUEST_DISPLAY_RESET:",
        handle_request,
        flags=re.S,
    )
    if sleep_case is None:
        fail("UI 请求处理缺少 SLEEP 自检常亮门禁")
    for token in (
        "watch.selftest.state==SELFTEST_RUN_RUNNING",
        "watch.selftest.retry_active",
        "resume_display_with_single_retry()",
    ):
        if token not in sleep_case.group(1):
            fail(f"UI SLEEP 自检常亮门禁语义缺失: {token}")

    flush_wait = sleep_case.group(1).find(
        "wait_for_pending_flush(UI_FULL_REDRAW_FLUSH_TIMEOUT_MS)"
    )
    display_suspend = sleep_case.group(1).find(
        "suspend_display_with_single_retry()"
    )
    if flush_wait < 0 or display_suspend < 0 or flush_wait > display_suspend:
        fail("UI 熄屏必须先等待 LVGL flush 完成，再执行 AMOLED 低功耗休眠")

    suspend_helper = normalize_c(
        c_function_body(ui_source, "suspend_display_with_single_retry")
    )
    resume_helper = normalize_c(
        c_function_body(ui_source, "resume_display_with_single_retry")
    )
    if suspend_helper.count("co5300_bsp_suspend()") != 2:
        fail("AMOLED 休眠失败只能执行唯一一次重试")
    if resume_helper.count("co5300_bsp_resume()") != 2:
        fail("AMOLED 恢复失败只能执行唯一一次重试")

    refresh_model = normalize_c(c_function_body(ui_source, "refresh_runtime_model"))
    for token in (
        "model.selftest_running||model.retry_running",
        "resume_display_with_single_retry()",
        "elseif(!selftest_active)",
    ):
        if token not in refresh_model:
            fail(f"UI 自检常亮状态边沿语义缺失: {token}")
    if "co5300_bsp_set_display(false)" in refresh_model:
        fail("UI 自检终态不得立即熄屏")


def validate_repository_hygiene() -> None:
    pyc_files = sorted(
        path.relative_to(ROOT)
        for path in ROOT.rglob("*.pyc")
        if ".git" not in path.parts
        and "build" not in path.parts
        and "managed_components" not in path.parts
    )
    if pyc_files:
        fail("仓库含 Python 字节码产物: " + ", ".join(map(str, pyc_files)))

    tracked = subprocess.run(
        ["git", "ls-files", "-co", "--exclude-standard"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.splitlines()
    for relative in tracked:
        path = ROOT / relative
        if not path.is_file():
            continue
        if path.suffix.lower() not in TEXT_SUFFIXES and path.name != "CMakeLists.txt":
            continue
        data = path.read_bytes()
        if data.startswith(b"\xef\xbb\xbf"):
            fail(f"文本文件包含 UTF-8 BOM: {relative}")
        try:
            data.decode("utf-8")
        except UnicodeDecodeError as error:
            fail(f"文本文件不是有效 UTF-8: {relative}: {error}")

    diff_check = subprocess.run(
        ["git", "diff", "--check", "HEAD", "--"],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if diff_check.returncode != 0:
        first_error = diff_check.stdout.splitlines()[0] if diff_check.stdout else "未知错误"
        fail(f"git diff --check 未通过: {first_error}")


def balanced_source(path: Path) -> None:
    text = read(path)
    stripped = re.sub(r'"(?:\\.|[^"\\])*"', '""', text)
    stripped = re.sub(r"//[^\n]*|/\*.*?\*/", "", stripped, flags=re.S)
    for opening, closing in (("{", "}"), ("(", ")"), ("[", "]")):
        depth = 0
        for character in stripped:
            if character == opening:
                depth += 1
            elif character == closing:
                depth -= 1
            if depth < 0:
                fail(f"{path.relative_to(ROOT)} 的 {opening}{closing} 提前闭合")
        if depth != 0:
            fail(f"{path.relative_to(ROOT)} 的 {opening}{closing} 不平衡: {depth}")


def validate_static_function_closure(path: Path) -> None:
    source = strip_c_comments(read(path))
    pattern = re.compile(
        r"^\s*static\s+[A-Za-z_][A-Za-z0-9_\s*]*?\s+"
        r"([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*([;{])",
        flags=re.M | re.S,
    )
    declarations: set[str] = set()
    definitions: set[str] = set()
    for name, terminator in pattern.findall(source):
        (declarations if terminator == ";" else definitions).add(name)
    missing_definitions = sorted(declarations - definitions)
    missing_declarations = sorted(definitions - declarations)
    if missing_definitions:
        fail(
            f"{path.relative_to(ROOT)} 静态函数只声明未实现: "
            + ", ".join(missing_definitions)
        )
    if missing_declarations:
        fail(
            f"{path.relative_to(ROOT)} 静态函数未在顶部声明: "
            + ", ".join(missing_declarations)
        )


def validate_public_api_closure(header: Path, source: Path, prefix: str) -> None:
    declaration_pattern = re.compile(
        rf"\b({re.escape(prefix)}[A-Za-z0-9_]*)\s*\([^;{{}}]*\)\s*;",
        flags=re.S,
    )
    definition_pattern = re.compile(
        rf"\b({re.escape(prefix)}[A-Za-z0-9_]*)\s*\([^;{{}}]*\)\s*\{{",
        flags=re.S,
    )
    declarations = set(declaration_pattern.findall(strip_c_comments(read(header))))
    definitions = set(definition_pattern.findall(strip_c_comments(read(source))))
    missing = sorted(declarations - definitions)
    if missing:
        fail(
            f"{header.relative_to(ROOT)} 公开 API 未在 {source.relative_to(ROOT)} 实现: "
            + ", ".join(missing)
        )


def require_contract_tokens(path: Path, tokens: dict[str, str]) -> None:
    source = strip_c_comments(read(path))
    for token, purpose in tokens.items():
        if token not in source:
            fail(f"{path.relative_to(ROOT)} 缺少{purpose}: {token}")


def authored_object_property(tree: object, object_name: str, property_name: str) -> str:
    matches: list[str] = []

    def visit(value: object) -> None:
        if isinstance(value, dict):
            properties = value.get("properties")
            if isinstance(properties, list):
                name = next(
                    (
                        item.get("strval")
                        for item in properties
                        if isinstance(item, dict)
                        and item.get("strtype") == "OBJECT/Name"
                    ),
                    None,
                )
                if name == object_name:
                    matches.extend(
                        str(item.get("strval"))
                        for item in properties
                        if isinstance(item, dict)
                        and item.get("strtype") == property_name
                    )
            for child in value.values():
                visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)

    visit(tree)
    if len(matches) != 1:
        fail(f"{object_name} 的 {property_name} 数量为 {len(matches)}，预期 1")
    return matches[0]


def validate_ui_feedback_contract() -> None:
    project = json.loads(read(ROOT / "lvgl-design/squareline_studio/watch-lvgl.spj"))
    component = json.loads(
        read(ROOT / "lvgl-design/squareline_studio/components/cmp_setting_toggle.ecomp")
    )
    authored_expectations = (
        (project, "ui_page_settings_root", "OBJECT/Clickable", "False"),
        (project, "ui_page_settings_root", "OBJECT/Click_focusable", "False"),
        (project, "ui_page_settings_root", "OBJECT/Gesture_bubble", "True"),
        (project, "ui_settings_settings_list_viewport", "OBJECT/Scrollable", "True"),
        (project, "ui_settings_settings_list_viewport", "OBJECT/Scroll_one", "False"),
        (project, "ui_settings_settings_list_page_1", "OBJECT/Snappable", "False"),
        (project, "ui_settings_settings_list_page_2", "OBJECT/Snappable", "False"),
        (project, "ui_settings_gesture_zone_settings_return", "OBJECT/Clickable", "True"),
        (project, "ui_settings_gesture_zone_settings_return", "OBJECT/Click_focusable", "True"),
        (project, "ui_settings_gesture_zone_settings_return", "OBJECT/Gesture_bubble", "False"),
        (project, "ui_settings_row_haptics_instance", "OBJECT/Gesture_bubble", "True"),
        (project, "ui_settings_row_click_audio_instance", "OBJECT/Gesture_bubble", "True"),
        (project, "ui_settings_row_raise_wake_instance", "OBJECT/Gesture_bubble", "True"),
        (project, "ui_settings_row_device_info", "OBJECT/Gesture_bubble", "True"),
        (project, "ui_settings_row_maintenance", "OBJECT/Gesture_bubble", "True"),
        (component, "cmp_setting_toggle", "OBJECT/Gesture_bubble", "True"),
    )
    for tree, object_name, property_name, expected in authored_expectations:
        actual = authored_object_property(tree, object_name, property_name)
        if actual != expected:
            fail(f"{object_name} 的 {property_name}={actual}，预期 {expected}")

    require_contract_tokens(
        ROOT / "lvgl-design/squareline_studio/tools/generate_squareline_project.py",
        {
            "root_clickable = False": "页面根不抢占设置列表手势的生成规则",
            '"ui_settings_settings_list_viewport"': "设置列表纵向滚动生成规则",
            '"OBJECT/Scroll_one": "False"': "设置列表禁止整页滚动生成规则",
            'h = 608': "设置七栏连续内容高度生成规则",
            'h = 264': "设置后三栏容器高度生成规则",
            '"ui_gesture_zone_settings_return"': "设置页底部独立返回手势区生成规则",
            '"cmp_setting_toggle"': "设置开关手势冒泡生成规则",
            '"ui_settings_row_raise_wake_instance"': "抬腕亮屏行手势冒泡生成规则",
            '"ui_settings_row_device_info"': "设备信息行手势冒泡生成规则",
            '"ui_settings_row_maintenance"': "维护行手势冒泡生成规则",
        },
    )

    generated_settings = normalize_c(
        read(GENERATED / "screens/ui_ui_scr_settings.c")
    )
    generated_toggle = normalize_c(
        read(GENERATED / "components/ui_comp_cmp_setting_toggle.c")
    )
    required_clear_flags = (
        (generated_settings, "ui_ui_page_settings_root", "LV_OBJ_FLAG_CLICKABLE"),
        (generated_settings, "ui_ui_page_settings_root", "LV_OBJ_FLAG_CLICK_FOCUSABLE"),
        (generated_settings, "ui_ui_settings_settings_list_viewport", "LV_OBJ_FLAG_GESTURE_BUBBLE"),
    )
    for source, symbol, required_flag in required_clear_flags:
        match = re.search(rf"lv_obj_clear_flag\({symbol},([^;]+)\);", source)
        if match is None or required_flag not in match.group(1):
            fail(f"{symbol} 未清除 {required_flag}，设置页滚动/返回手势所有权不闭合")

    for source, symbol, forbidden_flag in (
        (generated_settings, "ui_ui_settings_settings_list_viewport", "LV_OBJ_FLAG_SCROLLABLE"),
        (generated_settings, "ui_ui_settings_gesture_zone_settings_return", "LV_OBJ_FLAG_CLICKABLE"),
        (generated_settings, "ui_ui_settings_row_device_info", "LV_OBJ_FLAG_GESTURE_BUBBLE"),
        (generated_settings, "ui_ui_settings_row_maintenance", "LV_OBJ_FLAG_GESTURE_BUBBLE"),
        (generated_toggle, "cui_cmp_setting_toggle", "LV_OBJ_FLAG_GESTURE_BUBBLE"),
    ):
        match = re.search(rf"lv_obj_clear_flag\({symbol},([^;]+)\);", source)
        if match is None or forbidden_flag in match.group(1):
            fail(f"{symbol} 仍清除 {forbidden_flag}，设置页手势链未闭合")

    runtime_events = read(BINDINGS / "ui_runtime_events.c")
    require_contract_tokens(
        BINDINGS / "ui_runtime_events.c",
        {
            "UI_RUNTIME_INTENT_SOURCE_BUTTON": "按钮 intent 来源",
        },
    )
    require_contract_tokens(
        BINDINGS / "ui_runtime_binding.c",
        {
            "LV_OBJ_FLAG_SCROLL_CHAIN_HOR": "主壳全部触点横向滚动链",
            "LV_OBJ_FLAG_SCROLL_CHAIN_VER": "设置控件纵向滚动链",
            "LV_OBJ_FLAG_EVENT_BUBBLE": "释放事件向交互根冒泡",
            "UI_MAIN_SETTINGS_START_MAX_Y": "常驻页顶部按下起点门禁",
            "UI_SETTINGS_RETURN_START_MIN_Y": "设置页底部按下起点门禁",
            "UI_EDGE_SWIPE_COMMIT_PX": "边缘手势累计位移门禁",
            "UI_SETTINGS_LIST_CONTENT_HEIGHT": "设置七栏连续内容高度",
            "UI_SETTINGS_LIST_MAX_SCROLL_Y": "设置列表连续最大滚动量",
            "ui_touch_session_claim_navigation": "单触摸导航唯一提交",
            "UI_RUNTIME_INTENT_SOURCE_NON_BUTTON": "非按钮导航 intent 来源",
        },
    )
    runtime_binding = strip_c_comments(
        read(BINDINGS / "ui_runtime_binding.c")
    )
    render_code = normalize_c(
        c_function_body(runtime_binding, "ui_runtime_binding_render")
    )
    projection_position = render_code.find("apply_model_projection(model)")
    settings_normalization = (
        "if(resolved_screen==UI_RUNTIME_SCREEN_SETTINGS)"
        "{if(static_patch_changed){normalize_settings_interaction();}"
    )
    normalization_position = render_code.find(
        settings_normalization,
        projection_position + 1,
    )
    if projection_position < 0 or normalization_position < 0:
        fail("设置页交互规范化必须在静态 patch 变化后执行")

    settings_interaction = normalize_c(
        c_function_body(runtime_binding, "normalize_settings_interaction")
    )
    for token in (
        "add_descendant_flags("
        "ui_ui_settings_settings_list_viewport,"
        "LV_OBJ_FLAG_SCROLL_CHAIN_VER)",
        "lv_obj_clear_flag("
        "ui_ui_settings_settings_list_viewport,"
        "LV_OBJ_FLAG_SCROLL_CHAIN|"
        "LV_OBJ_FLAG_SCROLL_ONE|"
        "LV_OBJ_FLAG_SCROLL_ELASTIC)",
        "set_object_geometry("
        "ui_ui_settings_settings_list_content,"
        "0,0,378,UI_SETTINGS_LIST_CONTENT_HEIGHT)",
        "set_object_geometry("
        "ui_ui_settings_settings_list_page_2,"
        "0,UI_SETTINGS_LIST_TRAILING_GROUP_Y,378,"
        "UI_SETTINGS_LIST_TRAILING_GROUP_HEIGHT)",
        "set_object_geometry("
        "ui_ui_settings_gesture_zone_settings_return,"
        "UI_SETTINGS_RETURN_ZONE_X,"
        "UI_SETTINGS_RETURN_ZONE_Y,"
        "UI_SETTINGS_RETURN_ZONE_WIDTH,"
        "UI_SETTINGS_RETURN_ZONE_HEIGHT)",
    ):
        if token not in settings_interaction:
            fail(f"设置页运行时交互规范化缺少合同: {token}")

    settings_snapshot_warm = normalize_c(
        c_function_body(
            runtime_binding,
            "ui_runtime_binding_warm_settings_snapshot",
        )
    )
    snapshot_normalization_position = settings_snapshot_warm.find(
        "normalize_settings_interaction();"
    )
    snapshot_capture_position = settings_snapshot_warm.find(
        "lv_snapshot_take_to_buf("
    )
    if (
        snapshot_normalization_position < 0
        or snapshot_capture_position < 0
        or snapshot_normalization_position > snapshot_capture_position
    ):
        fail("设置内容快照重建前必须恢复 378×608 运行时几何")
    for token in (
        "s_settings_snapshot_warm_fault_latched||",
        "lv_obj_update_layout("
        "ui_ui_settings_settings_list_content)",
        "lv_snapshot_buf_size_needed("
        "snapshot_object,LV_IMG_CF_TRUE_COLOR)",
        "buffer_bytes<required_bytes",
        "s_settings_snapshot_warm_fault_latched=true",
    ):
        if token not in settings_snapshot_warm:
            fail(f"设置内容快照尺寸与故障熔断缺少合同: {token}")

    settings_snapshot_ready = normalize_c(
        c_function_body(runtime_binding, "settings_snapshot_cache_ready")
    )
    if "!s_settings_snapshot_warm_fault_latched" not in settings_snapshot_ready:
        fail("设置快照故障熔断后不得继续使用旧前台快照")

    shell_snapshot_toggle = normalize_c(
        c_function_body(runtime_binding, "set_shell_snapshot_active")
    )
    for token in (
        "lv_obj_add_flag(roots[slot],LV_OBJ_FLAG_HIDDEN)",
        "lv_obj_clear_flag("
        "s_shell_snapshot_images[slot],LV_OBJ_FLAG_HIDDEN)",
        "s_shell_snapshot_active=active",
        "request_full_redraw()",
    ):
        if token not in shell_snapshot_toggle:
            fail(f"常驻横滑快照直传切换缺少合同: {token}")
    clear_dirty = shell_snapshot_toggle.find("_lv_inv_area(display,NULL)")
    disable_invalidation = shell_snapshot_toggle.find(
        "lv_disp_enable_invalidation(display,false)"
    )
    if (
        clear_dirty < 0
        or disable_invalidation < 0
        or clear_dirty > disable_invalidation
    ):
        fail("常驻横滑起滑必须先清空旧脏区再暂停 LVGL invalidation")
    if "lv_disp_enable_invalidation(display,true)" not in shell_snapshot_toggle:
        fail("常驻横滑结束或异常必须恢复 LVGL invalidation")

    shell_fast_frame = normalize_c(
        c_function_body(
            runtime_binding,
            "ui_runtime_binding_shell_fast_frame",
        )
    )
    for token in (
        "s_shell_motion_active",
        "s_shell_snapshot_active",
        "shell_snapshot_cache_ready()",
        "shell_snapshot_buffer_for_slot(s_shell_slot)",
        "lv_obj_get_scroll_x(ui_ui_main_shell_pager)",
        ".current_pixels=",
        ".left_pixels=",
        ".right_pixels=",
        ".offset_px=",
    ):
        if token not in shell_fast_frame:
            fail(f"常驻横滑只读快帧描述缺少合同: {token}")

    settings_scroll = normalize_c(
        c_function_body(runtime_binding, "settings_scroll_event_cb")
    )
    if re.search(r"\bs_settings_list_page\b", runtime_binding):
        fail("设置列表不得保留二选一页码状态")
    if "lv_obj_scroll_to_y" in settings_scroll or "LV_ANIM_ON" in settings_scroll:
        fail("设置滚动结束不得吸附或动画回写到固定页")

    main_gesture = normalize_c(
        c_function_body(runtime_events, "ui_evt_main_shell_gesture")
    )
    settings_gesture = normalize_c(
        c_function_body(runtime_events, "ui_evt_settings_gesture")
    )
    if "lv_indev_get_gesture_dir" in main_gesture or any(
        token in main_gesture
        for token in ("UI_INTENT_SHELL_NEXT", "UI_INTENT_SHELL_PREVIOUS")
    ):
        fail("常驻页生成 gesture 回调不得重复接管原生 pager 横滑")
    if "lv_indev_get_gesture_dir" in settings_gesture:
        fail("设置页生成 gesture 回调不得继续使用识别瞬间坐标")

    manifest = json.loads(read(MANIFEST))
    route_events = {
        route.get("route_id"): route.get("event")
        for route in manifest.get("route_registry", {}).get("concrete_routes", [])
    }
    expected_route_events = {
        "route_settings_open": "swipe_down",
        "route_settings_back": "swipe_up/back",
    }
    for route_id, expected_event in expected_route_events.items():
        if route_events.get(route_id) != expected_event:
            fail(
                f"{route_id} 事件={route_events.get(route_id)}，预期 {expected_event}"
            )
    require_contract_tokens(
        ROOT / "components/services/ui_service/ui_service.c",
        {
            "UI_TOUCH_READ_I2C_TIMEOUT_MS": "触摸读点短 I2C 等待",
            "UI_EVENT_SNAPSHOT_TIMEOUT_MS": "用户事件状态快照有界等待",
            "UI_TOUCH_SAMPLE_UNKNOWN": "触摸读取空洞不得伪造释放",
            "ui_touch_session_feed": "连续触摸输入适配",
            "UI_TOUCH_GESTURE_LIMIT_PX": "触摸手势最小识别行程",
            "s_input_driver.gesture_limit = UI_TOUCH_GESTURE_LIMIT_PX":
                "LVGL 输入手势阈值配置",
            "UI_RUNTIME_INTENT_REJECTED": "拒绝意图反馈门禁",
            "audio_service_request_ui_feedback": "统一 UI 反馈提交",
            "request_ui_feedback(s_haptics_enabled": "开关最终状态反馈",
        },
    )
    ui_service = read(ROOT / "components/services/ui_service/ui_service.c")
    if "model.click_audio_enabled || model.selftest_running" in ui_service:
        fail("ui_service 仍在自检期间绕过点击音关闭设置")

    require_contract_tokens(
        ROOT / "components/services/audio_service/audio_service.c",
        {
            "s_ui_feedback_queue": "独立 UI 反馈队列",
            "audio_service_resolve_ui_feedback": "音振四组合纯策略",
            "atomic_load(&s_ui_haptics_enabled)": "最新触觉策略门禁",
            "atomic_load(&s_ui_click_audio_enabled)": "最新点击音策略门禁",
            "[AUDIO_RESOURCE_UI_CLICK] = "
            "\"/spiffs/audio/ui_click_16k_mono.wav\"":
                "SPIFFS 点击音固定资源映射",
            "play_resource(AUDIO_RESOURCE_UI_CLICK":
                "点击音复用固定 WAV 播放链路",
            "vTaskDelay(pdMS_TO_TICKS(NS4150_BSP_STARTUP_SETTLE_MS))":
                "覆盖 NS4150B 启动时间的稳定等待",
            "take_output_end_request": "自检取消与 UI STOP 分流",
            "s_last_power_level": "最近有效电量事实缓存",
            "AUDIO_OUTPUT_DRAIN_SAMPLE_COUNT": "音频 DMA 零尾",
            "vibration_bsp_lease_acquire(0)": "UI 触觉非阻塞租约",
            "xQueuePeek(s_audio_queue": "不吞掉资源播放的停止检测",
        },
    )
    audio_service = read(
        ROOT / "components/services/audio_service/audio_service.c"
    )
    normalized_audio_service = normalize_c(strip_c_comments(audio_service))
    ns4150_header = read(
        ROOT / "components/BSP/NS4150/ns4150_bsp.h"
    )
    ns4150_settle_match = re.search(
        r"#define\s+NS4150_BSP_STARTUP_SETTLE_MS\s+(\d+)U",
        ns4150_header,
    )
    if (
        ns4150_settle_match is None
        or int(ns4150_settle_match.group(1)) < 120
    ):
        fail("NS4150B 启动等待不得低于数据手册典型值 120 ms")
    play_resource_start = normalized_audio_service.rfind(
        "staticaudio_service_result_tplay_resource("
    )
    play_resource_body = normalized_audio_service[play_resource_start:]
    ordered_audio_tokens = (
        "ns4150_bsp_enable()",
        "vTaskDelay(pdMS_TO_TICKS(NS4150_BSP_STARTUP_SETTLE_MS))",
        "es8311_bsp_write(",
        "AUDIO_OUTPUT_DRAIN_SAMPLE_COUNT",
        "returnfinish_playback(",
    )
    previous_position = -1
    for token in ordered_audio_tokens:
        position = play_resource_body.find(token, previous_position + 1)
        if position < 0:
            fail(f"固定音频播放链路缺少有序步骤: {token}")
        previous_position = position
    for token, purpose in {
        "play_resource(AUDIO_RESOURCE_UI_CLICK,shutdown_requested,NULL,0U,NULL)":
            "UI 点击只响应 STOP 的共享播放调用",
        "playback_cancelled!=NULL?take_playback_end_request("
        "shutdown_requested,playback_cancelled):"
        "take_stop_request(shutdown_requested)":
            "自检取消与 UI STOP 的互斥消费语义",
    }.items():
        if token not in normalized_audio_service:
            fail(f"audio_service 缺少{purpose}: {token}")
    for removed_click_generator in (
        "AUDIO_UI_CLICK_SAMPLE_COUNT",
        "AUDIO_UI_CLICK_HALF_PERIOD_SAMPLES",
        "ui_click_sample(",
    ):
        if removed_click_generator in audio_service:
            fail(f"audio_service 仍保留运行时点击方波: {removed_click_generator}")
    require_contract_tokens(
        ROOT / "components/BSP/ES8311/es8311_bsp.c",
        {
            "channel_config.dma_desc_num = AUDIO_BSP_DMA_DESC_NUM": "I2S DMA 描述符数量",
            "channel_config.auto_clear_after_cb = true": "I2S TX 自动清零",
        },
    )
    require_contract_tokens(
        ROOT / "components/BSP/VIBRATION/vibration_bsp.c",
        {"xSemaphoreCreateMutexStatic": "振动硬件静态互斥租约"},
    )
    require_contract_tokens(
        ROOT / "components/services/selftest_service/selftest_service.c",
        {
            "vibration_bsp_lease_acquire": "自检振动租约获取",
            "release_vibration_lease": "自检振动租约释放",
        },
    )


def main() -> None:
    manifest = json.loads(read(MANIFEST))
    counts = manifest["counts"]
    expected = {
        "canonical_states": 23,
        "full_page_variants": 7,
        "screens": 9,
        "page_state_root_objects": 13,
        "squareline_components": 4,
        "code_only_states": 1,
    }
    for key, value in expected.items():
        if counts.get(key) != value:
            fail(f"manifest {key}={counts.get(key)}，预期 {value}")

    validate_firmware_version_contract()
    validate_firmware_profile_contract()
    validate_generated_display_geometry(manifest)
    validate_projection_semantics(manifest)

    filelist = [line for line in read(GENERATED / "filelist.txt").splitlines() if line]
    actual = sorted(str(path.relative_to(GENERATED)) for path in GENERATED.rglob("*.c"))
    if sorted(filelist) != actual:
        fail("generated/filelist.txt 与实际 SquareLine C 源文件不一致")
    if len(list((GENERATED / "screens").glob("*.c"))) != 9:
        fail("SquareLine Screen 源文件不是 9 个")
    if len(list((GENERATED / "fonts").glob("*.c"))) != 76:
        fail("SquareLine 字体源文件不是 76 个")
    project_info = json.loads(read(GENERATED / "project.info"))
    if project_info.get("editor_version") != "1.6.1" or not project_info.get(
        "export_datetime"
    ):
        fail("generated/project.info 缺少 SquareLine 1.6.1 正规导出证据")
    if "LVGL version: 8.3.11" not in read(GENERATED / "ui.h"):
        fail("SquareLine 导出头未锁定 LVGL 8.3.11")

    generated_text = "\n".join(read(path) for path in GENERATED.rglob("*.[ch]"))
    for forbidden in ("watch_state_snapshot", "maintenance_service_", "ble_service_", "CHANGE SCREEN"):
        if forbidden in generated_text:
            fail(f"generated 层出现禁止依赖或动作: {forbidden}")

    event_header = read(GENERATED / "ui_events.h")
    event_stub = read(GENERATED / "ui_events.c")
    event_binding = read(BINDINGS / "ui_runtime_events.c")
    declaration_pattern = re.compile(r"void\s+(ui_evt_[A-Za-z0-9_]+)\s*\(")
    declarations = set(declaration_pattern.findall(event_header))
    generated_definitions = set(declaration_pattern.findall(event_stub))
    binding_definitions = set(declaration_pattern.findall(event_binding))
    if len(declarations) != 65:
        fail(f"SquareLine 事件声明数量为 {len(declarations)}，预期 65")
    if declarations != generated_definitions or declarations != binding_definitions:
        fail("SquareLine 事件声明、空桩与 typed intent 实现不闭合")
    if "Your code here" not in event_stub or "ui_runtime_binding" in event_stub:
        fail("SquareLine ui_events.c 不再是标准空回调桩")

    runtime_source = read(BINDINGS / "ui_runtime_binding.c")
    validate_resolver_evidence(manifest, runtime_source)
    validate_route_semantics(runtime_source)
    validate_ui_service_semantics(
        read(ROOT / "components/services/ui_service/ui_service.c")
    )
    validate_ble_control_priority(
        read(ROOT / "components/services/ble_service/ble_service.c")
    )
    validate_selftest_runtime_edges()
    validate_internal_selftest_privacy(runtime_source)
    validate_all_page_frame_rate_contract()
    validate_ui_feedback_contract()
    validate_font_coverage()

    symbols = generated_object_symbols()
    referenced = set(re.findall(r"\b(ui_ui_[A-Za-z0-9_]+)\b", runtime_source + event_binding))
    missing = sorted(
        name
        for name in referenced - symbols
        if not re.search(
            r"(?:_screen_(?:init|destroy)|_modal_(?:create|destroy))$",
            name,
        )
    )
    if missing:
        fail("binding 引用了未导出的 SquareLine 对象: " + ", ".join(missing))

    cmake = read(ROOT / "components/ui/CMakeLists.txt")
    for required in (
        "generated/filelist.txt",
        'list(REMOVE_ITEM SQUARELINE_GENERATED_SRCS "ui_events.c")',
        '"bindings/ui_runtime_events.c"',
        '"bindings/ui_runtime_binding.c"',
        '"bindings/ui_manifest_projection.c"',
    ):
        if required not in cmake:
            fail(f"UI CMake 缺少登记: {required}")

    navigation = read(ROOT / "components/ui/ui_navigation.h") + read(
        ROOT / "components/ui/ui_navigation.c"
    )
    if "UI_PAGE_HOME_UNAVAILABLE" in navigation or "UI_PAGE_HOME_DISCONNECTED" not in navigation:
        fail("01-03 页面枚举未统一为 UI_PAGE_HOME_DISCONNECTED")

    for path in (
        BINDINGS / "ui_runtime_binding.c",
        BINDINGS / "ui_runtime_events.c",
        BINDINGS / "ui_manifest_projection.c",
        ROOT / "components/ui/ui_selftest_summary.c",
        ROOT / "components/platform/config_service/config_service.c",
        ROOT / "components/services/audio_service/audio_service.c",
        ROOT / "components/services/ble_service/ble_service.c",
        ROOT / "components/services/legbot_services.c",
        ROOT / "components/services/selftest_service/selftest_service.c",
        ROOT / "components/services/ui_service/ui_service.c",
    ):
        balanced_source(path)
        validate_static_function_closure(path)

    validate_public_api_closure(
        BINDINGS / "ui_runtime_binding.h",
        BINDINGS / "ui_runtime_binding.c",
        "ui_runtime_binding_",
    )
    validate_public_api_closure(
        BINDINGS / "ui_manifest_projection.h",
        BINDINGS / "ui_manifest_projection.c",
        "ui_manifest_projection_",
    )
    validate_public_api_closure(
        ROOT / "components/ui/ui_navigation.h",
        ROOT / "components/ui/ui_navigation.c",
        "ui_navigation_",
    )
    validate_public_api_closure(
        ROOT / "components/ui/ui_selftest_summary.h",
        ROOT / "components/ui/ui_selftest_summary.c",
        "ui_selftest_",
    )
    validate_public_api_closure(
        ROOT / "components/services/ui_service/ui_service.h",
        ROOT / "components/services/ui_service/ui_service.c",
        "ui_service_",
    )
    validate_public_api_closure(
        ROOT / "components/platform/config_service/config_service.h",
        ROOT / "components/platform/config_service/config_service.c",
        "config_service_",
    )
    validate_public_api_closure(
        ROOT / "components/services/audio_service/audio_service.h",
        ROOT / "components/services/audio_service/audio_service.c",
        "audio_service_",
    )
    validate_public_api_closure(
        ROOT / "components/BSP/VIBRATION/vibration_bsp.h",
        ROOT / "components/BSP/VIBRATION/vibration_bsp.c",
        "vibration_bsp_",
    )
    validate_public_api_closure(
        ROOT / "components/services/ble_service/ble_service.h",
        ROOT / "components/services/ble_service/ble_service.c",
        "ble_service_",
    )
    validate_public_api_closure(
        ROOT / "components/services/selftest_service/selftest_service.h",
        ROOT / "components/services/selftest_service/selftest_service.c",
        "selftest_service_",
    )
    validate_public_api_closure(
        GENERATED / "screens/ui_ui_scr_maintenance.h",
        GENERATED / "screens/ui_ui_scr_maintenance.c",
        "ui_ui_clear_binding_modal_",
    )

    require_contract_tokens(
        BINDINGS / "ui_runtime_binding.c",
        {
            "s_payment_lock_active": "支付锁解除复位状态",
            "!model->payment_pending": "支付重试去重门禁",
            "s_last_ble_connected": "BLE 事实变化路由复位",
            "s_pending_retry_item": "单项重试路由上下文",
            "shell_scroll_end_event_cb": "主壳原生横滑槽位同步",
            "LV_EVENT_SCROLL_END": "主壳横滑结束事件",
            "LV_EVENT_SCROLL": "主壳横滑行程事件",
            "lv_obj_get_scroll_x": "主壳横滑目标槽位解析",
            "s_committed_shell_slot != s_shell_slot": "主壳仅在槽位变化时主动对齐",
            "UI_SHELL_SWIPE_TRIGGER_PX": "主壳小行程切页阈值",
            "s_shell_scroll_peak_delta": "主壳单次横滑峰值行程",
            "apply_shell_static_patches": "四个常驻槽位同帧状态预投影",
            "button_intent_blocked_by_gesture": "按钮手势误触门禁",
            "failed_category_at": "七类自检失败项浏览",
            "touch_target_event_cb": "TOUCH 目标交互",
            "touch_slider_event_cb": "TOUCH 滑动进度交互",
            "output_replay_event_cb": "AUDIO/VIBRATION 重放 typed intent",
            "ui_ui_clear_binding_modal_create(lv_layer_top())":
                "09-01 modal 按需创建",
            "ui_ui_clear_binding_modal_destroy()": "09-01 modal 退出回收",
            "ui_runtime_binding_stop_amoled_test()": "AMOLED 退出回收",
        },
    )
    require_contract_tokens(
        ROOT / "components/services/ui_service/ui_service.c",
        {
            "ble_service_request_reconnect_now(0)": "立即重连 owner 提交",
            "ble_service_request_rescan(0)": "重新扫描 owner 提交",
            "config_service_ui_preferences_read": "UI 偏好持久化读取",
            "config_service_ui_preferences_write": "UI 偏好持久化写入",
            "audio_service_request_ui_feedback": "UI 触觉与点击音 owner 提交",
            "UI_CONTROL_PROJECTION_LOCK_MS 5000U": "五秒单字段乐观锁",
            "maintenance_service_request_selftest_retry": "单项自检重试",
            "watch.exoskeleton_low_battery": "外骨骼低电迟滞事实消费",
            "WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY": "手环严重低电事实消费",
            "WATCH_POWER_LEVEL_LOW_BATTERY_WARN": "手环普通低电事实消费",
            "manual_countdown_refresh_due": "人工自检倒计时刷新",
            "esp_app_get_description()": "运行镜像版本描述符读取",
            "app_description->version[0] != '\\0'": "固件版本未知占位",
            "watch.watch_id[0] != '\\0'": "Watch ID 未知占位",
            "watch.bound_exoskeleton_mac[0] != '\\0'":
                "已绑定 MAC 未知占位",
        },
    )
    require_contract_tokens(
        ROOT / "components/services/selftest_service/selftest_service.c",
        {
            "SELFTEST_VIBRATION_TIMEOUT_MS 30000U":
                "振动自检三十秒超时合同",
            "{.item_id = SELFTEST_ITEM_VIBRATION, .name = \"vibration\", "
            ".timeout_ms = SELFTEST_VIBRATION_TIMEOUT_MS":
                "振动自检元数据超时绑定",
            "{SELFTEST_ITEM_VIBRATION, SELFTEST_VIBRATION_TIMEOUT_MS, "
            "handle_vibration}": "振动 handler 超时绑定",
        },
    )
    require_contract_tokens(
        ROOT / "components/services/legbot_services.c",
        {
            "static StackType_t "
            "s_selftest_task_stack[LEGBOT_SELFTEST_SERVICE_STACK_BYTES]":
                "自检任务固定栈",
            "static StaticTask_t s_selftest_task_buffer":
                "自检任务固定 TCB",
            "xTaskCreateStatic(service_stub_task":
                "自检按需启动不依赖内部动态堆",
        },
    )
    require_contract_tokens(
        ROOT / "components/BSP/CO5300/co5300_bsp.h",
        {
            "#define CO5300_BSP_QSPI_CLOCK_HZ (80 * 1000 * 1000)":
                "CO5300 常驻横滑 60 FPS 像素时钟",
        },
    )
    for removed_alert_counter in (
        "ui_alerts_readonly_txt_alert_count",
        "ui_alerts_readonly_txt_priority",
        "已显示 %u/%u",
    ):
        if removed_alert_counter in runtime_source:
            fail(f"04-02 仍在运行时显示提醒条数: {removed_alert_counter}")

    generated_main_shell = (
        ROOT / "components/ui/generated/screens/ui_ui_scr_main_shell.c"
    ).read_text(encoding="utf-8")
    if "ui_ui_alerts_readonly_txt_alert_title" not in generated_main_shell:
        fail("04-02 生成代码缺少设备提醒标题对象")
    for removed_alert_counter in (
        "ui_ui_alerts_readonly_txt_alert_count",
        "ui_ui_alerts_readonly_txt_priority",
        "3 / 5",
    ):
        if removed_alert_counter in generated_main_shell:
            fail(f"04-02 生成代码仍保留提醒条数对象: {removed_alert_counter}")

    validate_repository_hygiene()

    print(
        json.dumps(
            {
                "ok": True,
                "states": 30,
                "screens": 9,
                "components": 4,
                "fonts": 76,
                "events": 65,
                "resolvers": 37,
                "generated_sources": len(filelist),
            },
            ensure_ascii=False,
        )
    )


if __name__ == "__main__":
    try:
        main()
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        print(f"ERROR: {error}")
        sys.exit(1)
