cmake_minimum_required(VERSION 3.16)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Test)

enable_testing()

add_executable(tst_HourlyMerge tst_HourlyMerge.cpp
    ${CMAKE_SOURCE_DIR}/src/data/WeatherCacheManager.cpp
    ${CMAKE_SOURCE_DIR}/src/util/TimeUtil.cpp
)
target_link_libraries(tst_HourlyMerge PRIVATE Qt6::Core Qt6::Test)
target_include_directories(tst_HourlyMerge PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(tst_AlertCondition tst_AlertCondition.cpp
    ${CMAKE_SOURCE_DIR}/src/util/WeatherCode.cpp
    ${CMAKE_SOURCE_DIR}/src/util/TimeUtil.cpp
)
target_link_libraries(tst_AlertCondition PRIVATE Qt6::Core Qt6::Test)
target_include_directories(tst_AlertCondition PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(tst_HttpService tst_HttpService.cpp
    ${CMAKE_SOURCE_DIR}/src/network/HttpClient.cpp
    ${CMAKE_SOURCE_DIR}/src/network/TencentApiClient.cpp
    ${CMAKE_SOURCE_DIR}/src/util/Config.cpp
)
target_link_libraries(tst_HttpService PRIVATE Qt6::Core Qt6::Test Qt6::Network)
target_include_directories(tst_HttpService PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(tst_LLMClient tst_LLMClient.cpp
    ${CMAKE_SOURCE_DIR}/src/llm/LLMClient.cpp
)
target_link_libraries(tst_LLMClient PRIVATE Qt6::Core Qt6::Test Qt6::Network)
target_include_directories(tst_LLMClient PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME HourlyMerge COMMAND tst_HourlyMerge)
add_test(NAME AlertCondition COMMAND tst_AlertCondition)
add_test(NAME HttpService COMMAND tst_HttpService)
add_test(NAME LLMClient COMMAND tst_LLMClient)

# Windows does not search the compiler directory when CTest launches an
# executable. Keep every test self-contained so it cannot accidentally load a
# different MinGW runtime from the machine-wide PATH.
if(WIN32)
    function(deploy_test_runtime target)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Core>
                $<TARGET_FILE:Qt6::Network>
                $<TARGET_FILE:Qt6::Test>
                $<TARGET_FILE_DIR:${target}>
        )

        if(MINGW)
            get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
            foreach(runtime_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
                if(EXISTS "${MINGW_BIN_DIR}/${runtime_dll}")
                    add_custom_command(TARGET ${target} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${MINGW_BIN_DIR}/${runtime_dll}"
                            $<TARGET_FILE_DIR:${target}>
                    )
                endif()
            endforeach()
        endif()
    endfunction()

    deploy_test_runtime(tst_HourlyMerge)
    deploy_test_runtime(tst_AlertCondition)
    deploy_test_runtime(tst_HttpService)
    deploy_test_runtime(tst_LLMClient)
endif()
