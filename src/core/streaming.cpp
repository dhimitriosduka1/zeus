#include <lightwave/image.hpp>
#include <lightwave/streaming.hpp>

#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>

#ifdef LW_OS_WINDOWS
#define USE_WIN32
#endif

#ifdef USE_WIN32
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <lightwave/iterators.hpp>

namespace lightwave {

#ifdef USE_WIN32
using socket_t = SOCKET;

/* The error string is the same in
 * http://msdn.microsoft.com/library/ms740668.aspx So more information about the
 * error could you find in this very nice page :D
 */
static inline const char *getErrorString() {
    switch (WSAGetLastError()) {
        case WSA_INVALID_HANDLE:
            return "Specified event object handle is invalid";
        case WSA_NOT_ENOUGH_MEMORY:
            return "Insufficient memory available";
        case WSA_INVALID_PARAMETER:
            return "One or more parameters are invalid";
        case WSA_OPERATION_ABORTED:
            return "Overlapped operation aborted";
        case WSA_IO_INCOMPLETE:
            return "Overlapped I/O event object not in signaled state";
        case WSA_IO_PENDING:
            return "Overlapped operations will complete later";
        case WSAEINTR:
            return "Interrupted function call";
        case WSAEBADF:
            return "File handle is not valid";
        case WSAEACCES:
            return "Permission denied";
        case WSAEFAULT:
            return "Bad address";
        case WSAEINVAL:
            return "Invalid argument";
        case WSAEMFILE:
            return "Too many open files";
        case WSAEWOULDBLOCK:
            return "Resource temporarily unavailable";
        case WSAEINPROGRESS:
            return "Operation now in progress";
        case WSAEALREADY:
            return "Operation already in progress";
        case WSAENOTSOCK:
            return "Socket operation on nonsocket";
        case WSAEDESTADDRREQ:
            return "Destination address required";
        case WSAEMSGSIZE:
            return "Message too long";
        case WSAEPROTOTYPE:
            return "Protocol wrong type for socket";
        case WSAENOPROTOOPT:
            return "Bad protocol option";
        case WSAEPROTONOSUPPORT:
            return "Protocol not supported";
        case WSAESOCKTNOSUPPORT:
            return "Socket type not supported";
        case WSAEOPNOTSUPP:
            return "Operation not supported";
        case WSAEPFNOSUPPORT:
            return "Protocol family not supported";
        case WSAEAFNOSUPPORT:
            return "Address family not supported by protocol family";
        case WSAEADDRINUSE:
            return "Address already in use";
        case WSAEADDRNOTAVAIL:
            return "Cannot assign requested address";
        case WSAENETDOWN:
            return "Network is down";
        case WSAENETUNREACH:
            return "Network is unreachable";
        case WSAENETRESET:
            return "Network dropped connection on reset";
        case WSAECONNABORTED:
            return "Software caused connection abort";
        case WSAECONNRESET:
            return "Connection reset by peer";
        case WSAENOBUFS:
            return "No buffer space available";
        case WSAEISCONN:
            return "Socket is already connected";
        case WSAENOTCONN:
            return "Socket is not connected";
        case WSAESHUTDOWN:
            return "Cannot send after socket shutdown";
        case WSAETOOMANYREFS:
            return "Too many references";
        case WSAETIMEDOUT:
            return "Connection timed out";
        case WSAECONNREFUSED:
            return "Connection refused";
        case WSAELOOP:
            return "Cannot translate name";
        case WSAENAMETOOLONG:
            return "Name too long";
        case WSAEHOSTDOWN:
            return "Host is down";
        case WSAEHOSTUNREACH:
            return "No route to host";
        case WSAENOTEMPTY:
            return "Directory not empty";
        case WSAEPROCLIM:
            return "Too many processes";
        case WSAEUSERS:
            return "User quota exceeded";
        case WSAEDQUOT:
            return "Disk quota exceeded";
        case WSAESTALE:
            return "Stale file handle reference";
        case WSAEREMOTE:
            return "Item is remote";
        case WSASYSNOTREADY:
            return "Network subsystem is unavailable";
        case WSAVERNOTSUPPORTED:
            return "Winsock.dll version out of range";
        case WSANOTINITIALISED:
            return "Successful WSAStartup not yet performed";
        case WSAEDISCON:
            return "Graceful shutdown in progress";
        case WSAENOMORE:
            return "No more results";
        case WSAECANCELLED:
            return "Call has been canceled";
        case WSAEINVALIDPROCTABLE:
            return "Procedure call table is invalid";
        case WSAEINVALIDPROVIDER:
            return "Service provider is invalid";
        case WSAEPROVIDERFAILEDINIT:
            return "Service provider failed to initialize";
        case WSASYSCALLFAILURE:
            return "System call failure";
        case WSASERVICE_NOT_FOUND:
            return "Service not found";
        case WSATYPE_NOT_FOUND:
            return "Class type not found";
        case WSA_E_NO_MORE:
            return "No more results";
        case WSA_E_CANCELLED:
            return "Call was canceled";
        case WSAEREFUSED:
            return "Database query was refused";
        case WSAHOST_NOT_FOUND:
            return "Host not found";
        case WSATRY_AGAIN:
            return "Nonauthoritative host not found";
        case WSANO_RECOVERY:
            return "This is a nonrecoverable error";
        case WSANO_DATA:
            return "Valid name, no data record of requested type";
        case WSA_QOS_RECEIVERS:
            return "QOS receivers";
        case WSA_QOS_SENDERS:
            return "QOS senders";
        case WSA_QOS_NO_SENDERS:
            return "No QOS senders";
        case WSA_QOS_NO_RECEIVERS:
            return "QOS no receivers";
        case WSA_QOS_REQUEST_CONFIRMED:
            return "QOS request confirmed";
        case WSA_QOS_ADMISSION_FAILURE:
            return "QOS admission error";
        case WSA_QOS_POLICY_FAILURE:
            return "QOS policy failure";
        case WSA_QOS_BAD_STYLE:
            return "QOS bad style";
        case WSA_QOS_BAD_OBJECT:
            return "QOS bad object";
        case WSA_QOS_TRAFFIC_CTRL_ERROR:
            return "QOS traffic control error";
        case WSA_QOS_GENERIC_ERROR:
            return "QOS generic error";
        case WSA_QOS_ESERVICETYPE:
            return "QOS service type error";
        case WSA_QOS_EFLOWSPEC:
            return "QOS flowspec error";
        case WSA_QOS_EPROVSPECBUF:
            return "Invalid QOS provider buffer";
        case WSA_QOS_EFILTERSTYLE:
            return "Invalid QOS filter style";
        case WSA_QOS_EFILTERTYPE:
            return "Invalid QOS filter type";
        case WSA_QOS_EFILTERCOUNT:
            return "Incorrect QOS filter count";
        case WSA_QOS_EOBJLENGTH:
            return "Invalid QOS object length";
        case WSA_QOS_EFLOWCOUNT:
            return "Incorrect QOS flow count";
        case WSA_QOS_EUNKOWNPSOBJ:
            return "Unrecognized QOS object";
        case WSA_QOS_EPOLICYOBJ:
            return "Invalid QOS policy object";
        case WSA_QOS_EFLOWDESC:
            return "Invalid QOS flow descriptor";
        case WSA_QOS_EPSFLOWSPEC:
            return "Invalid QOS provider-specific flowspec";
        case WSA_QOS_EPSFILTERSPEC:
            return "Invalid QOS provider-specific filterspec";
        case WSA_QOS_ESDMODEOBJ:
            return "Invalid QOS shape discard mode object";
        case WSA_QOS_ESHAPERATEOBJ:
            return "Invalid QOS shaping rate object";
        case WSA_QOS_RESERVED_PETYPE:
            return "Reserved policy QOS element type";
        default:
            return "Unknown error occured";
    }
}

static inline void printError() {
    logger(EWarn, "connection to tev failed: %s", getErrorString());
}

static inline bool isSocketError(int error) {
    if (error == SOCKET_ERROR) {
        printError();
        return true;
    }

    return false;
}

static inline bool getAddressFromString4(const std::string &ip,
                                         sockaddr_in &addr) {
    addrinfo *_addrinfo;
    bool good   = false;
    int errcode = ::getaddrinfo(ip.c_str(), nullptr, nullptr, &_addrinfo);
    if (errcode != 0) {
        logger(EWarn, "connection to tev failed: %s", getErrorString());
        return good;
    } else {
        for (auto it = _addrinfo; it; it = it->ai_next) {
            if (it->ai_family == AF_INET) {
                addr = *(sockaddr_in *) it->ai_addr;
                good = true;
                break;
            }
        }
    }

    freeaddrinfo(_addrinfo);
    return good;
}

static inline void closeSocket(socket_t socket) { ::closesocket(socket); }

static bool setBlocking(socket_t socket, bool is_blocking) {
    u_long state = is_blocking ? 0 : 1;
    int error    = ioctlsocket(socket, FIONBIO, &state);
    return !isSocketError(error);
}

static inline bool initNetwork() {
    WORD version;
    WSADATA wsa;

    version   = MAKEWORD(2, 2);
    int error = WSAStartup(version, &wsa);

    if (error) {
        logger(EWarn, "could not init WinSock2: %s", getErrorString());
        return false;
    }

    if (LOBYTE(wsa.wVersion) != 2 || HIBYTE(wsa.wVersion) != 2) {
        logger(EWarn, "could not find a usable version of Winsock.dll");
        WSACleanup();
        return false;
    }

    return true;
}

static inline void closeNetwork() { WSACleanup(); }

#else // Linux & macOS

using socket_t               = int;
constexpr int INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR   = -1;

static inline const char *getErrorString() { return ::strerror(errno); }

static inline void printError() {
    logger(EWarn, "connection to tev failed: %s", getErrorString());
}

static inline bool isSocketError(ssize_t error) {
    if (error == SOCKET_ERROR) {
        printError();
        return true;
    }

    return false;
}

static inline bool getAddressFromString4(const std::string &ip,
                                         sockaddr_in &addr) {
    addrinfo *_addrinfo;
    bool good   = false;
    int errcode = ::getaddrinfo(ip.c_str(), nullptr, nullptr, &_addrinfo);
    if (errcode != 0) {
        logger(EWarn, "connection to tev failed: ", gai_strerror(errcode));
        return good;
    } else {
        for (auto it = _addrinfo; it; it = it->ai_next) {
            if (it->ai_family == AF_INET) {
                addr = *(sockaddr_in *) it->ai_addr;
                good = true;
                break;
            }
        }
    }

    freeaddrinfo(_addrinfo);
    return good;
}

static inline void closeSocket(socket_t socket) { ::close(socket); }
#endif

class SocketInternal {
public:
    socket_t Socket = INVALID_SOCKET;
    std::string IP  = "127.0.0.1";
    uint16_t Port   = 0;

    bool IsClientConnection = false;
    bool IsOpen             = false;
    bool IsIp6              = false;

    SocketInternal() {
        static std::atomic_flag s_initialized = ATOMIC_FLAG_INIT;
        if (!s_initialized.test_and_set()) {
#ifdef USE_WIN32
            initNetwork(); // We should close the network support but why
                           // bother?
#else
            signal(SIGPIPE, SIG_IGN);
#endif
        }
    }

    ~SocketInternal() {
        if (Socket != INVALID_SOCKET)
            closeSocket(Socket);
    }

    // IPv4
    inline bool connect(uint16_t port, const std::string &ip) {
        assert(Socket == INVALID_SOCKET);

        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (Socket == INVALID_SOCKET) {
            logger(EWarn, "connection to tev failed: %s", getErrorString());
            return false;
        }

        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));

        if (!getAddressFromString4(ip, addr)) {
            logger(EWarn, "cannot convert IP address: %s", getErrorString());
            return false;
        }
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);

#ifdef LW_OS_WINDOWS
        // Set socket non-blocking
        if (!setBlocking(Socket, false))
            return false;

        int error = ::connect(Socket, (sockaddr *) &addr, sizeof(addr));

        if (error == SOCKET_ERROR) {
            // Handly async
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Some weird error happened, give up
                printError();
                return false;
            }

            // connection pending
            fd_set setW, setE;
            FD_ZERO(&setW);
            FD_SET(Socket, &setW);
            FD_ZERO(&setE);
            FD_SET(Socket, &setE);

            TIMEVAL time_out = { 0, 0 };
            time_out.tv_sec  = 0;
            time_out.tv_usec = 1000; // 1 ms

            error = select(0, NULL, &setW, &setE, &time_out);
            if (isSocketError(error))
                return false;

            if (error == 0) {
                logger(EWarn, "connection to tev failed: timeout");
                return false;
            }

            if (FD_ISSET(Socket, &setE)) {
                // connection failed
                int err = 0;
                int len = sizeof(err);
                getsockopt(Socket, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
                WSASetLastError(err);
                printError();
                return false;
            }
        }
        
        // Set socket back to be blocking
        if (!setBlocking(Socket, true))
            return false;
#else
        int error = ::connect(Socket, (sockaddr *) &addr, sizeof(addr));

        if (isSocketError(error))
            return false;
#endif

        IP   = ip;
        Port = port;
        return true;
    }
};

struct Streaming::UpdateThread {
    std::mutex mutex;
    std::thread thread;
    std::condition_variable cond;
    int interval = 500;
    bool stop    = false;

    UpdateThread(Streaming &streaming);
    ~UpdateThread();
};

class Streaming::Stream {
public:
    struct flush {};

    template <typename T> struct binary {
        const T *data;
        size_t len;

        binary(const T *d, size_t l) : data(d), len(l) {}
    };

    Stream();

    template <typename T> Stream &operator<<(const T &el) {
        if (m_buffer.size() < m_index + sizeof(T)) {
            m_buffer.resize(m_index + sizeof(T));
        }

        *(T *) &m_buffer[m_index] = el;
        m_index += sizeof(T);

        return *this;
    }

    template <typename T> Stream &operator<<(const std::vector<T> &el) {
        for (const auto &e : el) {
            *this << e;
        }

        return *this;
    }

    template <typename T> Stream &operator<<(binary<T> b) {
        const size_t len = b.len * sizeof(T);
        m_buffer.resize(m_index + len);
        memcpy(m_buffer.data() + m_index, b.data, len);
        m_index += (stream_size_t) len;

        return *this;
    }

    Stream &operator<<(const Vector2i &v) {
        *this << int32_t(v.x()) << int32_t(v.y());
        return *this;
    }

    Stream &operator<<(const std::string &el);
    Stream &operator<<(flush);

private:
#ifndef LW_OS_WINDOWS
    using stream_size_t = ssize_t;
#else
    using stream_size_t = int;
#endif

    void performFlush();
    void reset();

    std::vector<uint8_t> m_buffer;
    stream_size_t m_index = 0;
};

static std::unique_ptr<class SocketInternal> s_socket;
Streaming::Stream::Stream() {
    reset();

    if (s_socket) {
        // reuse existing socket
        return;
    }

    s_socket = std::make_unique<SocketInternal>();
    if (!s_socket->connect(14158, "127.0.0.1")) {
        // connection failed
        s_socket.reset();
    }
}

Streaming::Stream &Streaming::Stream::operator<<(const std::string &el) {
    const char *data = el.c_str();
    const size_t len = strlen(data) + 1;
    *this << binary(el.c_str(), len);

    return *this;
}

Streaming::Stream &Streaming::Stream::operator<<(flush) {
    performFlush();
    return *this;
}

void Streaming::Stream::performFlush() {
#ifdef USE_WIN32
#define SEND_FLAGS 0
#else
#define SEND_FLAGS MSG_NOSIGNAL
#endif

    *(uint32_t *) &m_buffer[0] = (uint32_t) m_index; // write size
    stream_size_t total        = 0;
    while (total < m_index && s_socket) {
        stream_size_t ret =
            ::send(s_socket->Socket, (const char *) m_buffer.data() + total,
                   (int) (m_index - total), SEND_FLAGS);
        if (isSocketError(ret)) {
            logger(EWarn, "connection to tev lost");

            // we were connected, now we aint
            s_socket.reset();
        } else if (ret == 0) {
            // give other side time to catch up
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        total += ret;
    }

    reset();
}

void Streaming::Stream::reset() {
    m_buffer.clear();
    m_index = 4;
}

Streaming::Streaming(const Image &image) : m_image(image) {
    m_stream = std::make_unique<Stream>();
    *m_stream
        // close existing image
        << char(2)      // type
        << m_image.id() // filename
        << Stream::flush()

        // create image
        << char(4)      // type
        << bool(true)   // grab focus
        << m_image.id() // filename
        << m_image.resolution() << int32_t(m_channels.size()) << m_channels
        << Stream::flush();
}

void Streaming::updateBlock(const Bounds2i &block) {
    std::unique_lock lock{ m_mutex };

    std::vector<float> data;
    for (int channel = 0; channel < Color::NumComponents; channel++) {
        data.clear();
        data.reserve(block.diagonal().product());
        for (auto pixel : block)
            data.push_back(m_image(pixel)[channel] * m_normalization);

        *m_stream
            // update channel
            << char(3) << bool(false) << m_image.id() << m_channels[channel]
            << block.min() << block.diagonal()
            << Stream::binary(data.data(), data.size()) << Stream::flush();
    }
}

void Streaming::update() {
    // we need to split up the image into smaller packets, since large ones
    // are not supported
    BlockSpiral queue{ Vector2i(m_image.resolution()), Vector2i(128) };
    for (auto block : queue) {
        updateBlock(block);
    }
}

Streaming::UpdateThread::UpdateThread(Streaming &streaming) {
    thread = std::thread([&]() {
        const auto duration = std::chrono::milliseconds(interval);
        while (!stop) {
            {
                std::unique_lock lock(mutex);
                cond.wait_for(lock, duration, [&]() { return stop; });
            }
            streaming.update();
        }
    });
}

Streaming::UpdateThread::~UpdateThread() {
    {
        std::lock_guard lock(mutex);
        stop = true;
    }
    cond.notify_one();
    thread.join();
}

void Streaming::startRegularUpdates() {
    m_updater = std::make_unique<UpdateThread>(*this);
}

void Streaming::stopRegularUpdates() { m_updater = nullptr; }

Streaming::~Streaming() { stopRegularUpdates(); }

} // namespace lightwave
