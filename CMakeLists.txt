cmake_minimum_required(VERSION 3.30)
project(NimbusWeather VERSION 1.0.3 LANGUAGES CXX)

include(GNUInstallDirs)

# A portable Windows Qt application must keep its executable, Qt DLLs,
# compiler runtime and qt.conf in the same directory. The default GNU layout
# would place the executable at the package root and runtime DLLs in bin/.
if(WIN32)
    set(CMAKE_INSTALL_BINDIR ".")
    set(CMAKE_INSTALL_LIBDIR ".")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找所需依赖：Widgets用于系统托盘，Network用于腾讯API请求
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick Network Widgets)

# LLM 功能开关：默认关闭，cmake -DWITH_LLM=ON 开启
option(WITH_LLM "Build with LLM-powered weather alerts" OFF)

# 预定义各个阶段的源文件变量
set(TOOL_SRCS
    src/util/Config.cpp
    src/util/TimeUtil.cpp
    src/util/WeatherCode.cpp
    src/util/ScreenHelper.cpp
)
set(DATA_SRCS
    src/data/WeatherCacheManager.cpp
)
add_compile_definitions(NIMBUS_WEATHER_VERSION="${PROJECT_VERSION}")

set(NET_SRCS
    src/network/HttpClient.cpp
    src/network/TencentApiClient.cpp
    src/network/GitHubReleaseClient.cpp
)
set(SERVICE_SRCS
    src/service/WeatherService.cpp
    src/service/LocationService.cpp
    src/service/AlertService.cpp
    src/service/NotificationManager.cpp
)
set(VM_SRCS
    src/viewmodel/WeatherViewModel.cpp
    src/viewmodel/SettingsViewModel.cpp
    src/viewmodel/TrayViewModel.cpp
)

if(WITH_LLM)
    add_compile_definitions(WITH_LLM)
    set(LLM_SRCS
        src/llm/LLMClient.cpp
        src/llm/LLMAlertGenerator.cpp
    )
endif()

set(APP_SRCS
    src/main.cpp
    ${TOOL_SRCS}
    ${DATA_SRCS}
    ${NET_SRCS}
    ${SERVICE_SRCS}
    ${VM_SRCS}
    ${LLM_SRCS}
    resources/app.rc
    resources/resources.qrc
)

# 使用 Qt 的可执行目标以完成 QML 导入扫描和部署元数据生成；WIN32
# 属性用于隐藏 Windows 默认的控制台黑框。
qt_add_executable(NimbusWeather WIN32 ${APP_SRCS})

# 注册 QML 模块
set(WEATHER_APP_QML_FILES
    qml/MainWindow.qml
    qml/components/Theme.qml
    qml/components/TimePickerDialog.qml
    qml/components/Toolbar.qml
    qml/components/WeatherCard.qml
    qml/components/HourlyCard.qml
    qml/components/NavigationButton.qml
    qml/components/CitySelector.qml
    qml/components/TimeComboBox.qml
    qml/components/APISettingsPane.qml
    qml/views/TodayView.qml
    qml/views/PastView.qml
    qml/views/FutureView.qml
    qml/views/SettingsView.qml
)

if(WITH_LLM)
    list(APPEND WEATHER_APP_QML_FILES qml/components/LLMSettingsPane.qml)
endif()

qt_add_qml_module(NimbusWeather
    URI "NimbusWeather"
    VERSION 1.0
    QML_FILES ${WEATHER_APP_QML_FILES}
)

# 链接核心库
target_link_libraries(NimbusWeather PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::Widgets
)

# Windows: link Shell32 for SHAppBarMessage (DPAPI loaded dynamically at runtime)
if(WIN32)
    target_link_libraries(NimbusWeather PRIVATE Shell32)
endif()

# Keep the application bound to the same MinGW runtime that compiled it.
# App-local DLLs take precedence over older MinGW versions that may appear
# earlier in PATH (for example, a Qt 5 development environment).
if(WIN32 AND MINGW)
    get_filename_component(NIMBUS_WEATHER_MINGW_BIN_DIR
                           "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(runtime_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        if(EXISTS "${NIMBUS_WEATHER_MINGW_BIN_DIR}/${runtime_dll}")
            add_custom_command(TARGET NimbusWeather POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${NIMBUS_WEATHER_MINGW_BIN_DIR}/${runtime_dll}"
                    $<TARGET_FILE_DIR:NimbusWeather>
            )
        endif()
    endforeach()
endif()

# Build a self-contained install tree from the same target that is used during
# development. Qt's deployment API resolves the QML imports, Qt plugins and the
# compiler runtime from the selected Qt toolchain, so packaging cannot
# accidentally pick similarly named DLLs from PATH.
if(WIN32)
    set(NIMBUS_WEATHER_RUNTIME_DESTINATION ".")
else()
    set(NIMBUS_WEATHER_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

install(TARGETS NimbusWeather
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${NIMBUS_WEATHER_RUNTIME_DESTINATION}"
)

qt_generate_deploy_qml_app_script(
    TARGET NimbusWeather
    OUTPUT_SCRIPT NIMBUS_WEATHER_DEPLOY_SCRIPT
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --no-system-d3d-compiler --no-opengl-sw
)
install(SCRIPT "${NIMBUS_WEATHER_DEPLOY_SCRIPT}")

# CPack produces the portable archive and the enterprise MSI from this single
# install manifest. AI and Standard remain separate upgrade families so an
# installed edition can be serviced without affecting the other one.
if(WITH_LLM)
    set(NIMBUS_WEATHER_EDITION "AI")
    set(NIMBUS_WEATHER_PACKAGE_NAME "Nimbus Weather AI")
    set(NIMBUS_WEATHER_UPGRADE_GUID "9404FEBE-278C-41A8-ACFE-6704207F31AF")
else()
    set(NIMBUS_WEATHER_EDITION "Standard")
    set(NIMBUS_WEATHER_PACKAGE_NAME "Nimbus Weather Standard")
    set(NIMBUS_WEATHER_UPGRADE_GUID "0CF5BEF0-3574-4A1B-8118-D31CF6CC4489")
endif()

configure_file("${CMAKE_SOURCE_DIR}/LICENSE"
               "${CMAKE_BINARY_DIR}/LICENSE.txt" COPYONLY)

set(CPACK_GENERATOR "ZIP;WIX")
set(CPACK_PACKAGE_NAME "${NIMBUS_WEATHER_PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "shimamuraDS")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "A lightweight Qt weather desktop client with system tray integration")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME
    "NimbusWeather-${PROJECT_VERSION}-win64-${NIMBUS_WEATHER_EDITION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Nimbus Weather ${NIMBUS_WEATHER_EDITION}")
set(CPACK_PACKAGE_EXECUTABLES "NimbusWeather" "Nimbus Weather")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")

set(CPACK_WIX_VERSION 4)
set(CPACK_WIX_ARCHITECTURE x64)
set(CPACK_WIX_INSTALL_SCOPE perMachine)
set(CPACK_WIX_UPGRADE_GUID "${NIMBUS_WEATHER_UPGRADE_GUID}")
set(CPACK_WIX_UI_REF "WixUI_InstallDir")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "Nimbus Weather")
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/resources/NimbusWeather.ico")
set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT
    "https://github.com/shimamuraDS/nimbus-weather-desktop")
set(CPACK_WIX_PROPERTY_ARPCONTACT "shimamuraDS")

set(NIMBUS_WIX_ROOT "" CACHE PATH "Pinned WiX 4 tool directory used by CPack")
if(NIMBUS_WIX_ROOT)
    set(CPACK_WIX_ROOT "${NIMBUS_WIX_ROOT}")
endif()

include(CPack)

# 添加测试子目录
enable_testing()
add_subdirectory(tests)
