#pragma once

#include <QUrl>

#include <string>

QUrl id_to_url(std::string const& item_id, QUrl const& base_url);
std::string url_to_id(QUrl const& item_url, QUrl const& base_url);
