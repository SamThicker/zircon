// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/binding.h>
#include <ddk/protocol/test.h>

#include <unittest/unittest.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

extern struct test_case_element* test_case_ddk_metadata;
extern struct test_case_element* test_case_ddk_usb_request;

zx_device_t* ddk_test_dev;

static void ddk_test_output_func(const char* line, int len, void* arg) {
    zx_handle_t h = *(zx_handle_t*)arg;
    // len is not actually the number of bytes to output
    zx_socket_write(h, 0u, line, strlen(line), NULL);
}

static void update_test_report(bool success, test_report_t* report) {
    report->n_tests++;
    if (success) {
        report->n_success++;
    } else {
        report->n_failed++;
    }
}

static zx_status_t ddk_test_func(void* cookie, const void* arg, size_t arglen, test_report_t* report) {
    zx_device_t* dev = (zx_device_t*)cookie;

    test_protocol_t proto;
    zx_status_t status = device_get_protocol(dev, ZX_PROTOCOL_TEST, &proto);
    if (status != ZX_OK) {
        return status;
    }

    zx_handle_t output = proto.ops->get_output_socket(proto.ctx);
    if (output != ZX_HANDLE_INVALID) {
        unittest_set_output_function(ddk_test_output_func, &output);
    }

    memset(report, 0, sizeof(*report));
    update_test_report(unittest_run_one_test(test_case_ddk_metadata, TEST_ALL), report);
    update_test_report(unittest_run_one_test(test_case_ddk_usb_request, TEST_ALL), report);
    return report->n_failed == 0 ? ZX_OK : ZX_ERR_INTERNAL;
}

zx_status_t ddk_test_bind(void* ctx, zx_device_t* parent) {
    test_protocol_t proto;
    zx_status_t status = device_get_protocol(parent, ZX_PROTOCOL_TEST, &proto);
    if (status != ZX_OK) {
        return status;
    }

    ddk_test_dev = parent;
    proto.ops->set_test_func(proto.ctx, &(test_func_t){ddk_test_func, parent});
    return ZX_OK;
}
