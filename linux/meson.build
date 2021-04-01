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
	'qmk-hid-identify.cc',
	'hid-report-desc.cc',
	'../common/hid-device.cc',
	'../common/usb-vid-pid.cc',
]

cpp = meson.get_compiler('cpp')

if not (cpp.get_id() == 'gcc' and cpp.version().version_compare('<5'))
	if cpp.has_argument('-Wshadow')
		add_project_arguments('-Wshadow', language: 'cpp')
	endif
endif

executable('qmk-hid-identify',
	source_files,
	install: true)

run_target('cppcheck',
	command: [
		'cppcheck',
		'--project=' + join_paths(meson.current_build_dir(), 'compile_commands.json'),
		'--enable=all',
		'--inline-suppr',
	] + source_files)
