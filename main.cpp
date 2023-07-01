#include <cstdint>
#include <future>
#include <mavsdk/mavsdk.h>
#include <iostream>
#include <thread>

using namespace mavsdk;
using namespace std::this_thread;
using namespace std::chrono;

#define RC_BUTTON_ON_THR 1500
#define RC_3SWITCH_THR1 1400
#define RC_3SWITCH_THR2 1600

enum class CameraMode {
    Idle,
    PhotoCapture,
    RecOff,
    RecOn
};

void usage(const std::string& bin_name)
{
    std::cerr << "Usage : " << bin_name << " <connection_url>\n"
              << "Connection URL format should be :\n"
              << " For TCP : tcp://[server_host][:server_port]\n"
              << " For UDP : udp://[bind_host][:bind_port]\n"
              << " For Serial : serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    CameraMode cameraMode{CameraMode::Idle};

    Mavsdk mavsdk;
    ConnectionResult connection_result = mavsdk.add_any_connection(argv[1]);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << '\n';
        return 1;
    }

    auto system = mavsdk.first_autopilot(3.0);
    if (!system) {
        std::cerr << "Timed out waiting for system\n";
        return 1;
    }

    mavsdk.intercept_incoming_messages_async([&cameraMode](mavlink_message_t& message) {
        if (message.msgid == MAVLINK_MSG_ID_RC_CHANNELS) {
            mavlink_rc_channels_t rc_channels;
            mavlink_msg_rc_channels_decode(&message, &rc_channels);

            if (rc_channels.chan6_raw < RC_3SWITCH_THR1 ) {
                // Idle
                cameraMode = CameraMode::Idle;
                if (rc_channels.chan5_raw > RC_BUTTON_ON_THR ) {
                    // Photo trigger
                    // std::cout << "Photo trigger" << rc_channels.chan5_raw << '\n';
                    cameraMode = CameraMode::PhotoCapture;
                }
            } else if (rc_channels.chan6_raw > RC_3SWITCH_THR1 && rc_channels.chan6_raw < RC_3SWITCH_THR2) {
                // RecOff
                cameraMode = CameraMode::RecOff;
            } else {
                // RecOn
                cameraMode = CameraMode::RecOn;
            }

            return true;
        } else {
            return true; // drop all other msgs //false prints tons of debug drops
        }
    });


    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "Camera mode: " << (int)cameraMode << '\n';
    }

    return 0;
}
