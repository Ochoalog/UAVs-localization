#!/usr/bin/env python3
"""
High-Resolution Video Generator for lambda-BRIEF Particle Convergence
Visualizes satellite map, particle swarm elimination, Ground Truth, and Estimated UAV trajectory.
"""

import sys
import os
import argparse
import math
import numpy as np
import cv2

def parse_args():
    parser = argparse.ArgumentParser(description="Render particle elimination video for lambda-BRIEF")
    parser.add_argument("--results_dir", "-r", type=str, required=True,
                        help="Path to results directory (containing errors.txt and particles_log.csv)")
    parser.add_argument("--map", "-m", type=str, required=True,
                        help="Path to satellite map image")
    parser.add_argument("--output", "-o", type=str, default="localization_video.mp4",
                        help="Output video file path (default: localization_video.mp4)")
    parser.add_argument("--fps", type=float, default=6.0,
                        help="Frames per second for video (default: 6.0)")
    parser.add_argument("--video_width", type=int, default=1280,
                        help="Output video width (default: 1280)")
    parser.add_argument("--video_height", type=int, default=720,
                        help="Output video height (default: 720)")
    parser.add_argument("--zoom_margin", type=int, default=300,
                        help="Margin in pixels around drone trajectory for dynamic ROI cropping (0 for full map)")
    return parser.parse_args()

def load_errors(errors_path):
    frames, est_x, est_y, est_z = [], [], [], []
    gt_x, gt_y, gt_z = [], [], []
    pos_err, yaw_err = [], []

    if not os.path.exists(errors_path):
        return None

    with open(errors_path, "r") as f:
        header = f.readline()
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 9:
                frames.append(int(parts[0]))
                est_x.append(float(parts[1]))
                est_y.append(float(parts[2]))
                est_z.append(float(parts[3]))
                gt_x.append(float(parts[4]))
                gt_y.append(float(parts[5]))
                gt_z.append(float(parts[6]))
                pos_err.append(float(parts[7]))
                yaw_err.append(float(parts[8]))

    return {
        "frames": frames,
        "est_x": est_x, "est_y": est_y, "est_z": est_z,
        "gt_x": gt_x, "gt_y": gt_y, "gt_z": gt_z,
        "pos_err": pos_err, "yaw_err": yaw_err
    }

def load_particles(particles_path):
    particles_by_frame = {}
    if not os.path.exists(particles_path):
        return particles_by_frame

    with open(particles_path, "r") as f:
        header = f.readline()
        for line in f:
            parts = line.strip().split(",")
            if len(parts) >= 7:
                frame_idx = int(parts[0])
                # x, y, z, yaw_deg, weight
                x = float(parts[2])
                y = float(parts[3])
                z = float(parts[4])
                yaw = float(parts[5])
                weight = float(parts[6])

                if frame_idx not in particles_by_frame:
                    particles_by_frame[frame_idx] = []
                particles_by_frame[frame_idx].append((x, y, z, yaw, weight))

    return particles_by_frame

def draw_hud(img, frame_idx, total_frames, pos_err_m, yaw_err_deg, est_pose, gt_pose):
    """Draws a semi-transparent HUD overlay on top-left corner"""
    hud_w, hud_h = 420, 160
    overlay = img.copy()
    cv2.rectangle(overlay, (15, 15), (15 + hud_w, 15 + hud_h), (20, 20, 20), -1)
    cv2.addWeighted(overlay, 0.75, img, 0.25, 0, img)
    cv2.rectangle(img, (15, 15), (15 + hud_w, 15 + hud_h), (100, 100, 100), 1)

    font = cv2.FONT_HERSHEY_SIMPLEX
    color_white = (255, 255, 255)
    color_green = (80, 255, 80)
    color_cyan = (255, 255, 80)
    color_yellow = (80, 230, 255)

    cv2.putText(img, f"lambda-BRIEF Particle Filter", (30, 42), font, 0.65, color_cyan, 2)
    cv2.putText(img, f"Frame: [{frame_idx + 1:03d} / {total_frames:03d}]", (30, 68), font, 0.5, color_white, 1)
    cv2.putText(img, f"Ground Truth: ({gt_pose[0]:.1f}, {gt_pose[1]:.1f}) px", (30, 90), font, 0.45, color_green, 1)
    cv2.putText(img, f"Estimated:    ({est_pose[0]:.1f}, {est_pose[1]:.1f}) px", (30, 110), font, 0.45, color_yellow, 1)
    cv2.putText(img, f"Pos Error: {pos_err_m:.2f} m  |  Yaw Error: {yaw_err_deg:.1f} deg", (30, 138), font, 0.55, (50, 120, 255), 2)

def draw_legend(img, x_pos, y_pos):
    """Draws color legend for markers"""
    overlay = img.copy()
    cv2.rectangle(overlay, (x_pos, y_pos), (x_pos + 260, y_pos + 100), (20, 20, 20), -1)
    cv2.addWeighted(overlay, 0.75, img, 0.25, 0, img)
    cv2.rectangle(img, (x_pos, y_pos), (x_pos + 260, y_pos + 100), (100, 100, 100), 1)

    font = cv2.FONT_HERSHEY_SIMPLEX
    # Ground truth
    cv2.circle(img, (x_pos + 20, y_pos + 25), 6, (0, 255, 0), -1)
    cv2.putText(img, "Ground Truth (GPS/IMU)", (x_pos + 38, y_pos + 30), font, 0.45, (255, 255, 255), 1)

    # Estimated
    cv2.circle(img, (x_pos + 20, y_pos + 50), 6, (0, 255, 255), -1)
    cv2.putText(img, "Estimated Pose (MCL)", (x_pos + 38, y_pos + 55), font, 0.45, (255, 255, 255), 1)

    # Particle cloud
    cv2.circle(img, (x_pos + 20, y_pos + 75), 4, (0, 140, 255), -1)
    cv2.putText(img, "Particles (Swarm)", (x_pos + 38, y_pos + 80), font, 0.45, (255, 255, 255), 1)

def main():
    args = parse_args()
    errors_path = os.path.join(args.results_dir, "errors.txt")
    particles_path = os.path.join(args.results_dir, "particles_log.csv")

    err_data = load_errors(errors_path)
    if not err_data or len(err_data["frames"]) == 0:
        print(f"Error: Could not load errors from {errors_path}")
        sys.exit(1)

    particles_data = load_particles(particles_path)
    print(f"[Video] Loaded {len(err_data['frames'])} error frames and {len(particles_data)} particle frames.")

    print(f"[Video] Loading satellite map: {args.map} ...")
    map_img = cv2.imread(args.map)
    if map_img is None:
        print(f"Error: Could not read satellite map from {args.map}")
        sys.exit(1)

    map_h, map_w = map_img.shape[:2]
    total_frames = len(err_data["frames"])

    # Determine viewport (Crop ROI or Full Map)
    all_x = err_data["gt_x"] + err_data["est_x"]
    all_y = err_data["gt_y"] + err_data["est_y"]
    min_x, max_x = min(all_x), max(all_x)
    min_y, max_y = min(all_y), max(all_y)

    crop_roi = False
    if args.zoom_margin > 0:
        crop_roi = True
        roi_x1 = max(0, int(min_x - args.zoom_margin))
        roi_y1 = max(0, int(min_y - args.zoom_margin))
        roi_x2 = min(map_w, int(max_x + args.zoom_margin))
        roi_y2 = min(map_h, int(max_y + args.zoom_margin))

        # Enforce aspect ratio
        roi_w = roi_x2 - roi_x1
        roi_h = roi_y2 - roi_y1
        target_aspect = args.video_width / args.video_height
        curr_aspect = roi_w / roi_h

        if curr_aspect > target_aspect:
            # Expand height
            new_h = int(roi_w / target_aspect)
            diff = new_h - roi_h
            roi_y1 = max(0, roi_y1 - diff // 2)
            roi_y2 = min(map_h, roi_y2 + diff // 2)
        else:
            # Expand width
            new_w = int(roi_h * target_aspect)
            diff = new_w - roi_w
            roi_x1 = max(0, roi_x1 - diff // 2)
            roi_x2 = min(map_w, roi_x2 + diff // 2)

        base_map = map_img[roi_y1:roi_y2, roi_x1:roi_x2]
        x_offset, y_offset = roi_x1, roi_y1
    else:
        base_map = map_img
        x_offset, y_offset = 0, 0

    target_w, target_h = args.video_width, args.video_height
    scale_x = target_w / base_map.shape[1]
    scale_y = target_h / base_map.shape[0]

    # Setup OpenCV Video Writer
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out_video = cv2.VideoWriter(args.output, fourcc, args.fps, (target_w, target_h))

    print(f"[Video] Rendering {total_frames} frames to {args.output} ({target_w}x{target_h} @ {args.fps} FPS)...")

    gt_trail = []
    est_trail = []

    for f_idx in range(total_frames):
        frame_num = err_data["frames"][f_idx]
        frame_canvas = cv2.resize(base_map, (target_w, target_h), interpolation=cv2.INTER_AREA)

        # 1. Draw Particles (if available for this frame)
        if frame_num in particles_data:
            p_list = particles_data[frame_num]
            max_w = max([p[4] for p in p_list]) if p_list else 1.0
            min_w = min([p[4] for p in p_list]) if p_list else 0.0

            overlay = frame_canvas.copy()
            for (px, py, pz, pyaw, pw) in p_list:
                # Convert to canvas coordinates
                cx = int((px - x_offset) * scale_x)
                cy = int((py - y_offset) * scale_y)

                if 0 <= cx < target_w and 0 <= cy < target_h:
                    norm_w = (pw - min_w) / (max_w - min_w + 1e-9)
                    # Color interpolation: low weight = orange/red (0, 140, 255), high weight = bright yellow/green (0, 255, 200)
                    r_val = int(255 * (1.0 - norm_w * 0.4))
                    g_val = int(140 + 115 * norm_w)
                    b_val = int(30 * (1.0 - norm_w))
                    color = (b_val, g_val, r_val)

                    radius = 3 if norm_w < 0.7 else 5
                    cv2.circle(overlay, (cx, cy), radius, color, -1)

                    # Draw orientation tick for strong particles
                    if norm_w > 0.5:
                        rad = math.radians(pyaw)
                        arrow_len = 10
                        tx = int(cx + arrow_len * math.cos(rad))
                        ty = int(cy + arrow_len * math.sin(rad))
                        cv2.line(overlay, (cx, cy), (tx, ty), (255, 255, 255), 1)

            cv2.addWeighted(overlay, 0.65, frame_canvas, 0.35, 0, frame_canvas)

        # 2. Draw Trajectory Trails
        curr_gt_x = int((err_data["gt_x"][f_idx] - x_offset) * scale_x)
        curr_gt_y = int((err_data["gt_y"][f_idx] - y_offset) * scale_y)
        curr_est_x = int((err_data["est_x"][f_idx] - x_offset) * scale_x)
        curr_est_y = int((err_data["est_y"][f_idx] - y_offset) * scale_y)

        gt_trail.append((curr_gt_x, curr_gt_y))
        est_trail.append((curr_est_x, curr_est_y))

        # Ground Truth trail (green)
        for i in range(1, len(gt_trail)):
            cv2.line(frame_canvas, gt_trail[i-1], gt_trail[i], (0, 220, 0), 2, cv2.LINE_AA)
        # Estimated trail (yellow/cyan)
        for i in range(1, len(est_trail)):
            cv2.line(frame_canvas, est_trail[i-1], est_trail[i], (0, 230, 255), 2, cv2.LINE_AA)

        # 3. Draw Current Markers
        # Ground Truth: Green target with ring
        cv2.circle(frame_canvas, (curr_gt_x, curr_gt_y), 10, (0, 255, 0), 2, cv2.LINE_AA)
        cv2.circle(frame_canvas, (curr_gt_x, curr_gt_y), 4, (0, 255, 0), -1, cv2.LINE_AA)

        # Estimated: Cyan/Yellow target with crosshair
        cv2.circle(frame_canvas, (curr_est_x, curr_est_y), 10, (0, 255, 255), 2, cv2.LINE_AA)
        cv2.circle(frame_canvas, (curr_est_x, curr_est_y), 4, (0, 255, 255), -1, cv2.LINE_AA)
        cv2.drawMarker(frame_canvas, (curr_est_x, curr_est_y), (0, 255, 255), cv2.MARKER_CROSS, 16, 2)

        # 4. Error line connecting estimated and ground truth
        cv2.line(frame_canvas, (curr_gt_x, curr_gt_y), (curr_est_x, curr_est_y), (0, 0, 255), 1, cv2.LINE_AA)

        # 5. Draw HUD and Legend
        draw_hud(
            frame_canvas,
            f_idx,
            total_frames,
            err_data["pos_err"][f_idx],
            err_data["yaw_err"][f_idx],
            (err_data["est_x"][f_idx], err_data["est_y"][f_idx]),
            (err_data["gt_x"][f_idx], err_data["gt_y"][f_idx])
        )

        draw_legend(frame_canvas, target_w - 280, 15)

        out_video.write(frame_canvas)

    out_video.release()
    print(f"\n[Success] Video generated successfully: {args.output}")

if __name__ == "__main__":
    main()
