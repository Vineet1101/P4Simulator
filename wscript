# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# # Check the libs (Deprecated, with set_pkg_config_env.sh)
# def configure(conf):
#     # Check and configure required libraries
#     libraries = {
#         'bm': 'BM',
#         'boost_system': 'BOOST',
#         'simple_switch': 'SW'
#     }
#     for lib, store in libraries.items():
#         conf.check_cfg(package=lib, uselib_store=store, args=['--cflags', '--libs'], mandatory=True)
    
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
        'model/p4-queue.cc',
        'model/p4-bridge-channel.cc',
        'model/p4-p2p-channel.cc',
        'model/custom-header.cc',
        'model/p4-controller.cc',
        'model/p4-topology-reader.cc',
        'model/p4-switch-core.cc',
        "model/p4-psa-switch-core.cc",
        'model/p4-switch-interface.cc',
        'model/bridge-p4-net-device.cc',
        'model/custom-p2p-net-device.cc',
        'model/format-utils.cc',
        'model/switch-api.cc',
        'helper/p4-helper.cc',
        'helper/global.cc',
        'helper/p4-exception-handle.cc',
        'helper/p4-topology-reader-helper.cc',
        'helper/p4-p2p-helper.cc'
        ]

    module_test = bld.create_ns3_module_test_library('p4sim')
    module_test.source = [
        # 'test/p4sim-test-suite.cc',
        # 'test/format-utils-test-suite.cc',
        # # 'test/p4-queue-disc-test-suite.cc',
        # 'test/p4-topology-reader-test-suite.cc',
        # 'test/p4-p2p-channel-test-suite.cc',
        ]
    
    # Tests encapsulating example programs should be listed here
    if (bld.env['ENABLE_EXAMPLES']):
        module_test.source.extend([
        #    'test/p4sim-examples-test-suite.cc',
             ])

    headers = bld(features='ns3header')
    headers.module = 'p4sim'
    headers.source = [
        'model/p4-queue.h',
        'model/p4-bridge-channel.h',
        'model/p4-p2p-channel.h',
        'model/custom-header.h',
        'model/p4-controller.h',
        'model/p4-topology-reader.h',
        'model/p4-switch-core.h',
        "model/p4-psa-switch-core.h",
        'model/p4-switch-interface.h',
        'model/bridge-p4-net-device.h',
        'model/custom-p2p-net-device.h',
        'model/format-utils.h',
        'model/switch-api.h',
        'model/register_access.h',
        'helper/p4-helper.h',
        'helper/global.h',
        'helper/p4-exception-handle.h',
        'helper/p4-topology-reader-helper.h',
        'helper/p4-p2p-helper.h'
        ]

    # Add library dependencies (Deprecated)
    # module.use += ['BM', 'BOOST', 'SW']
    # module.use += ['BM']
    
    # Add library dependencies
    module.uselib_local = ['BM']
    module.includes = ['/usr/local/include']
    module.lib = ['bmall', 'bmpi', 'pi', 'gmp']
    module.libpath = ['/usr/local/lib']
    
    # Recursive compilation example (if enabled)
    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # Generate Python bindings (if needed)
    # bld.ns3_python_bindings()

