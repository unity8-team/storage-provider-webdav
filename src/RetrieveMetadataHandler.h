#pragma once

#include <QObject>

#include <functional>
#include <memory>

#include "PropFindHandler.h"

class RetrieveMetadataHandler : public PropFindHandler {
    Q_OBJECT
public:
    typedef std::function<void(unity::storage::provider::Item const& item,
                               boost::exception_ptr const& error)> Callback;

    RetrieveMetadataHandler(DavProvider const& provider,
                            std::string const& item_id,
                            unity::storage::provider::Context const& ctx,
                            Callback callback);
    ~RetrieveMetadataHandler();

private:
    Callback const callback_;

protected:
    void finish() override;
};
