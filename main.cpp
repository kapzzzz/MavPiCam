#include <mavsdk/mavsdk.h>
#include <sys/statvfs.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <thread>

using namespace mavsdk;
using namespace std::this_thread;
using namespace std::chrono;

#define RC_BUTTON_ON_THR 1500
#define RC_3SWITCH_THR1 1400
#define RC_3SWITCH_THR2 1600

enum class CameraMode { Idle, PhotoCapture, RecOn };

void usage(const std::string& bin_name) {
    std::cerr << "Usage : " << bin_name << " <connection_url>\n"
              << "Connection URL format should be :\n"
              << " For TCP : tcp://[server_host][:server_port]\n"
              << " For UDP : udp://[bind_host][:bind_port]\n"
              << " For Serial : serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}

std::string createFileName(const std::string& extension) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    char timeString[std::size("yyyy-mm-dd_hhmmss")];
    std::strftime(std::data(timeString), std::size(timeString), "%d-%m-%y_%H%M%S", std::localtime(&t_c));
    return std::string(timeString) + "." + extension;
}

bool checkFreeDiskSpace() {
    double size;
    struct statvfs buf;

    if ((statvfs("/", &buf)) < 0) {
        std::cout << "Failed to get statvfs" << std::endl;
        return false;
    } else {
        size = (((double)buf.f_bavail * buf.f_bsize) / 1048576);
        if (size < 1000.0) {
            std::cout << "There is less than 1GB space left. Rejecting..." << std::endl;
            return false;
        } else {
            printf("There is: %.0f MB free space left\n", size);
        }
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    int idleLogCtr{0};
    CameraMode cameraModeReq{CameraMode::Idle};
    CameraMode cameraModeActual{CameraMode::Idle};

    Mavsdk mavsdk;
    ConnectionResult connection_result = mavsdk.add_any_connection(argv[1]);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << '\n';
        return 1;
    }

    std::optional<std::shared_ptr<mavsdk::System>> system;
    do {
        system = mavsdk.first_autopilot(3.0);
        if (!system) {
            std::cerr << "Timed out waiting for system\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    } while (!system);

    mavsdk.intercept_incoming_messages_async([&cameraModeReq](mavlink_message_t& message) {
        if (message.msgid == MAVLINK_MSG_ID_RC_CHANNELS) {
            mavlink_rc_channels_t rc_channels;
            mavlink_msg_rc_channels_decode(&message, &rc_channels);

            if (rc_channels.chan6_raw < RC_3SWITCH_THR1) {
                // Idle
                cameraModeReq = CameraMode::Idle;
                if (rc_channels.chan5_raw > RC_BUTTON_ON_THR) {
                    // Photo trigger
                    cameraModeReq = CameraMode::PhotoCapture;
                }
            } else {
                cameraModeReq = CameraMode::RecOn;
            }

            return true;
        } else {
            return true;  // drop all other msgs //false prints tons of debug drops
        }
    });

    while (true) {
        // Camera mode state machine
        switch (cameraModeActual) {
            case CameraMode::Idle: {
                if (cameraModeReq == CameraMode::PhotoCapture) {
                    cameraModeActual = CameraMode::PhotoCapture;
                } else if (cameraModeReq == CameraMode::RecOn) {
                    cameraModeActual = CameraMode::RecOn;
                }
                if (idleLogCtr > 100) {
                    idleLogCtr = 0;
                    std::cout << "Camera mode: Idle" << std::endl;
                }
                idleLogCtr++;
                break;
            }
            // TODO:    free memory check and reject
            //          specify target directory
            //          flags
            //          servicefile
            case CameraMode::PhotoCapture: {
                if (checkFreeDiskSpace()) {
                    // libcamera-jpeg -n --rotation 180 -q 99 -o test.jpg
                    std::cout << "Capturing the photo..." << std::endl;
                    std::string cmd{"libcamera-jpeg -n --rotation 180 -q 99 -o " + createFileName("jpg")};
                    auto result = std::system(cmd.c_str());
                    if (result) {
                        std::cout << "Photo capture returned: " << result << std::endl;
                    }
                }

                cameraModeActual = CameraMode::Idle;
                break;
            }
            case CameraMode::RecOn: {
                if (checkFreeDiskSpace()) {
                    std::cout << "Video recording started..." << std::endl;
                    // libcamera-vid -n --rotation 180 --height 1080 --width 1920 --hdr -t 30000 -o test.h264
                    // --framerate 50
                    std::string cmd{
                        "libcamera-vid -n --rotation 180 --height 1080 --width 1920 --hdr -t 60000 --framerate 50 -o " +
                        createFileName("h264")};
                    auto result = std::system(cmd.c_str());
                    if (result) {
                        std::cout << "Video recording returned: " << result << std::endl;
                    }
                }

                cameraModeActual = CameraMode::Idle;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}

// TODO:
// - gps and other telemetry stuff in metadata?
// - use lib instead of calling the libcamera appps
