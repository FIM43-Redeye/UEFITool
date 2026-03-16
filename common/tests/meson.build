test_sources = files(
  'test_utility.cpp',
  'test_ffs.cpp',
  'test_types.cpp',
  'test_descriptor.cpp',
  'test_treemodel.cpp',
  'test_guiddatabase.cpp',
  'catch2/catch_amalgamated.cpp',
  # ffsops.cpp is not part of uefitoolcommon static library,
  # so we compile it directly into the test binary.
  '../ffsops.cpp',
)

uefitool_tests = executable('uefitool_tests',
  sources: test_sources,
  include_directories: include_directories('catch2', '..'),
  link_with: [uefitoolcommon, lzma, bstrlib, brotli],
  dependencies: [zlib],
  cpp_args: [
    '-DU_ENABLE_NVRAM_PARSING_SUPPORT',
    '-DU_ENABLE_ME_PARSING_SUPPORT',
    '-DU_ENABLE_FIT_PARSING_SUPPORT',
    '-DU_ENABLE_GUID_DATABASE_SUPPORT',
  ],
  override_options: ['cpp_std=c++23'],
)

test('uefitool_tests', uefitool_tests)
