#include "screen_capture.h"
#include "utils/logger.h"

cv::Mat ScreenCapture::capture_full() {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC hdc_screen = GetDC(nullptr);
    HDC hdc_mem = CreateCompatibleDC(hdc_screen);
    HBITMAP hbm = CreateCompatibleBitmap(hdc_screen, width, height);
    SelectObject(hdc_mem, hbm);

    if (!BitBlt(hdc_mem, 0, 0, width, height, hdc_screen, 0, 0, SRCCOPY)) {
        LOG_ERROR("BitBlt full screen failed: {}", GetLastError());
        DeleteObject(hbm);
        DeleteDC(hdc_mem);
        ReleaseDC(nullptr, hdc_screen);
        return {};
    }

    cv::Mat mat = hbitmap_to_mat(hbm, width, height);

    DeleteObject(hbm);
    DeleteDC(hdc_mem);
    ReleaseDC(nullptr, hdc_screen);

    return mat;
}

cv::Mat ScreenCapture::capture_region(const CaptureRegion& region) {
    HDC hdc_screen = GetDC(nullptr);
    HDC hdc_mem = CreateCompatibleDC(hdc_screen);
    HBITMAP hbm = CreateCompatibleBitmap(hdc_screen, region.width, region.height);
    SelectObject(hdc_mem, hbm);

    if (!BitBlt(hdc_mem, 0, 0, region.width, region.height,
                hdc_screen, region.x, region.y, SRCCOPY)) {
        LOG_ERROR("BitBlt region failed: {}", GetLastError());
        DeleteObject(hbm);
        DeleteDC(hdc_mem);
        ReleaseDC(nullptr, hdc_screen);
        return {};
    }

    cv::Mat mat = hbitmap_to_mat(hbm, region.width, region.height);

    DeleteObject(hbm);
    DeleteDC(hdc_mem);
    ReleaseDC(nullptr, hdc_screen);

    return mat;
}

cv::Mat ScreenCapture::capture_window(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        LOG_ERROR("Invalid window handle");
        return {};
    }

    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        LOG_ERROR("GetWindowRect failed: {}", GetLastError());
        return {};
    }

    CaptureRegion region;
    region.x = rect.left;
    region.y = rect.top;
    region.width = rect.right - rect.left;
    region.height = rect.bottom - rect.top;

    return capture_region(region);
}

bool ScreenCapture::save_screenshot(const std::string& path, const CaptureRegion* region) {
    cv::Mat img = region ? capture_region(*region) : capture_full();
    if (img.empty()) {
        LOG_ERROR("Screenshot empty, cannot save to {}", path);
        return false;
    }
    bool ok = cv::imwrite(path, img);
    if (!ok) {
        LOG_ERROR("Failed to save screenshot to {}", path);
    }
    return ok;
}

cv::Mat ScreenCapture::hbitmap_to_mat(HBITMAP hbm, int width, int height) {
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    HDC hdc_mem = CreateCompatibleDC(nullptr);
    if (!hdc_mem) {
        LOG_ERROR("CreateCompatibleDC failed: {}", GetLastError());
        return {};
    }

    cv::Mat mat(height, width, CV_8UC4);
    BOOL ok = GetDIBits(hdc_mem, hbm, 0, height,
                        mat.data, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
    DeleteDC(hdc_mem);

    if (!ok) {
        LOG_ERROR("GetDIBits failed: {}", GetLastError());
        return {};
    }

    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);
    return bgr;
}
