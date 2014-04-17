# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('ns3-worm', ['core', 'internet'])
    module.source = [
        'model/ns3-worm.cc',
        'model/ns3-wormudp.cc',
        # 'model/ns3-wormtcp.cc',
        'helper/ns3-worm-helper.cc',
        'model/ns3_wormtargetvector.cc',
        ]

    module_test = bld.create_ns3_module_test_library('ns3-worm')
    module_test.source = [
        'test/ns3-worm-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'ns3-worm'
    headers.source = [
        'model/ns3-worm.h',
        'model/ns3-wormudp.h',
        # 'model/ns3-wormtcp.h',
        'helper/ns3-worm-helper.h',
        'model/ns3_wormtargetvector.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

