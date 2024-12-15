#ifndef NETINTERFACE_H
#define NETINTERFACE_H

#include <string>
#include <memory>

class NetInterface
{
public:
    using Ptr = std::shared_ptr<NetInterface>;
    std::string name_;

    // Download
    std::int64_t rx_bytes_;
    std::int64_t rx_mibytes_;
    std::int64_t rx_gibytes_;
    std::int64_t rx_pkts_;
    std::int64_t rx_errs_;
    std::int64_t rx_drop_;

    // Upload
    std::int64_t tx_bytes_;
    std::int64_t tx_mibytes_;
    std::int64_t tx_gibytes_;
    std::int64_t tx_pkts_;
    std::int64_t tx_errs_;
    std::int64_t tx_drop_;

    NetInterface(const std::string& name,
                 std::int64_t rx_bytes,
                 std::int64_t tx_bytes,
                 std::int64_t rx_pkts = 0,
                 std::int64_t rx_errs = 0,
                 std::int64_t rx_drop = 0,
                 std::int64_t tx_pkts = 0,
                 std::int64_t tx_errs = 0,
                 std::int64_t tx_drop = 0);

    bool operator==(const NetInterface&) const;
    NetInterface& operator=(const NetInterface&);
};

#endif // NETINTERFACE_H
