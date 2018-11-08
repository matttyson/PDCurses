
if(NOT WIN32)
    message(FATAL_ERROR "This direct2d backend is windows only")
endif()

add_definitions(/D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions(/DPDCDEBUG)
endif()

add_library(pdcd2d_static STATIC
    ${CMAKE_CURRENT_LIST_DIR}/pdcclip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pdcdisp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pdcgetsc.c
    ${CMAKE_CURRENT_LIST_DIR}/pdckbd.c
    ${CMAKE_CURRENT_LIST_DIR}/pdcscrn.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pdcsetsc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pdcsetsc.c
    ${CMAKE_CURRENT_LIST_DIR}/pdcutil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ColorGlyphEffect.cpp
	${CMAKE_CURRENT_LIST_DIR}/ColorGlyphEffectShader.h
    ${CMAKE_CURRENT_LIST_DIR}/Guid.cpp
	${CMAKE_CURRENT_LIST_DIR}/pdc_d2d_util.c

    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/addch.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/addchstr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/addstr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/attr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/beep.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/bkgd.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/border.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/clear.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/color.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/debug.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/delch.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/deleteln.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/deprec.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/getch.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/getstr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/getyx.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/inch.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/inchstr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/initscr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/inopts.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/insch.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/insstr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/instr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/kernel.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/keyname.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/mouse.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/move.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/outopts.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/overlay.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/pad.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/panel.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/printw.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/refresh.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/scanw.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/scroll.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/scr_dump.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/slk.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/termattr.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/terminfo.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/touch.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/util.c
    ${CMAKE_CURRENT_LIST_DIR}/../pdcurses/window.c
)



include_directories(..)


# Build Demos

add_executable(firework
    ${CMAKE_CURRENT_LIST_DIR}/../demos/firework.cpp
)
set_target_properties(firework PROPERTIES COMPILE_FLAGS "/TP")
target_link_libraries(firework pdcd2d_static d2d1 windowscodecs dxguid D3D11)

#add_executable(ozdemo
#    ${CMAKE_CURRENT_LIST_DIR}/../demos/ozdemo.c
#)
#target_link_libraries(firework pdcd2d_static)

#add_executable(ptest
#    ${CMAKE_CURRENT_LIST_DIR}/../demos/ptest.c
#)
#target_link_libraries(ptest pdcd2d_static)

#add_executable(rain
#    ${CMAKE_CURRENT_LIST_DIR}/../demos/rain.c
#)
#target_link_libraries(rain pdcd2d_static)

#add_executable(testcurs
#    ${CMAKE_CURRENT_LIST_DIR}/../demos/testcurs.c
#)
#target_link_libraries(testcurs pdcd2d_static)

add_executable(tuidemo
    ${CMAKE_CURRENT_LIST_DIR}/../demos/tui.c
    ${CMAKE_CURRENT_LIST_DIR}/../demos/tuidemo.c
)
#set_target_properties(tuidemo PROPERTIES COMPILE_FLAGS "/TP")
target_link_libraries(tuidemo pdcd2d_static d2d1 windowscodecs dxguid D3D11)

#add_executable(worm
#    ${CMAKE_CURRENT_LIST_DIR}/../demos/worm.c
#)
#target_link_libraries(worm pdcd2d_static)

#add_executable(xmas
#    ${CMAKE_CURRENT_LIST_DIR}/../demos/xmas.c
#)
#target_link_libraries(xmas pdcd2d_static)
