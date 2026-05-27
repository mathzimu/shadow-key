#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct MatchResult {
    cv::Point location;
    double confidence;
    int template_width;
    int template_height;
};

struct MatchConfig {
    double threshold = 0.8;
    bool enable_multiscale = true;
    double scale_min = 0.8;
    double scale_max = 1.2;
    double scale_step = 0.1;
    bool enable_multi_target = true;
    int max_targets = 5;
    double min_distance = 20.0;
};

class ImageMatcher {
public:
    static std::vector<MatchResult> find_template(
        const cv::Mat& source,
        const cv::Mat& templ,
        const MatchConfig& config = {});

    static std::vector<MatchResult> find_template_on_screen(
        const cv::Mat& templ,
        const MatchConfig& config = {});

    static MatchResult find_best(
        const cv::Mat& source,
        const cv::Mat& templ,
        const MatchConfig& config = {});

private:
    static void non_max_suppression(std::vector<MatchResult>& results,
                                     double min_distance);
};
