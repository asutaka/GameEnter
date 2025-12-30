#include "network_manager.h"
#include <iostream>
#include <cstring>
#ifdef _WIN32
#include <Ws2tcpip.h> // Cho TCP_NODELAY trên Windows
#else
#include <sys/socket.h>  // Cho setsockopt
#include <netinet/in.h>  // Cho IPPROTO_TCP
#include <netinet/tcp.h> // Cho TCP_NODELAY trên Linux/Android
#include <arpa/inet.h>   // Cho inet_pton
#include <unistd.h>      // Cho close
#endif

namespace nes {

NetworkManager::NetworkManager() 
    : state_(State::DISCONNECTED), is_host_(false), socket_(INVALID_SOCKET), listen_socket_(INVALID_SOCKET), running_(false) {
}

NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::init() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
#endif
    return true;
}

void NetworkManager::shutdown() {
    disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

void NetworkManager::disconnect() {
    running_ = false;
    
    if (socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET;
    }

    if (listen_socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(listen_socket_);
#else
        close(listen_socket_);
#endif
        listen_socket_ = INVALID_SOCKET;
    }

    if (network_thread_.joinable()) network_thread_.join();
    if (receive_thread_.joinable()) receive_thread_.join();
    
    state_ = State::DISCONNECTED;
    
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    input_queue_.clear();
}

bool NetworkManager::start_host(int port) {
    if (state_ != State::DISCONNECTED) return false;
    
    state_ = State::HOSTING;
    is_host_ = true;
    running_ = true;
    network_thread_ = std::thread(&NetworkManager::host_thread_func, this, port);
    return true;
}

bool NetworkManager::connect_to(const std::string& ip, int port) {
    if (state_ != State::DISCONNECTED) return false;
    
    state_ = State::CONNECTING;
    is_host_ = false;
    running_ = true;
    network_thread_ = std::thread(&NetworkManager::client_thread_func, this, ip, port);
    return true;
}

void NetworkManager::host_thread_func(int port) {
    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) {
        std::cerr << "Error creating listen socket" << std::endl;
        state_ = State::DISCONNECTED;
        return;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(listen_socket_, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
#ifdef _WIN32
        closesocket(listen_socket_);
#else
        close(listen_socket_);
#endif
        listen_socket_ = INVALID_SOCKET;
        state_ = State::DISCONNECTED;
        return;
    }

    if (listen(listen_socket_, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
#ifdef _WIN32
        closesocket(listen_socket_);
#else
        close(listen_socket_);
#endif
        listen_socket_ = INVALID_SOCKET;
        state_ = State::DISCONNECTED;
        return;
    }

    std::cout << "Hosting on port " << port << "..." << std::endl;

    // Accept a client
    SOCKET client_socket = accept(listen_socket_, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
        // This is expected if we close the socket to cancel hosting
        if (running_) {
             std::cerr << "Accept failed" << std::endl;
        }
        if (listen_socket_ != INVALID_SOCKET) {
#ifdef _WIN32
            closesocket(listen_socket_);
#else
            close(listen_socket_);
#endif
            listen_socket_ = INVALID_SOCKET;
        }
        state_ = State::DISCONNECTED;
        return;
    }

    // Close listen socket, we only support 1 client
    if (listen_socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(listen_socket_);
#else
        close(listen_socket_);
#endif
        listen_socket_ = INVALID_SOCKET;
    }
    
    socket_ = client_socket;
    
    // Disable Nagle Algorithm for low latency
    int flag = 1;
    setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

    state_ = State::CONNECTED;
    std::cout << "Client connected!" << std::endl;
    
    // Start receive loop
    receive_thread_ = std::thread(&NetworkManager::receive_loop, this);
}

void NetworkManager::client_thread_func(std::string ip, int port) {
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        state_ = State::DISCONNECTED;
        return;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    std::cout << "Connecting to " << ip << ":" << port << "..." << std::endl;

    if (connect(socket_, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed" << std::endl;
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET;
        state_ = State::DISCONNECTED;
        return;
    }

    // Disable Nagle Algorithm
    int flag = 1;
    setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

    state_ = State::CONNECTED;
    std::cout << "Connected to host!" << std::endl;

    // Start receive loop
    receive_thread_ = std::thread(&NetworkManager::receive_loop, this);
}

void NetworkManager::receive_loop() {
    while (running_ && state_ == State::CONNECTED) {
        // Read packet type (1 byte header)
        uint8_t packet_type = 0;
        int received = recv(socket_, (char*)&packet_type, 1, 0);
        if (received <= 0) {
            std::cout << "Connection lost or closed." << std::endl;
            state_ = State::DISCONNECTED;
            return;
        }
        
        if (packet_type == 0) {
            // Game input packet
            Packet packet;
            int total_received = 0;
            char* buffer = (char*)&packet;
            int bytes_to_read = sizeof(Packet);
            
            while (total_received < bytes_to_read) {
                int recv_bytes = recv(socket_, buffer + total_received, bytes_to_read - total_received, 0);
                if (recv_bytes <= 0) {
                    std::cout << "Connection lost or closed." << std::endl;
                    state_ = State::DISCONNECTED;
                    return;
                }
                total_received += recv_bytes;
            }
            
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            input_queue_.push_back(packet);
            
        } else if (packet_type == 1) {
            // Chat message
            ChatMessage chat_msg;
            int total_received = 0;
            char* buffer = (char*)&chat_msg;
            int bytes_to_read = sizeof(ChatMessage);
            
            while (total_received < bytes_to_read) {
                int recv_bytes = recv(socket_, buffer + total_received, bytes_to_read - total_received, 0);
                if (recv_bytes <= 0) {
                    std::cout << "Connection lost or closed." << std::endl;
                    state_ = State::DISCONNECTED;
                    return;
                }
                total_received += recv_bytes;
            }
            
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            chat_queue_.push_back(std::string(chat_msg.message));
        }
    }
}

bool NetworkManager::send_input(uint32_t frame_id, uint8_t input) {
    return send_input(frame_id, input, 0);  // No checksum
}

bool NetworkManager::send_input(uint32_t frame_id, uint8_t input, uint32_t checksum) {
    if (state_ != State::CONNECTED) return false;
    
    // Send packet type header (0 = game input)
    uint8_t packet_type = 0;
    send(socket_, (char*)&packet_type, 1, 0);
    
    Packet packet;
    packet.frame_id = frame_id;
    packet.input_state = input;
    packet.checksum = checksum;
    
    int sent = send(socket_, (char*)&packet, sizeof(Packet), 0);
    return sent == sizeof(Packet);
}

bool NetworkManager::pop_remote_input(Packet& out_packet) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (input_queue_.empty()) return false;
    
    out_packet = input_queue_.front();
    input_queue_.pop_front();
    return true;
}

bool NetworkManager::send_chat_message(const std::string& message) {
    if (state_ != State::CONNECTED) return false;
    if (message.empty() || message.length() >= 128) return false;
    
    // Send packet type header (1 = chat message)
    uint8_t packet_type = 1;
    send(socket_, (char*)&packet_type, 1, 0);
    
    ChatMessage chat_msg;
    std::memset(chat_msg.message, 0, sizeof(chat_msg.message));
    std::strncpy(chat_msg.message, message.c_str(), sizeof(chat_msg.message) - 1);
    
    int sent = send(socket_, (char*)&chat_msg, sizeof(ChatMessage), 0);
    return sent == sizeof(ChatMessage);
}

bool NetworkManager::pop_chat_message(std::string& out_message) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (chat_queue_.empty()) return false;
    
    out_message = chat_queue_.front();
    chat_queue_.pop_front();
    return true;
}

}
