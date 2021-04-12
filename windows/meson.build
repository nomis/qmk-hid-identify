project('qmk-hid-identify',
	['cpp'],
	default_options: [
		'cpp_std=c++17',
		'warning_level=3',
		'werror=true',
		'b_lto=false', # Causes stack corruption with -static
	],
	license: 'GPL3+',
	meson_version: '>=0.53.0',
)

windows = import('windows')

events_files = custom_target('events',
	input: 'events.mc',
	output: ['events.h', 'events.rc', 'events_en_GB.bin'],
	command: [find_program('windmc'), '-C', '65001', '-U', '-b', '-h', '@OUTDIR@', '-r', '@OUTDIR@', '@INPUT@'])

vcs_file = vcs_tag(input: 'version.h.in', output: 'version.h')

resource_file = windows.compile_resources('app.rc',
	depends: [events_files, vcs_file],
	# Workaround for https://github.com/mesonbuild/meson/issues/8612
	include_directories: include_directories('.'))

source_files = [
	files(
		'main.cc',
		'hid-enumerate.cc',
		'hid-identify.cc',
		'registry.cc',
		'service.cc',
		'service-control.cc',
		'windows++.cc',
		'../common/hid-device.cc',
		'../common/usb-vid-pid.cc',
	),
	resource_file,
]

cpp = meson.get_compiler('cpp')

if not (cpp.get_id() == 'gcc' and cpp.version().version_compare('<5'))
	if cpp.has_argument('-Wshadow')
		add_project_arguments('-Wshadow', language: 'cpp')
	endif
endif

add_project_arguments('-DNTDDI_VERSION=0x0A000000', language: 'cpp')
add_project_arguments('-D_WIN32_WINNT=0x0A00', language: 'cpp')

add_project_arguments('-DUNICODE', language: 'cpp')
add_project_arguments('-DNOGDI', language: 'cpp')
# Workaround old headers that haven't been updated to handle NOGDI properly
add_project_arguments('-DLPLOGFONTA=LPVOID', language: 'cpp')
add_project_arguments('-DLPLOGFONTW=LPVOID', language: 'cpp')
add_project_arguments('-DLPDEVMODE=LPVOID', language: 'cpp')

add_project_link_arguments('-municode', language: 'cpp')
add_project_link_arguments('-static', language: 'cpp')

cpp_libs = [
	cpp.find_library('setupapi'),
	cpp.find_library('hid'),
	cpp.find_library('ktmw32'),
]

executable('qmk-hid-identify',
	source_files,
	dependencies: cpp_libs,
	install: true)

cppcheck = find_program('cppcheck', required: false)
if cppcheck.found()
	run_target('cppcheck',
		command: [
			cppcheck,
			'--project=' + join_paths(meson.current_build_dir(), 'compile_commands.json'),
			'--enable=all',
			'--inline-suppr',
		] + source_files)
endif
