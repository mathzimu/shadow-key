#pragma once
#include "platform_types.h"
#include <opencv2/opencv.hpp>
#include <string>

/// Region of the screen to capture.
struct CaptureRegion {
    int x, y, width, height;
};

/// Screen-capture helper.
///
/// On Windows it wraps BitBlt; on macOS it wraps CoreGraphics display
/// capture. Results are returned as OpenCV BGR Mats.
class ScreenCapture {
public:
    ScreenCapture() = delete;

    /// Capture the entire virtual desktop.
    [[nodiscard]] static cv::Mat capture_full();

    /// Capture a rectangular region.
    [[nodiscard]] static cv::Mat capture_region(const CaptureRegion& region);

#if defined(_WIN32)
    /// Capture the client area of a given window.
    [[nodiscard]] static cv::Mat capture_window(HWND hwnd);
#endif

    /// Save a screenshot to disk.
    /// @param path    Output file path (format chosen by extension).
    /// @param region  If null, captures the full screen.
    /// @return true on success.
    [[nodiscard]] static bool save_screenshot(
        const std::string& path,
        const CaptureRegion* region = nullptr);

#if defined(_WIN32)
private:
    [[nodiscard]] static cv::Mat hbitmap_to_mat(HBITMAP hbm,
                                                int width, int height);
#endif
};
