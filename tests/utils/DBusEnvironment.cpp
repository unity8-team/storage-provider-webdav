/*
 * Copyright (C) 2016 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: James Henstridge <james.henstridge@canonical.com>
 */

#include "DBusEnvironment.h"

#include <testsetup.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>

DBusEnvironment::DBusEnvironment()
    : runner_(new QtDBusTest::DBusTestRunner())
{
}

DBusEnvironment::~DBusEnvironment() = default;

QDBusConnection const& DBusEnvironment::connection() const
{
    return runner_->sessionConnection();
}

QString const& DBusEnvironment::busAddress() const
{
    return runner_->sessionBus();
}
