/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 */

#pragma once

#include <QUrl>

#include <string>

QUrl id_to_url(std::string const& item_id, QUrl const& base_url);
std::string url_to_id(QUrl const& item_url, QUrl const& base_url);

std::string make_child_id(std::string const& parent_id, std::string const& name,
                          bool is_folder=false);

bool is_folder(std::string const& item_id);
