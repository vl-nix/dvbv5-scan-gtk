project('dvbv5-gtk', 'c', version: '21.0', meson_version: '>= 0.50.0', default_options: ['warning_level=2', 'c_std=gnu11'])

cc = meson.get_compiler('c')

c_args = ['-DVERSION="' + meson.project_version() + '"']

c_args += ['-DPROGNAME="Dvbv5-Gtk"']

c_args = ['-DVERSION="' + meson.project_version() + '"']

test_cflags = [
    '-Wconversion',
    '-fno-strict-aliasing',
    '-Wpointer-arith',
    '-Wmissing-declarations',
    '-Wmissing-prototypes',
    '-Wimplicit-function-declaration',
    '-Wformat=2',
    '-Wformat-security',
    '-Wnested-externs',
    '-Wold-style-definition',
    '-Wundef',
    '-Wunused',
    '-Wcast-align',
    '-Wmissing-noreturn',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wlogical-op',
    '-Wignored-qualifiers',
    '-Wno-discarded-qualifiers',
    '-Werror=implicit',
    '-Werror=nonnull',
    '-Werror=init-self',
    '-Werror=main',
    '-Werror=missing-braces',
    '-Werror=sequence-point',
    '-Werror=return-type',
    '-Werror=trigraphs',
    '-Werror=array-bounds',
    '-Werror=write-strings',
    '-Werror=address',
    '-Werror=int-to-pointer-cast',
    '-Werror=pointer-to-int-cast',
    '-Werror=empty-body',
    '-Werror=write-strings'
]

common_cflags = cc.get_supported_arguments(test_cflags)
c_args += common_cflags

dvb5_deps = [dependency('gtk+-3.0', version: '>= 3.22'), dependency('libdvbv5', version: '>= 1.18')]

c = run_command('sh', '-c', 'for file in src/*.h src/*.c; do echo $file; done')
src = c.stdout().strip().split('\n')

executable(meson.project_name(), src, dependencies: dvb5_deps, c_args: c_args, install: true)
