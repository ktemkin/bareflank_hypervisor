//
// Bareflank Hypervisor
//
// Copyright (C) 2015 Assured Information Security, Inc.
// Author: Rian Quinn        <quinnr@ainfosec.com>
// Author: Brendan Kerrigan  <kerriganb@ainfosec.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include <test.h>

vcpu_ut::vcpu_ut()
{
}

bool
vcpu_ut::init()
{
    return true;
}

bool
vcpu_ut::fini()
{
    return true;
}

bool
vcpu_ut::list()
{
    this->test_vcpu_factory_get_vcpu_invalid_vcpuid();
    this->test_vcpu_factory_get_vcpu_valid_vcpuid();
    this->test_vcpu_factory_add_vcpu_invalid_vcpuid();
    this->test_vcpu_factory_add_vcpu_success();

    this->test_vcpu_invalid_default_vcpu();
    this->test_vcpu_invalid_id_only_vcpu();
    this->test_vcpu_get_id();

    return true;
}

int
main(int argc, char *argv[])
{
    return RUN_ALL_TESTS(vcpu_ut);
}
