
namespace FTSSrv2 {

class ConnectionWaiter {
public:
    virtual ~ConnectionWaiter() {};

    virtual int init(std::uint16_t in_usPort) = 0;
    virtual bool waitForThenDoConnection(std::int64_t in_ulMaxWaitMillisec = FTSC_TIME_OUT) = 0;

protected:
    ConnectionWaiter() {};
};

} // namespace FTSSrv2
