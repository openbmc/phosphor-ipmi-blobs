project(
  'phosphor-ipmi-blobs',
  'cpp',
  version: '0.1',
  meson_version: '>=1.1.1',
  default_options: [
    'cpp_std=c++23',
    'warning_level=3',
    'werror=true',
  ])

ipmi_blob_dep = declare_dependency(
  include_directories: include_directories('.'))

install_headers(
  'blobs-ipmid/blobs.hpp',
  subdir: 'blobs-ipmid')

import('pkgconfig').generate(
  name: 'phosphor-ipmi-blobs',
  description: 'Phosphor Blob Transfer Interface',
  version: meson.project_version())

cpp = meson.get_compiler('cpp')

phosphor_logging_dep = dependency('phosphor-logging')
ipmid_dep = dependency('libipmid')
channellayer_dep = dependency('libchannellayer')

blob_manager_pre = declare_dependency(
  dependencies: [
    ipmi_blob_dep,
    dependency('ipmiblob'),
    ipmid_dep,
    phosphor_logging_dep,
  ])

blob_manager_lib = static_library(
  'blobmanager',
  'fs.cpp',
  'internal/sys.cpp',
  'ipmi.cpp',
  'manager.cpp',
  'process.cpp',
  'utils.cpp',
  implicit_include_directories: false,
  dependencies: blob_manager_pre)

blob_manager_dep = declare_dependency(
  link_with: blob_manager_lib,
  dependencies: blob_manager_pre)

conf_data = configuration_data()
conf_data.set_quoted(
  'BLOB_LIB_PATH',
  get_option('prefix') / get_option('libdir') / 'blob-ipmid')
conf_h = configure_file(
  output: 'config.h',
  configuration: conf_data)

shared_module(
  'blobmanager',
  conf_h,
  'main.cpp',
  implicit_include_directories: false,
  dependencies: [
    blob_manager_dep,
    ipmid_dep,
    channellayer_dep,
  ],
  install: true,
  install_dir: get_option('libdir') / 'ipmid-providers')

if get_option('examples')
  subdir('example')
endif

if get_option('tests')
  subdir('test')
endif
