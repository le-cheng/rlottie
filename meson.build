project('rlottie',
        'cpp',
        default_options : ['cpp_std=c++14', 'optimization=s'],
        version : '0.2',
        license : 'MIT',
        meson_version : '>=0.49',
        )

add_project_arguments('-DDEMO_DIR="@0@/example/resource/"'.format(meson.current_source_dir()), language : 'cpp')

inc = [include_directories('inc')]
config_dir = include_directories('.')
inc += config_dir

config_h = configuration_data()


if get_option('module') == true
    config_h.set10('LOTTIE_IMAGE_MODULE_SUPPORT', true)

    if meson.get_compiler('cpp').get_id() != 'msvc'
      lib_prefix = 'lib'
    else
      lib_prefix = ''
    endif
    if host_machine.system() == 'darwin'
      lib_suffix = '.dylib'
    elif host_machine.system() == 'windows'
      lib_suffix = '.dll'
    else
      lib_suffix = '.so'
    endif
    if get_option('moduledir') != ''
      config_h.set_quoted('LOTTIE_IMAGE_MODULE_PLUGIN',
        get_option('prefix') / get_option('moduledir') / lib_prefix + 'rlottie-image-loader' + lib_suffix)
    else
      config_h.set_quoted('LOTTIE_IMAGE_MODULE_PLUGIN', lib_prefix + 'rlottie-image-loader' + lib_suffix)
    endif
endif

if get_option('cache') == true
    config_h.set10('LOTTIE_CACHE_SUPPORT', true)
endif

if get_option('log') == true
    config_h.set10('LOTTIE_LOGGING_SUPPORT', true)
endif

if get_option('dumptree') == true
    config_h.set10('LOTTIE_LOGGING_SUPPORT', true)
    config_h.set10('LOTTIE_DUMP_TREE_SUPPORT', true)
endif


configure_file(
  output: 'config.h',
  configuration: config_h
)


subdir('inc')
subdir('src')

if get_option('example') == true
    subdir('example')
endif

if get_option('test') == true
   subdir('test')
endif


if get_option('cmake') == true and host_machine.system() != 'windows'
    cmake_bin = find_program('cmake', required: false)
    if cmake_bin.found()
        cmake = import('cmake')
        cmake.write_basic_package_version_file(
            version: meson.project_version(),
            name: 'rlottie',
        )

        cmakeconf = configuration_data()
        cmakeconf.set('VERSION', meson.project_version())

        cmake.configure_package_config_file(
            input: meson.current_source_dir() + '/cmake/rlottieConfig.cmake.in',
            name: 'rlottie',
            configuration: cmakeconf,
        )
    endif
endif

summary = '''

Summary:
    rlottie version :        @0@
    Build type      :        @1@
    Thread Support  :        @2@
    Module Support  :        @3@
    Cache  Support  :        @4@
    Example         :        @5@
    Test            :        @6@
    Prefix          :        @7@
'''.format(
        meson.project_version(),
        get_option('buildtype'),
        get_option('thread'),
        get_option('module'),
        get_option('cache'),
        get_option('example'),
        get_option('test'),
        get_option('prefix'),
    )

message(summary)



