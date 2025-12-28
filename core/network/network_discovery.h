#ifndef NETWORK_DISCOVERY_H
#define NETWORK_DISCOVERY_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    // Linux/Mac includes
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

namespace nes {

class NetworkDiscovery {
public:
    struct Peer {
        std::string device_id;
        std::string ip;
        std::string username;
        std::string game_name;
        std::string rom_path;  // ROM path for validation
        uint16_t port;
        std::chrono::steady_clock::time_point last_seen;
    };

    NetworkDiscovery();
    ~NetworkDiscovery();

    bool init();
    void shutdown();

    // Start broadcasting presence
    void start_advertising(const std::string& device_id, const std::string& username, 
                          const std::string& game_name, const std::string& rom_path, uint16_t tcp_port);
    
    // Stop broadcasting
    void stop_advertising();

    // Get list of currently active peers
    std::vector<Peer> get_peers();

private:
    void broadcast_loop();
    void receive_loop();

    SOCKET udp_socket_;
    std::thread broadcast_thread_;
    std::thread receive_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> advertising_;

    // Advertising data
    std::string my_device_id_;
    std::string my_username_;
    std::string my_game_name_;
    std::string my_rom_path_;
    uint16_t my_tcp_port_;

    // Peers list
    std::mutex peers_mutex_;
    std::vector<Peer> peers_;
};

}

#endif // NETWORK_DISCOVERY_H
