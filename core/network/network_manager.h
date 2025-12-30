#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // Link with ws2_32.lib
    // Note: In CMake we will need to link this library
#else
    // Linux/Mac includes (Future proofing)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

namespace nes {

class NetworkManager {
public:
    enum class State {
        DISCONNECTED,
        HOSTING,     // Waiting for client
        CONNECTING,  // Trying to connect to host
        CONNECTED    // Connection established
    };

    struct Packet {
        uint32_t frame_id;
        uint8_t input_state;
        uint32_t checksum;  // Game state checksum (0 if not a checksum frame)
    };
    
    struct ChatMessage {
        char message[128];  // Max 127 characters + null terminator
    };

    NetworkManager();
    ~NetworkManager();

    bool init();
    void shutdown();

    // Host mode: Start listening for connections
    bool start_host(int port = 6502);
    
    // Client mode: Connect to a host
    bool connect_to(const std::string& ip, int port = 6502);
    
    // Common
    void disconnect();
    
    // Send local input for a specific frame
    bool send_input(uint32_t frame_id, uint8_t input);
    
    // Send local input with checksum (for desync detection)
    bool send_input(uint32_t frame_id, uint8_t input, uint32_t checksum);
    
    // Chat functions (for lobby)
    bool send_chat_message(const std::string& message);
    bool pop_chat_message(std::string& out_message);
    
    // Try to pop the next input packet from the remote player
    // Returns true if a packet was retrieved
    bool pop_remote_input(Packet& out_packet);
    
    State get_state() const { return state_; }
    bool is_connected() const { return state_ == State::CONNECTED; }
    bool is_host() const { return is_host_; }

private:
    void host_thread_func(int port);
    void client_thread_func(std::string ip, int port);
    void receive_loop(); // Main loop for receiving data once connected

    std::atomic<State> state_;
    std::atomic<bool> is_host_;
    SOCKET socket_;
    SOCKET listen_socket_ = INVALID_SOCKET; // Added to allow closing from disconnect()
    std::thread network_thread_;
    std::thread receive_thread_;
    std::atomic<bool> running_;
    
    // Input buffer (Thread safe)
    std::mutex buffer_mutex_;
    std::deque<Packet> input_queue_;
    std::deque<std::string> chat_queue_;  // Chat messages
};

}

#endif // NETWORK_MANAGER_H
