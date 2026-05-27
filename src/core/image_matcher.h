#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

/// Result of a single template-match.
struct MatchResult {
    cv::Point location;        ///< Top-left corner of the match.
    double    confidence;      ///< Normalized correlation coefficient [0, 1].
    int       template_width;  ///< Width of the template at the matched scale.
    int       template_height; ///< Height of the template at the matched scale.
};

/// Configuration for image-template matching.
struct MatchConfig {
    double  threshold          = 0.8;    ///< Minimum confidence to keep a match.
    bool    enable_multiscale  = true;   ///< Search across a range of scales.
    double  scale_min          = 0.8;    ///< Smallest scale factor.
    double  scale_max          = 1.2;    ///< Largest scale factor.
    double  scale_step         = 0.1;    ///< Step between successive scales.
    bool    enable_multi_target = true;  ///< Return multiple non-overlapping matches.
    int     max_targets        = 5;      ///< Maximum matches to return.
    double  min_distance       = 20.0;   ///< Minimum pixel distance between matches.
};

/// Template-based image matching (cv::matchTemplate).
class ImageMatcher {
public:
    ImageMatcher() = delete;

    /// Find all matches of @p templ inside @p source that meet the threshold.
    [[nodiscard]] static std::vector<MatchResult>
    find_template(const cv::Mat& source, const cv::Mat& templ,
                  const MatchConfig& config = {});

    /// Convenience: capture the full screen then find matches.
    [[nodiscard]] static std::vector<MatchResult>
    find_template_on_screen(const cv::Mat& templ,
                            const MatchConfig& config = {});

    /// Return the single best match (highest confidence).
    [[nodiscard]] static MatchResult
    find_best(const cv::Mat& source, const cv::Mat& templ,
              const MatchConfig& config = {});

private:
    static void non_max_suppression(std::vector<MatchResult>& results,
                                    double min_distance);
};
