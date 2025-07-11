project(
    'phosphor-ecc',
    'cpp',
    version: '1.0',
    default_options: ['cpp_std=c++23'],
    meson_version: '>=1.1.1',
)

executable(
    'ecc_main',
    ['ecc_main.cpp', 'ecc_manager.cpp'],
    dependencies: [
        dependency('sdbusplus'),
        dependency('sdeventplus'),
        dependency('phosphor-dbus-interfaces'),
        dependency('phosphor-logging'),
    ],
    install: true,
    install_dir: get_option('bindir'),
)

install_data(sources: 'maxlog.conf', install_dir: '/etc/ecc')

conf_data = configuration_data()
conf_data.set('BUSNAME', '"xyz.openbmc_project.memory.ECC"')
conf_data.set('OBJROOT', '"/xyz/openbmc_project/metrics/memory/"')
conf_data.set('OBJPATH', '"/xyz/openbmc_project/metrics/memory/BmcECC"')
conf_data.set('sysfsRootPath', '"/sys/devices/system/edac/mc/mc0/"')
conf_data.set(
    'sysfsEDACReportPath',
    '"/sys/module/edac_core/parameters/edac_report"',
)
conf_data.set(
    'ECC_PHOSPHOR_LOGGING',
    get_option('ecc-phosphor-logging').enabled(),
)

configure_file(output: 'config.h', configuration: conf_data)

systemd = dependency('systemd')
conf_data = configuration_data()
conf_data.set('bindir', get_option('prefix') / get_option('bindir'))
configure_file(
    input: 'phosphor-ecc.service.in',
    output: 'phosphor-ecc.service',
    configuration: conf_data,
    install: true,
    install_dir: systemd.get_variable('systemd_system_unit_dir'),
)
