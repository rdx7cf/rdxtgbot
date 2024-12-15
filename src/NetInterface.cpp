#include "NetInterface.h"

// NetInterface

NetInterface::NetInterface(const std::string& name,
             std::int64_t rx_bytes,
             std::int64_t tx_bytes,
             std::int64_t rx_pkts,
             std::int64_t rx_errs,
             std::int64_t rx_drop,
             std::int64_t tx_pkts,
             std::int64_t tx_errs,
             std::int64_t tx_drop)
    : name_(name),
      rx_bytes_(rx_bytes), rx_pkts_(rx_pkts), rx_errs_(rx_errs), rx_drop_(rx_drop),
      tx_bytes_(tx_bytes), tx_pkts_(tx_pkts), tx_errs_(tx_errs), tx_drop_(tx_drop)
{
    tx_mibytes_ = tx_bytes_ >> 20;
    tx_gibytes_ = tx_bytes_ >> 30;

    rx_mibytes_ = rx_bytes_ >> 20;
    rx_gibytes_ = rx_bytes_ >> 30;
}

bool NetInterface::operator==(const NetInterface& entry) const
{
    if(name_ != entry.name_)
        return false;
    if(rx_bytes_ != entry.rx_bytes_)
        return false;
    if(rx_mibytes_ != entry.rx_mibytes_)
        return false;
    if(rx_gibytes_ != entry.rx_gibytes_)
        return false;
    if(rx_pkts_ != entry.rx_pkts_)
        return false;
    if(rx_errs_ != entry.rx_errs_)
        return false;
    if(rx_drop_ != entry.rx_drop_)
        return false;
    if(tx_bytes_ != entry.tx_bytes_)
        return false;
    if(tx_mibytes_ != entry.tx_mibytes_)
        return false;
    if(tx_gibytes_ != entry.tx_gibytes_)
        return false;
    if(tx_pkts_ != entry.tx_pkts_)
        return false;
    if(tx_errs_ != entry.tx_errs_)
        return false;
    if(tx_drop_ != entry.tx_drop_)
        return false;

    return true;
}

NetInterface& NetInterface::operator=(const NetInterface& entry)
{
    if(this == &entry)
        return *this;

    name_ = entry.name_;
    rx_bytes_ = entry.rx_bytes_;
    rx_mibytes_ = entry.rx_mibytes_;
    rx_gibytes_ = entry.rx_gibytes_;
    rx_pkts_ = entry.rx_pkts_;
    rx_errs_ = entry.rx_errs_;
    rx_drop_ = entry.rx_drop_;
    tx_bytes_ = entry.tx_bytes_;
    tx_mibytes_ = entry.tx_mibytes_;
    tx_gibytes_ = entry.tx_gibytes_;
    tx_pkts_ = entry.tx_pkts_;
    tx_errs_ = entry.tx_errs_;
    tx_drop_ = entry.tx_drop_;

    return *this;
}
