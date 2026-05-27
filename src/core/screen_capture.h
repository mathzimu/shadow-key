#pragma once
#include <opencv2/opencv.hpp>
#include <windows.h>

struct CaptureRegion {
    int x, y, width, height;
};

class ScreenCapture {
public:
    static cv::Mat capture_full();
    static cv::Mat capture_region(const CaptureRegion& region);
    static cv::Mat capture_window(HWND hwnd);
    static bool save_screenshot(const std::string& path, const CaptureRegion* region = nullptr);

private:
    static cv::Mat hbitmap_to_mat(HBITMAP hbm, int width, int height);
};
