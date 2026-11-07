# JIS layout shift module (in-repo)
#
# Keymap processing happens only on the central (right) half, so the
# behaviors are built there only.

if(CONFIG_ZMK_JIS_LAYOUT_SHIFT AND ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/jis_layout_state.c
    ${CMAKE_CURRENT_LIST_DIR}/src/behavior_jis_key_press.c
    ${CMAKE_CURRENT_LIST_DIR}/src/behavior_jis_layout_toggle.c
  )
endif()

# Alt+Tab ウィンドウ切替 behavior (ボールジェスチャー mg_switch 用)。
# keycode イベントを raise するため central (または非 split) 限定でビルド。
if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/behavior_alt_tab.c
  )
endif()
