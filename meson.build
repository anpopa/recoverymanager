project('recoverymanager', 'c',
  version: '0.1.0',
  meson_version: '>= 0.40.0',
  )

add_project_arguments([
  '-I' + meson.build_root(),
  ], language: 'c')

rmg_c_compiler_args = [
  '-Wall',
  '-Wextra',
  '-Wno-unused-function',
  '-Wformat',
  '-Wno-variadic-macros',
  '-Wno-strict-aliasing',
  '-fstack-protector-strong',
  '-fwrapv',
  '-Wformat-signedness',
  '-Wmissing-include-dirs',
  '-Wimplicit-fallthrough=5',
  '-Wunused-parameter',
  '-Wuninitialized',
  '-Walloca',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wfloat-equal',
  '-Wshadow',
  '-Wcast-qual',
  '-Wconversion',
  '-Wlogical-op',
  '-Werror',
  '-Wformat-security',
  '-Walloc-zero',
  '-Wcast-align',
  '-Wredundant-decls'
  ]

rmg_defaults_data = configuration_data()
rmg_defaults_data.set('version', meson.project_version())
rmg_defaults_data.set('config_dir', get_option('CONFDIR'))
rmg_defaults_data.set('install_prefix', get_option('prefix'))
configure_file(input : 'source/rmg-defaults.h.in',
  output : 'rmg-defaults.h',
  configuration : rmg_defaults_data)

rmg_msgtype_data = configuration_data()
rmg_msgtype_data.set('version', meson.project_version())
configure_file(input : 'source/rmg-message-type.h.in',
  output : 'rmg-message-type.h',
  configuration : rmg_msgtype_data)

rmg_config_data = configuration_data()
rmg_config_data.set('config_dir', get_option('CONFDIR'))
rmg_config_data.set('version', meson.project_version())
configure_file(input : 'config/recoverymanager.conf.in',
  output : 'recoverymanager.conf',
  configuration : rmg_config_data,
  install_dir : get_option('CONFDIR'))

dep_lxc = declare_dependency()
if get_option('LXC')
  add_project_arguments('-DWITH_LXC', language : 'c')
  dep_lxc = dependency('lxc', version : '>=3.0')
endif

dep_systemd = dependency('libsystemd')
sd_service_data = configuration_data()
sd_service_data.set('install_prefix', get_option('prefix'))
configure_file(input : 'config/recoverymanager.service.in',
    output : 'recoverymanager.service',
    configuration : sd_service_data,
    install_dir : '/etc/systemd/system')

dep_genivi_dlt = declare_dependency()
if get_option('GENIVI_DLT')
  add_project_arguments('-DWITH_GENIVI_DLT', language : 'c')
  dep_genivi_dlt = dependency('automotive-dlt', method : 'pkg-config')
endif

if get_option('TESTS')
  add_project_arguments('-DWITH_TESTS', language : 'c')
endif


dep_glib = dependency('glib-2.0', version : '>=2.58')
dep_sqlite = dependency('sqlite3')

if get_option('RMGRHOST')
	rmgrhost_sources = [
    	'common/rmg-message.c',
    	'common/rmg-logging.c',
    	'common/rmg-options.c',
    	'common/rmg-utils.c',
    	'common/rmg-sdnotify.c',
    	'common/rmg-journal.c',
    	'source/rmh-main.c',
    	'source/rmh-client.c',
    	'source/rmh-server.c',
    	'source/rmh-application.c',
    	]

	rmgrhost_deps = [
    	dep_glib,
    	dep_lxc,
    	dep_systemd,
    	dep_sqlite,
    	dep_genivi_dlt,
    	]

	executable('rmgrhost', 
    	rmgrhost_sources,
    	dependencies: rmgrhost_deps,
    	c_args: rmg_c_compiler_args,
    	install: true,
    	)
endif

if get_option('RMGRNODE')
	rmgrnode_sources = [
    	'common/rmg-message.c',
    	'common/rmg-logging.c',
    	'common/rmg-options.c',
    	'common/rmg-utils.c',
    	'common/rmg-sdnotify.c',
    	'common/rmg-journal.c',
    	'source/rmn-main.c',
    	'source/rmn-manager.c',
    	'source/rmn-application.c',
    	]

	rmgrnode_deps = [
    	dep_glib,
    	dep_lxc,
    	dep_systemd,
    	dep_sqlite,
    	dep_genivi_dlt,
    	]

	executable('rmgrnode', 
    	rmgrhost_sources,
    	dependencies: rmgrnode_deps,
    	c_args: rmg_c_compiler_args,
    	install: true,
    	)
endif
