#pragma once

#include <QUrl>

#include <string>

QUrl id_to_url(std::string const& item_id, QUrl const& base_url);
std::string url_to_id(QUrl const& item_url, QUrl const& base_url);

std::string make_child_id(std::string const& parent_id, std::string const& name,
                          bool is_folder=false);

bool is_folder(std::string const& item_id);
