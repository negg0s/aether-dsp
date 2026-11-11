#pragma once

#include "../types.hpp"
#include "poincare.hpp"
#include "runge_kutta.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <numbers>

namespace aether::math {

/**
 * @brief Acoustic Arrival Event recorded when a geodesic ray passes a listener.
 */
struct RayArrival {
    float time_sec{0.0f};
    float amplitude{0.0f};
    float azimuth_deg{0.0f};
    float distance_hyp{0.0f};
};

/**
 * @brief Single Geodesic Acoustic Ray state in the Poincare disk Riemannian manifold.
 */
struct GeodesicRay {
    Vec2 pos{0.0f, 0.0f};
    Vec2 vel{0.0f, 0.0f};
    float proper_time{0.0f};
    float total_distance{0.0f};
    float energy{1.0f};
    int bounces{0};
    bool active{true};
};

/**
 * @brief Hamiltonian Geodesic Sound Ray Tracer on Hyperbolic 2D Manifolds (Poincare Disk).
 *
 * Integrates the non-linear Christoffel geodesic equations:
 *   d^2 x^i / dt^2 + Gamma^i_jk (dx^j / dt)(dx^k / dt) = 0
 * using 4th-order Runge-Kutta symplectic integration to compute non-Euclidean
 * acoustic ray paths, wavefront propagation, and Manifold Impulse Responses (MIR).
 */
class GeodesicRayTracer {
public:
    struct Config {
        int num_rays{128};
        float step_dt{0.0008f};
        float max_time{1.5f};
        float boundary_radius{0.92f};
        float reflection_coeff{0.75f};
        float capture_radius{0.045f};
        float speed_of_sound{343.0f}; // m/s equivalent scale
    };

    GeodesicRayTracer() : GeodesicRayTracer(Config{}) {}
    explicit GeodesicRayTracer(Config cfg) : config_(cfg) {}

    /**
     * @brief Compute Geodesic Equations state derivative (velocity & acceleration).
     *
     * In Poincare metric g_ij = (2 / (1 - r^2))^2 delta_ij:
     *   ddot(x) = (2 / (1 - r^2)) * [ x * (ydot^2 - xdot^2) - 2 * y * xdot * ydot ]
     *   ddot(y) = (2 / (1 - r^2)) * [ y * (xdot^2 - ydot^2) - 2 * x * xdot * ydot ]
     */
    static void compute_geodesic_derivs(const Vec2& pos, const Vec2& vel, Vec2& dpos, Vec2& dvel) {
        dpos = vel;

        const float r2 = std::min(pos.length_squared(), 0.999f);
        const float denom = 1.0f - r2;
        const float factor = 2.0f / (denom > 1e-4f ? denom : 1e-4f);

        const float vx2 = vel.x * vel.x;
        const float vy2 = vel.y * vel.y;
        const float vxy = vel.x * vel.y;

        dvel.x = factor * (pos.x * (vy2 - vx2) - 2.0f * pos.y * vxy);
        dvel.y = factor * (pos.y * (vx2 - vy2) - 2.0f * pos.x * vxy);
    }

    /**
     * @brief Trace an ensemble of sound rays from a source to a receiver.
     */
    std::vector<RayArrival> trace(const Vec2& source, const Vec2& receiver) {
        config_.num_rays = std::clamp(config_.num_rays, 1, 1024);
        config_.step_dt = std::clamp(config_.step_dt, 0.00005f, 0.05f);
        config_.max_time = std::clamp(config_.max_time, 0.01f, 5.0f);
        config_.speed_of_sound = std::clamp(config_.speed_of_sound, 10.0f, 2000.0f);
        config_.boundary_radius = std::clamp(config_.boundary_radius, 0.1f, 0.98f);
        config_.capture_radius = std::clamp(config_.capture_radius, 0.005f, 0.5f);

        std::vector<RayArrival> arrivals;
        std::vector<GeodesicRay> rays(config_.num_rays);

        const float lambda_src = PoincareDisk::conformal_factor(source);
        const float v_coord = 1.0f / (lambda_src > 1e-3f ? lambda_src : 1.0f);

        for (int i = 0; i < config_.num_rays; ++i) {
            const float angle = (static_cast<float>(i) / config_.num_rays) * 2.0f * std::numbers::pi_v<float>;
            rays[i].pos = source;
            rays[i].vel = Vec2{std::cos(angle) * v_coord, std::sin(angle) * v_coord};
            rays[i].proper_time = 0.0f;
            rays[i].total_distance = 0.0f;
            rays[i].energy = 1.0f;
            rays[i].bounces = 0;
            rays[i].active = true;
        }

        const int max_steps = static_cast<int>(config_.max_time / config_.step_dt);

        for (int step = 0; step < max_steps; ++step) {
            bool any_active = false;

            for (auto& ray : rays) {
                if (!ray.active) continue;
                any_active = true;

                Vec2 k1_pos, k1_vel;
                compute_geodesic_derivs(ray.pos, ray.vel, k1_pos, k1_vel);

                Vec2 p2 = ray.pos + k1_pos * (config_.step_dt * 0.5f);
                Vec2 v2 = ray.vel + k1_vel * (config_.step_dt * 0.5f);
                Vec2 k2_pos, k2_vel;
                compute_geodesic_derivs(p2, v2, k2_pos, k2_vel);

                Vec2 p3 = ray.pos + k2_pos * (config_.step_dt * 0.5f);
                Vec2 v3 = ray.vel + k2_vel * (config_.step_dt * 0.5f);
                Vec2 k3_pos, k3_vel;
                compute_geodesic_derivs(p3, v3, k3_pos, k3_vel);

                Vec2 p4 = ray.pos + k3_pos * config_.step_dt;
                Vec2 v4 = ray.vel + k3_vel * config_.step_dt;
                Vec2 k4_pos, k4_vel;
                compute_geodesic_derivs(p4, v4, k4_pos, k4_vel);

                ray.pos = ray.pos + (k1_pos + k2_pos * 2.0f + k3_pos * 2.0f + k4_pos) * (config_.step_dt / 6.0f);
                ray.vel = ray.vel + (k1_vel + k2_vel * 2.0f + k3_vel * 2.0f + k4_vel) * (config_.step_dt / 6.0f);

                const float r = ray.pos.length();
                const float cf = PoincareDisk::conformal_factor(ray.pos);
                const float ds = cf * ray.vel.length() * config_.step_dt;
                ray.total_distance += ds;
                ray.proper_time += ds / config_.speed_of_sound;

                if (r >= config_.boundary_radius) {
                    if (ray.bounces >= 8 || ray.energy < 0.01f) {
                        ray.active = false;
                        continue;
                    }
                    const Vec2 normal = ray.pos.normalized();
                    const float v_dot_n = ray.vel.dot(normal);
                    if (v_dot_n > 0.0f) {
                        ray.vel = ray.vel - normal * (2.0f * v_dot_n);
                        ray.energy *= config_.reflection_coeff;
                        ray.bounces++;
                        ray.pos = normal * (config_.boundary_radius - 0.002f);
                    }
                }

                const float dist_to_rec = (ray.pos - receiver).length();
                if (dist_to_rec < config_.capture_radius) {
                    const float d_hyp = PoincareDisk::geodesic_distance(source, ray.pos);
                    const float attenuation = (1.0f / (1.0f + d_hyp)) * std::exp(-0.15f * ray.total_distance);
                    const float angle_to_rec = std::atan2(receiver.y - ray.pos.y, receiver.x - ray.pos.x) * (180.0f / std::numbers::pi_v<float>);

                    arrivals.push_back({
                        ray.proper_time,
                        ray.energy * attenuation * (1.0f - dist_to_rec / config_.capture_radius),
                        angle_to_rec,
                        d_hyp
                    });

                    // Slightly reduce energy upon partial wavefront capture
                    ray.energy *= 0.85f;
                }
            }

            if (!any_active) break;
        }

        // Sort arrivals chronologically
        std::sort(arrivals.begin(), arrivals.end(), [](const RayArrival& a, const RayArrival& b) {
            return a.time_sec < b.time_sec;
        });

        return arrivals;
    }

    /**
     * @brief Synthesize a Manifold Impulse Response (MIR) from ray arrival events.
     */
    std::vector<StereoFrame> synthesize_impulse_response(const std::vector<RayArrival>& arrivals,
                                                        uint32_t sample_rate,
                                                        float total_duration = 1.0f) const {
        const size_t total_samples = static_cast<size_t>(total_duration * sample_rate);
        std::vector<StereoFrame> ir(total_samples, StereoFrame{0.0f, 0.0f});

        for (const auto& arr : arrivals) {
            const size_t idx = static_cast<size_t>(arr.time_sec * sample_rate);
            if (idx >= total_samples) continue;

            // Spatial binaural distribution from arrival azimuth
            const float rad = arr.azimuth_deg * (std::numbers::pi_v<float> / 180.0f);
            const float pan_l = 0.5f * (1.0f - std::sin(rad));
            const float pan_r = 0.5f * (1.0f + std::sin(rad));

            // Fractional sample sinc/spline impulse spreading
            ir[idx].left += arr.amplitude * pan_l;
            ir[idx].right += arr.amplitude * pan_r;

            if (idx + 1 < total_samples) {
                ir[idx + 1].left += arr.amplitude * pan_l * 0.4f;
                ir[idx + 1].right += arr.amplitude * pan_r * 0.4f;
            }
        }

        // Normalize peak impulse response
        float max_peak = 1e-6f;
        for (const auto& f : ir) {
            max_peak = std::max({max_peak, std::abs(f.left), std::abs(f.right)});
        }
        if (max_peak > 0.0f) {
            const float gain = 0.85f / max_peak;
            for (auto& f : ir) {
                f.left *= gain;
                f.right *= gain;
            }
        }

        return ir;
    }

private:
    Config config_;
};

} // namespace aether::math
