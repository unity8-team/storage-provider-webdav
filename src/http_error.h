#pragma once

#include <boost/thread.hpp>

class QByteArray;
class QNetworkReply;

static constexpr int MAX_ERROR_BODY_LENGTH = 64 * 1024;

boost::exception_ptr translate_http_error(QNetworkReply *reply,
                                          QByteArray const& body,
                                          std::string const& item_id={});
