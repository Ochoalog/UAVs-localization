#pragma once

namespace lambda_brief {

/**
 * Two-phase operation of the localization filter (see docs/analise_fim_do_voo_adm2.md).
 *
 * GLOBAL   - the filter is still searching: altimeter / compass priors, random particle
 *            injection and the "global" VO scale dead-band are active. They make the global
 *            convergence fast and seed-independent.
 * TRACKING - the cloud has converged: priors and injection are switched off and the VO scale
 *            dead-band returns to the legacy value. On ADM/0036 the priors and the injection
 *            are what make the filter lose track in the landing segment (H3/H4 of the
 *            analysis), while the legacy configuration tracks at ~15 m to the end.
 *
 * GLOBAL -> TRACKING after `converge_frames` consecutive frames with
 *   cloud spread sqrt(var_x + var_y) < converge_std_px  AND
 *   observation contrast C >= converge_contrast (when C is available and the threshold > 0).
 * The contrast condition guards against locking a converged FALSE mode: on ADM/0036 a cloud
 * tracking the true pose shows C ~ 1.3-2.2, a false mode C ~ 0.8-1.2 (single-seed evidence,
 * tune per dataset).
 *
 * TRACKING -> GLOBAL (optional, relocalize_std_px > 0) after `relocalize_frames` consecutive
 * frames with spread > relocalize_std_px.
 */
struct PhaseConfig {
    bool enabled{false};
    double converge_std_px{100.0};
    int converge_frames{5};
    double converge_contrast{1.3};
    double relocalize_std_px{0.0};
    int relocalize_frames{5};
};

enum class FilterPhase { GLOBAL, TRACKING };

class PhaseController {
public:
    explicit PhaseController(const PhaseConfig& config = PhaseConfig{}) : config_(config) {}

    FilterPhase phase() const { return phase_; }
    bool tracking() const { return phase_ == FilterPhase::TRACKING; }
    int streak() const { return streak_; }

    /**
     * @brief Feed the state of the frame that was just processed.
     * @return true when the phase changed (the new phase applies to the NEXT frame).
     */
    bool update(double spread_px, bool contrast_valid, double contrast) {
        if (!config_.enabled) return false;
        if (phase_ == FilterPhase::GLOBAL) {
            const bool contrast_ok = config_.converge_contrast <= 0.0 || !contrast_valid ||
                                     contrast >= config_.converge_contrast;
            const bool ok = spread_px < config_.converge_std_px && contrast_ok;
            streak_ = ok ? streak_ + 1 : 0;
            if (streak_ >= config_.converge_frames) {
                phase_ = FilterPhase::TRACKING;
                streak_ = 0;
                return true;
            }
        } else {
            const bool lost = config_.relocalize_std_px > 0.0 && spread_px > config_.relocalize_std_px;
            streak_ = lost ? streak_ + 1 : 0;
            if (lost && streak_ >= config_.relocalize_frames) {
                phase_ = FilterPhase::GLOBAL;
                streak_ = 0;
                return true;
            }
        }
        return false;
    }

private:
    PhaseConfig config_;
    FilterPhase phase_{FilterPhase::GLOBAL};
    int streak_{0};
};

} // namespace lambda_brief
