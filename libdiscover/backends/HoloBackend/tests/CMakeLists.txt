set(EXTRA_LIBS
    libdiscover-backend-holo-logging-category
)
add_unit_test(holotest HoloTest.cpp atomupd1_adaptor.cpp mock_server.cpp )
set_tests_properties(holotest PROPERTIES TIMEOUT 600)

target_link_libraries(holotest
    Qt::Core
    Qt::Widgets
    Qt::DBus
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
)
