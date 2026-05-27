#include "image_matcher.h"
#include "screen_capture.h"
#include "utils/logger.h"
#include <cmath>
#include <algorithm>

std::vector<MatchResult> ImageMatcher::find_template(
    const cv::Mat& source,
    const cv::Mat& templ,
    const MatchConfig& config) {

    std::vector<MatchResult> all_results;

    if (config.enable_multiscale) {
        for (double scale = config.scale_min; scale <= config.scale_max; scale += config.scale_step) {
            cv::Mat scaled;
            int new_w = static_cast<int>(templ.cols * scale);
            int new_h = static_cast<int>(templ.rows * scale);
            if (new_w <= 0 || new_h <= 0 || new_w > source.cols || new_h > source.rows)
                continue;
            cv::resize(templ, scaled, cv::Size(new_w, new_h));

            cv::Mat result;
            cv::matchTemplate(source, scaled, result, cv::TM_CCOEFF_NORMED);

            for (int y = 0; y < result.rows; ++y) {
                for (int x = 0; x < result.cols; ++x) {
                    float conf = result.at<float>(y, x);
                    if (conf >= config.threshold) {
                        all_results.push_back({
                            cv::Point(x, y),
                            static_cast<double>(conf),
                            new_w,
                            new_h
                        });
                    }
                }
            }
        }
    } else {
        cv::Mat result;
        cv::matchTemplate(source, templ, result, cv::TM_CCOEFF_NORMED);

        for (int y = 0; y < result.rows; ++y) {
            for (int x = 0; x < result.cols; ++x) {
                float conf = result.at<float>(y, x);
                if (conf >= config.threshold) {
                    all_results.push_back({
                        cv::Point(x, y),
                        static_cast<double>(conf),
                        templ.cols,
                        templ.rows
                    });
                }
            }
        }
    }

    non_max_suppression(all_results, config.min_distance);

    if (config.enable_multi_target && static_cast<int>(all_results.size()) > config.max_targets) {
        std::partial_sort(all_results.begin(), all_results.begin() + config.max_targets,
                          all_results.end(),
                          [](const MatchResult& a, const MatchResult& b) {
                              return a.confidence > b.confidence;
                          });
        all_results.resize(config.max_targets);
    } else if (!config.enable_multi_target && !all_results.empty()) {
        auto best = std::max_element(all_results.begin(), all_results.end(),
                                     [](const MatchResult& a, const MatchResult& b) {
                                         return a.confidence < b.confidence;
                                     });
        if (best != all_results.end()) {
            MatchResult tmp = *best;
            all_results.clear();
            all_results.push_back(tmp);
        }
    }

    return all_results;
}

std::vector<MatchResult> ImageMatcher::find_template_on_screen(
    const cv::Mat& templ,
    const MatchConfig& config) {

    cv::Mat screen = ScreenCapture::capture_full();
    if (screen.empty()) {
        LOG_ERROR("Screen capture failed for image matching");
        return {};
    }
    return find_template(screen, templ, config);
}

MatchResult ImageMatcher::find_best(
    const cv::Mat& source,
    const cv::Mat& templ,
    const MatchConfig& config) {

    auto results = find_template(source, templ, config);
    if (results.empty()) {
        return {{0, 0}, 0.0, 0, 0};
    }

    auto best = std::max_element(results.begin(), results.end(),
                                  [](const MatchResult& a, const MatchResult& b) {
                                      return a.confidence < b.confidence;
                                  });
    return *best;
}

void ImageMatcher::non_max_suppression(std::vector<MatchResult>& results,
                                        double min_distance) {
    if (results.empty()) return;

    std::sort(results.begin(), results.end(),
              [](const MatchResult& a, const MatchResult& b) {
                  return a.confidence > b.confidence;
              });

    std::vector<MatchResult> filtered;
    filtered.push_back(results[0]);

    for (size_t i = 1; i < results.size(); ++i) {
        bool keep = true;
        for (const auto& kept : filtered) {
            double dx = results[i].location.x - kept.location.x;
            double dy = results[i].location.y - kept.location.y;
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < min_distance) {
                keep = false;
                break;
            }
        }
        if (keep) {
            filtered.push_back(results[i]);
        }
    }

    results = std::move(filtered);
}
