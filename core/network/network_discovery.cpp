#include "network_discovery.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace nes {

// Discovery Port
const int DISCOVERY_PORT = 6503;

struct DiscoveryPacket {
    char header[4]; // "NESD"
    char device_id[33];
    char username[32];
    char game_name[32];
    char rom_path[256];  // ROM path for validation
    uint16_t tcp_port;
};

NetworkDiscovery::NetworkDiscovery() 
    : udp_socket_(INVALID_SOCKET), running_(false), advertising_(false) {
}

NetworkDiscovery::~NetworkDiscovery() {
    shutdown();
}

bool NetworkDiscovery::init() {
    // WSAStartup is assumed to be called by NetworkManager or Main
    // But we can call it again just in case (it uses a reference counter)
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    udp_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create UDP socket" << std::endl;
        return false;
    }

    // Enable address reuse (allows multiple instances on same machine)
    int reuse = 1;
    if (setsockopt(udp_socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
        std::cerr << "Failed to enable SO_REUSEADDR" << std::endl;
    }

    // Enable Broadcast
    int broadcast = 1;
    if (setsockopt(udp_socket_, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) < 0) {
        std::cerr << "Failed to enable broadcast" << std::endl;
        return false;
    }

    // Bind to port to receive broadcasts
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_socket_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind UDP socket (Port " << DISCOVERY_PORT << " might be in use)" << std::endl;
        // We can still send broadcasts even if bind fails, but we won't receive any.
        // For now, let's allow it but warn.
    }

    running_ = true;
    receive_thread_ = std::thread(&NetworkDiscovery::receive_loop, this);
    
    return true;
}

void NetworkDiscovery::shutdown() {
    running_ = false;
    advertising_ = false;

    if (udp_socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(udp_socket_);
#else
        close(udp_socket_);
#endif
        udp_socket_ = INVALID_SOCKET;
    }

    if (broadcast_thread_.joinable()) broadcast_thread_.join();
    if (receive_thread_.joinable()) receive_thread_.join();

#ifdef _WIN32
    WSACleanup();
#endif
}

void NetworkDiscovery::start_advertising(const std::string& device_id, const std::string& username, 
                                         const std::string& game_name, const std::string& rom_path, uint16_t tcp_port) {
    if (advertising_) return;

    my_device_id_ = device_id;
    my_username_ = username;
    my_game_name_ = game_name;
    my_rom_path_ = rom_path;
    my_tcp_port_ = tcp_port;
    
    advertising_ = true;
    broadcast_thread_ = std::thread(&NetworkDiscovery::broadcast_loop, this);
}

void NetworkDiscovery::stop_advertising() {
    advertising_ = false;
    if (broadcast_thread_.joinable()) broadcast_thread_.join();
}

void NetworkDiscovery::broadcast_loop() {
    while (running_ && advertising_) {
        DiscoveryPacket packet;
        memcpy(packet.header, "NESD", 4);
        strncpy(packet.device_id, my_device_id_.c_str(), 32);
        packet.device_id[32] = '\0';
        strncpy(packet.username, my_username_.c_str(), 31);
        packet.username[31] = '\0';
        strncpy(packet.game_name, my_game_name_.c_str(), 31);
        packet.game_name[31] = '\0';
        strncpy(packet.rom_path, my_rom_path_.c_str(), 255);
        packet.rom_path[255] = '\0';
        packet.tcp_port = my_tcp_port_;

        sockaddr_in dest_addr;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(DISCOVERY_PORT);
        dest_addr.sin_addr.s_addr = INADDR_BROADCAST;

        sendto(udp_socket_, (char*)&packet, sizeof(packet), 0, (sockaddr*)&dest_addr, sizeof(dest_addr));

        // Broadcast every 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void NetworkDiscovery::receive_loop() {
    while (running_) {
        DiscoveryPacket packet;
        sockaddr_in sender_addr;
        int sender_len = sizeof(sender_addr);

        int received = recvfrom(udp_socket_, (char*)&packet, sizeof(packet), 0, (sockaddr*)&sender_addr, &sender_len);
        
        if (received == sizeof(DiscoveryPacket)) {
            if (memcmp(packet.header, "NESD", 4) == 0) {
                // Ignore our own packets
                if (advertising_ && my_device_id_ == packet.device_id) continue;

                std::lock_guard<std::mutex> lock(peers_mutex_);
                
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sender_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
                std::string ip(ip_str);

                // Update existing or add new
                bool found = false;
                for (auto& peer : peers_) {
                    if (peer.device_id == packet.device_id) {
                        peer.ip = ip; // Update IP in case it changed
                        peer.username = packet.username; // Update username
                        peer.game_name = packet.game_name;
                        peer.rom_path = packet.rom_path;
                        peer.port = packet.tcp_port;
                        peer.last_seen = std::chrono::steady_clock::now();
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    Peer new_peer;
                    new_peer.device_id = packet.device_id;
                    new_peer.ip = ip;
                    new_peer.username = packet.username;
                    new_peer.game_name = packet.game_name;
                    new_peer.rom_path = packet.rom_path;
                    new_peer.port = packet.tcp_port;
                    new_peer.last_seen = std::chrono::steady_clock::now();
                    peers_.push_back(new_peer);
                }
            }
        }
        
        // Small sleep to avoid CPU hogging if socket is non-blocking (it's blocking by default though)
        // If blocking, recvfrom waits. But if socket closed, it returns error.
    }
}

std::vector<NetworkDiscovery::Peer> NetworkDiscovery::get_peers() {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    // Remove stale peers (> 5 seconds)
    auto now = std::chrono::steady_clock::now();
    peers_.erase(std::remove_if(peers_.begin(), peers_.end(), 
        [&](const Peer& p) {
            return std::chrono::duration_cast<std::chrono::seconds>(now - p.last_seen).count() > 5;
        }), peers_.end());

    return peers_;
}

}
