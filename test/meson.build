gtest = dependency(
    'gtest',
    main: true,
    disabler: true,
    required: get_option('tests'),
)
gmock = dependency('gmock', disabler: true, required: get_option('tests'))

tests = [
    'ipmi_close_unittest',
    'ipmi_commit_unittest',
    'ipmi_delete_unittest',
    'ipmi_enumerate_unittest',
    'ipmi_getcount_unittest',
    'ipmi_open_unittest',
    'ipmi_read_unittest',
    'ipmi_sessionstat_unittest',
    'ipmi_stat_unittest',
    'ipmi_unittest',
    'ipmi_validate_unittest',
    'ipmi_write_unittest',
    'ipmi_writemeta_unittest',
    'manager_close_unittest',
    'manager_commit_unittest',
    'manager_delete_unittest',
    'manager_expire_unittest',
    'manager_getsession_unittest',
    'manager_open_unittest',
    'manager_read_unittest',
    'manager_sessionstat_unittest',
    'manager_stat_unittest',
    'manager_unittest',
    'manager_write_unittest',
    'manager_writemeta_unittest',
    'process_unittest',
    'utils_unittest',
]

foreach t : tests
    test(
        t,
        executable(
            t.underscorify(),
            t + '.cpp',
            'helper.cpp',
            implicit_include_directories: false,
            dependencies: [blob_manager_dep, gtest, gmock],
        ),
    )
endforeach
