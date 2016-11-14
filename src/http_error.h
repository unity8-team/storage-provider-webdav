#pragma once

#include <boost/thread.hpp>

class QNetworkReply;

boost::exception_ptr translate_http_error(QNetworkReply *reply,
                                          std::string const& item_id={});
