# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def configure(conf):
    # Check and configure required libraries
    libraries = {
        'bm': 'BM',
        'boost_system': 'BOOST',
        'simple_switch': 'SW'
    }

    for lib, store in libraries.items():
        conf.check_cfg(package=lib, uselib_store=store, args=['--cflags', '--libs'], mandatory=True)
    
def build(bld):
    req_ns3_modules = [
        'applications',
        'bridge',
        'core',
        'internet',
        'network',
        'point-to-point',
        'point-to-point-layout',
        'test',
        'topology-read',
        'traffic-control',
        'virtual-net-device'
    ]
    module = bld.create_ns3_module('p4sim', req_ns3_modules)
    module.source = [
        'model/p4sim.cc',
        'model/p4-bridge-channel.cc',
        'model/p4-controller.cc',
        'model/p4-topology-reader.cc',
        'model/p4-switch-core.cc',
        'model/p4-switch-interface.cc',
        'model/bridge-p4-net-device.cc',
        'model/standard-metadata-tag.cc',
        'model/p4-rr-pri-queue-disc.cc',
        'model/format-utils.cc',
        'model/switch-api.cc',
        'helper/p4-helper.cc',
        'helper/priority-port-tag.cc',
        'helper/global.cc',
        'helper/p4-exception-handle.cc',
        'helper/p4-topology-reader-helper.cc'
        ]

    # module.include = [
    #     '/home/vagrant/behavioral-model/third_party/spdlog/bm/',
    # ]
    
    module_test = bld.create_ns3_module_test_library('p4sim')
    module_test.source = [
        'test/p4sim-test-suite.cc',
        'test/format-utils-test-suite.cc',
        'test/p4-queue-disc-test-suite.cc',
        'test/p4-topology-reader-test-suite.cc',
        ]
    
    # Tests encapsulating example programs should be listed here
    if (bld.env['ENABLE_EXAMPLES']):
        module_test.source.extend([
        #    'test/p4sim-examples-test-suite.cc',
             ])

    headers = bld(features='ns3header')
    headers.module = 'p4sim'
    headers.source = [
        'model/p4sim.h',
        'model/p4-bridge-channel.h',
        'model/p4-controller.h',
        'model/p4-topology-reader.h',
        'model/p4-switch-core.h',
        'model/p4-switch-interface.h',
        'model/bridge-p4-net-device.h',
        'model/standard-metadata-tag.h',
        'model/p4-rr-pri-queue-disc.h',
        'model/format-utils.h',
        'model/switch-api.h',
        'model/register_access.h',
        'helper/p4-helper.h',
        'helper/priority-port-tag.h',
        'helper/global.h',
        'helper/p4-exception-handle.h',
        'helper/p4-topology-reader-helper.h'
        ]

    # Add library dependencies
    module.use += ['BM', 'BOOST', 'SW']

    # Recursive compilation example (if enabled)
    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # Generate Python bindings (if needed)
    # bld.ns3_python_bindings()

