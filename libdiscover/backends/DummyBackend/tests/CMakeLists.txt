add_unit_test(dummytest
    DummyTest.cpp
)

add_unit_test(updatedummytest
    UpdateDummyTest.cpp
)
target_link_libraries(updatedummytest
    KF6::CoreAddons
)

add_test(NAME headless-updates
         COMMAND Plasma::Discover --backends dummy --headless-update)
