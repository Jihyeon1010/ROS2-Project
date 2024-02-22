#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <image_transport/image_transport.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/highgui/highgui.hpp>
#include <vector>
#include <string>
#include <memory>
#include <system_error>

// Function to list potential video devices using v4l2-ctl 
std::vector<std::string> listVideoDevices() {
    std::vector<std::string> devices;
    FILE *pipe = popen("v4l2-ctl --list-devices", "r");

    if (!pipe) {
        throw std::system_error(errno, std::generic_category(), "Failed to execute v4l2-ctl");
    }

    char buffer[256];
    std::string result = "";

    while (!feof(pipe)) {
        if (fgets(buffer, 256, pipe) != NULL)
            result += buffer;
    }

    pclose(pipe);

    // Simple parsing for this example
    std::size_t pos = 0;
    while ((pos = result.find("\n", pos)) != std::string::npos) {
        std::string deviceLine = result.substr(0, pos);
        if (!deviceLine.empty() && deviceLine[0] == '/') {
            devices.push_back(deviceLine.substr(0, deviceLine.find("(")));
        }
        result.erase(0, pos + 1);
    }

    return devices;
}


class CameraNode : public rclcpp::Node
{
public:
    CameraNode() : Node("auto_camera_node")
    {
        search_interval_ = declare_parameter<double>("search_interval", 5.0);
        current_device_ = declare_parameter<std::string>("device", "/dev/video0");
        image_width_ = declare_parameter<int>("image_width", 640);
        image_height_ = declare_parameter<int>("image_height", 480);
        frame_id_ = declare_parameter<std::string>("frame_id", "camera_frame");

        searchAndStartCamera();

        search_timer_ = create_wall_timer(
            std::chrono::duration<double>(search_interval_),
            std::bind(&CameraNode::searchAndStartCamera, this)); 
    }

private:

    void searchAndStartCamera() {
        try {
            std::vector<std::string> devices = listVideoDevices();

            if (devices.empty()) {
                RCLCPP_INFO(get_logger(), "No cameras found.");
                return; 
            }

            // Simple logic: Try to open each device
            for (const auto& devicePath : devices) {
                if (startCamera(devicePath)) {
                    return; // Camera opened successfully
                }
            }

            RCLCPP_WARN(get_logger(), "Could not open any of the detected cameras.");

        } catch (const std::exception& e) {
            RCLCPP_ERROR(get_logger(), "Error listing video devices: %s", e.what());
        }
    }

    bool startCamera(const std::string& devicePath) {
        cap_.open(devicePath); 
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, image_width_);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, image_height_);

        if (!cap_.isOpened()) {
            RCLCPP_WARN(get_logger(), "Failed to open camera on '%s'", devicePath.c_str());
            return false;
        }

        current_device_ = devicePath; // Update parameter

        RCLCPP_INFO(get_logger(), "Started camera on '%s'", devicePath.c_str()); 

        image_transport::ImageTransport it(this);
        image_publisher_ = it.advertise("image_raw", 1);
        timer_ = create_wall_timer(
            std::chrono::duration<double>(timer_period_),
            std::bind(&CameraNode::timer_callback, this));

        return true;
    }

    // Rest of code similar to previous example...

};

int main(int argc, char * argv[]) { ... } 
