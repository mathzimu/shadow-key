#pragma once
#include <opencv2/opencv.hpp>
#include <windows.h>
#include <string>

/// Region of the screen to capture.
struct CaptureRegion {
    int x, y, width, height;
};

/// Screen-capture helper wrapping BitBlt.
///
/// Captures the full desktop, a specified region, or a given HWND
/// and returns the result as an OpenCV BGR Mat.
class ScreenCapture {
public:
    ScreenCapture() = delete;

    /// Capture the entire virtual desktop.
    [[nodiscard]] static cv::Mat capture_full();

    /// Capture a rectangular region.
    [[nodiscard]] static cv::Mat capture_region(const CaptureRegion& region);

    /// Capture the client area of a given window.
    [[nodiscard]] static cv::Mat capture_window(HWND hwnd);

    /// Save a screenshot to disk.
    /// @param path    Output file path (format chosen by extension).
    /// @param region  If null, captures the full screen.
    /// @return true on success.
    [[nodiscard]] static bool save_screenshot(
        const std::string& path,
        const CaptureRegion* region = nullptr);

private:
    [[nodiscard]] static cv::Mat hbitmap_to_mat(HBITMAP hbm,
                                                 int width, int height);
};
