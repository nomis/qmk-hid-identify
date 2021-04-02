project('qmk-hid-identify',
	['cpp'],
	default_options: [
		'cpp_std=c++17',
		'warning_level=3',
		'werror=true',
	],
	license: 'GPL3+',
	meson_version: '>=0.53.0',
)

source_files = [
	'main.cc',
	'hid-identify.cc',
	'windows++.cc',
	'../common/hid-device.cc',
	'../common/usb-vid-pid.cc',
]

cpp = meson.get_compiler('cpp')

if not (cpp.get_id() == 'gcc' and cpp.version().version_compare('<5'))
	if cpp.has_argument('-Wshadow')
		add_project_arguments('-Wshadow', language: 'cpp')
	endif
endif

add_project_arguments('-DNTDDI_VERSION=0x0A000000', language: 'cpp')
add_project_arguments('-D_WIN32_WINNT=0x0A00', language: 'cpp')

add_project_arguments('-DNOGDI', language: 'cpp')
# Workaround old headers that haven't been updated to handle NOGDI properly
add_project_arguments('-DLPLOGFONTA=LPVOID', language: 'cpp')
add_project_arguments('-DLPLOGFONTW=LPVOID', language: 'cpp')
add_project_arguments('-DLPDEVMODE=LPVOID', language: 'cpp')

cpp_libs = [
	cpp.find_library('setupapi'),
	cpp.find_library('hid'),
]

executable('qmk-hid-identifyA',
	source_files,
	dependencies: cpp_libs)

executable('qmk-hid-identifyW',
	source_files,
	cpp_args: ['-DUNICODE'],
	dependencies: cpp_libs,
	install: true)

run_target('cppcheck',
	command: [
		'cppcheck',
		'--project=' + join_paths(meson.current_build_dir(), 'compile_commands.json'),
		'--enable=all',
		'--inline-suppr',
	] + source_files)
