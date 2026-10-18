#!/usr/bin/env python3
"""
High-Resolution Video Generator for lambda-BRIEF Particle Convergence and Visual Odometry.
Visualizes satellite map, particle swarm elimination, Ground Truth, Estimated UAV trajectory,
and Pure Visual Odometry (Dead-Reckoning).

Supports layout modes:
  - 'map': Satellite map with particles, GT, MCL estimate, and Odometry trails.
  - 'camera': Onboard UAV RGB camera with cockpit HUD telemetry, compass, optical flow, and PiP mini-map.
  - 'side_by_side': Split-screen showing Onboard Camera (left) and Satellite Map (right).
"""

import sys
import os
import argparse
import math
import glob
import numpy as np
import cv2

def parse_args():
    parser = argparse.ArgumentParser(description="Render particle elimination & odometry video for lambda-BRIEF")
    parser.add_argument("--results_dir", "-r", type=str, required=True,
                        help="Path to results directory (containing errors.txt and optionally particles_log.csv)")
    parser.add_argument("--map", "-m", type=str, default="",
                        help="Path to satellite map image (required for 'map' and 'side_by_side' layouts)")
    parser.add_argument("--dataset_dir", "-d", type=str, default="",
                        help="Path to dataset directory (e.g. Datasets/ADM/0036), used for images and odom")
    parser.add_argument("--images_dir", type=str, default="",
                        help="Direct path to camera images directory (e.g. Images_RGB)")
    parser.add_argument("--odom_file", type=str, default="",
                        help="Path to odometry file (dx dy dz dyaw reliable per line). If empty, auto-detects.")
    parser.add_argument("--output", "-o", type=str, default="localization_video.mp4",
                        help="Output video file path (default: localization_video.mp4)")
    parser.add_argument("--fps", type=float, default=6.0,
                        help="Frames per second for video (default: 6.0)")
    parser.add_argument("--video_width", type=int, default=1280,
                        help="Output video width (default: 1280 for map/camera, 1920 for side_by_side)")
    parser.add_argument("--video_height", type=int, default=720,
                        help="Output video height (default: 720 for map/camera, 1080 for side_by_side)")
    parser.add_argument("--zoom_margin", type=int, default=300,
                        help="Margin in pixels around trajectories for dynamic ROI cropping (0 for full map)")
    parser.add_argument("--pixel_to_meter", type=float, default=0.188679245,
                        help="Meters per pixel (default: 0.188679245 for ADM, 0.10 for UFRGS)")
    parser.add_argument("--layout", type=str, choices=["map", "camera", "side_by_side"], default="map",
                        help="Video layout: 'map' (default), 'camera', or 'side_by_side'")
    parser.add_argument("--show_odometry", action="store_true", default=True,
                        help="Show pure visual odometry trajectory (default: True)")
    parser.add_argument("--no_odometry", action="store_false", dest="show_odometry",
                        help="Disable odometry trajectory")
    parser.add_argument("--show_flow", action="store_true", default=True,
                        help="Show optical flow / feature vectors on camera view (default: True)")
    parser.add_argument("--start_frame", type=int, default=-1,
                        help="Start frame index in traj.txt (auto-detected if negative)")
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
                x = float(parts[2])
                y = float(parts[3])
                z = float(parts[4])
                yaw = float(parts[5])
                weight = float(parts[6])

                if frame_idx not in particles_by_frame:
                    particles_by_frame[frame_idx] = []
                particles_by_frame[frame_idx].append((x, y, z, yaw, weight))

    return particles_by_frame

def auto_find_file(filename_patterns, search_dirs):
    for d in search_dirs:
        if not d or not os.path.exists(d):
            continue
        for pattern in filename_patterns:
            matches = glob.glob(os.path.join(d, pattern))
            if matches:
                return sorted(matches)[0]
            matches_sub = glob.glob(os.path.join(d, "**", pattern), recursive=True)
            if matches_sub:
                return sorted(matches_sub)[0]
    return None

def load_odometry(odom_path):
    if not odom_path or not os.path.exists(odom_path):
        return []
    records = []
    with open(odom_path, "r") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 5:
                dx = float(parts[0])
                dy = float(parts[1])
                dz = float(parts[2])
                dyaw = float(parts[3])
                rel = int(float(parts[4]))
                records.append((dx, dy, dz, dyaw, rel))
    return records

def load_initial_heading(dataset_dir, results_dir):
    """Attempts to find the initial yaw (in radians) from traj_truth.txt"""
    truth_file = auto_find_file(["traj_truth.txt", "GT2.txt"], [dataset_dir, results_dir])
    if truth_file and os.path.exists(truth_file):
        try:
            with open(truth_file, "r") as f:
                first_line = f.readline().strip()
                parts = first_line.split()
                if len(parts) >= 4:
                    yaw_deg = float(parts[3])
                    return math.radians(yaw_deg)
        except Exception:
            pass
    return 0.0

def integrate_odometry(init_pose, odom_records, num_frames, pixel_to_meter=0.188679245):
    """
    Integrates relative odometry (dx, dy, dz, dyaw) starting from init_pose (x, y, z, yaw_rad).
    Returns:
      odom_poses: list of (x, y, z, yaw_rad)
      odom_steps: list of (dx, dy, dz, dyaw, rel)
      odom_cum_dist: list of accumulated travel distance (meters)
    """
    x, y, z, yaw = init_pose
    poses = [(x, y, z, yaw)]
    steps = [(0.0, 0.0, 1.0, 0.0, 1)]
    cum_dist = [0.0]

    tot_dist = 0.0
    for i in range(num_frames - 1):
        if i < len(odom_records):
            dx, dy, dz, dyaw, rel = odom_records[i]
            steps.append((dx, dy, dz, dyaw, rel))
            if rel != 0:
                nx = dx * z
                ny = dy * z
                step_m = math.hypot(nx, ny) * pixel_to_meter
                tot_dist += step_m

                x += math.cos(yaw) * nx - math.sin(yaw) * ny
                y += math.sin(yaw) * nx + math.cos(yaw) * ny
                z *= dz
                yaw += dyaw
                yaw = (yaw + math.pi) % (2.0 * math.pi) - math.pi
        else:
            steps.append((0.0, 0.0, 1.0, 0.0, 1))

        poses.append((x, y, z, yaw))
        cum_dist.append(tot_dist)

    return poses, steps, cum_dist

def load_camera_frames(dataset_dir, images_dir, results_dir, total_frames, start_frame_arg):
    """Loads image paths for camera frames matching the slice"""
    resolved_paths = []

    cand_dirs = []
    if images_dir and os.path.exists(images_dir):
        cand_dirs.append(images_dir)
    if dataset_dir:
        cand_dirs.append(os.path.join(dataset_dir, "Images_RGB"))
        cand_dirs.append(dataset_dir)

    traj_file = auto_find_file(["traj.txt"], [dataset_dir, results_dir])
    if traj_file and os.path.exists(traj_file):
        with open(traj_file, "r") as f:
            lines = [l.strip() for l in f if l.strip()]

        offset = start_frame_arg
        if offset < 0:
            s = (dataset_dir + " " + results_dir).lower()
            if "0036" in s:
                offset = 84
            elif "0035" in s:
                offset = 18
            elif "ufrgs" in s:
                offset = 204
            else:
                offset = 0

        slice_lines = lines[offset : offset + total_frames]
        for l in slice_lines:
            bname = os.path.basename(l)
            found = None
            for d in cand_dirs:
                cand = os.path.join(d, bname)
                if os.path.exists(cand):
                    found = cand
                    break
            resolved_paths.append(found if found else l)

    if not resolved_paths or any(p is None or not os.path.exists(p) for p in resolved_paths):
        for d in cand_dirs:
            imgs = sorted(glob.glob(os.path.join(d, "*.jpg")) + glob.glob(os.path.join(d, "*.png")) + glob.glob(os.path.join(d, "*.JPG")))
            if len(imgs) >= total_frames:
                offset = start_frame_arg if start_frame_arg >= 0 else 0
                resolved_paths = imgs[offset : offset + total_frames]
                break

    return resolved_paths

def draw_compass(img, center, radius, yaw_rad, label="HEADING"):
    """Draws an orientation compass dial on the canvas"""
    cx, cy = center
    cv2.circle(img, center, radius, (20, 20, 20), -1)
    cv2.circle(img, center, radius, (80, 80, 80), 2, cv2.LINE_AA)
    cv2.circle(img, center, 3, (255, 255, 255), -1)

    font = cv2.FONT_HERSHEY_SIMPLEX
    cv2.putText(img, "N", (cx - 5, cy - radius + 15), font, 0.4, (80, 80, 255), 1, cv2.LINE_AA)
    cv2.putText(img, "S", (cx - 4, cy + radius - 6), font, 0.35, (160, 160, 160), 1, cv2.LINE_AA)
    cv2.putText(img, "E", (cx + radius - 15, cy + 5), font, 0.35, (160, 160, 160), 1, cv2.LINE_AA)
    cv2.putText(img, "W", (cx - radius + 6, cy + 5), font, 0.35, (160, 160, 160), 1, cv2.LINE_AA)

    angle_screen = yaw_rad - math.pi / 2.0
    tip_x = int(cx + (radius - 12) * math.cos(angle_screen))
    tip_y = int(cy + (radius - 12) * math.sin(angle_screen))
    back_x = int(cx - (radius - 22) * math.cos(angle_screen))
    back_y = int(cy - (radius - 22) * math.sin(angle_screen))

    cv2.line(img, center, (tip_x, tip_y), (0, 230, 255), 2, cv2.LINE_AA)
    cv2.circle(img, (tip_x, tip_y), 4, (0, 230, 255), -1, cv2.LINE_AA)
    cv2.line(img, center, (back_x, back_y), (80, 80, 80), 1, cv2.LINE_AA)

    deg = math.degrees(yaw_rad) % 360
    cv2.putText(img, f"{deg:.1f} deg", (cx - 24, cy + radius + 16), font, 0.4, (255, 255, 255), 1, cv2.LINE_AA)

def draw_minimap(img, odom_trail, gt_trail, curr_idx, pos=(1000, 500), size=(260, 200)):
    """Draws a mini-map inset on the canvas showing accumulated trajectory"""
    mx, my = pos
    mw, mh = size
    h, w = img.shape[:2]

    if mx + mw > w or my + mh > h:
        mx = max(10, w - mw - 20)
        my = max(10, h - mh - 20)

    overlay = img.copy()
    cv2.rectangle(overlay, (mx, my), (mx + mw, my + mh), (15, 15, 15), -1)
    cv2.addWeighted(overlay, 0.85, img, 0.15, 0, img)
    cv2.rectangle(img, (mx, my), (mx + mw, my + mh), (80, 80, 80), 1)

    font = cv2.FONT_HERSHEY_SIMPLEX
    cv2.putText(img, "TRAJECTORY MINI-MAP", (mx + 10, my + 18), font, 0.4, (200, 200, 200), 1, cv2.LINE_AA)

    all_pts = gt_trail + odom_trail
    if not all_pts:
        return

    all_x = [p[0] for p in all_pts]
    all_y = [p[1] for p in all_pts]
    min_x, max_x = min(all_x), max(all_x)
    min_y, max_y = min(all_y), max(all_y)

    span_x = max(1.0, max_x - min_x)
    span_y = max(1.0, max_y - min_y)

    pad = 20
    draw_w = mw - 2 * pad
    draw_h = mh - 2 * pad - 15

    scale = min(draw_w / span_x, draw_h / span_y)

    def to_mini(pt):
        px = int(mx + pad + (pt[0] - min_x) * scale)
        py = int(my + pad + 20 + (pt[1] - min_y) * scale)
        return (px, py)

    cv2.line(img, (mx + pad, my + mh // 2), (mx + mw - pad, my + mh // 2), (35, 35, 35), 1)
    cv2.line(img, (mx + mw // 2, my + pad + 15), (mx + mw // 2, my + mh - pad), (35, 35, 35), 1)

    for i in range(1, curr_idx + 1):
        if i < len(gt_trail):
            cv2.line(img, to_mini(gt_trail[i-1]), to_mini(gt_trail[i]), (0, 200, 0), 2, cv2.LINE_AA)

    for i in range(1, curr_idx + 1):
        if i < len(odom_trail):
            cv2.line(img, to_mini(odom_trail[i-1]), to_mini(odom_trail[i]), (220, 50, 220), 2, cv2.LINE_AA)

    if curr_idx < len(gt_trail):
        gt_pt = to_mini(gt_trail[curr_idx])
        cv2.circle(img, gt_pt, 4, (0, 255, 0), -1, cv2.LINE_AA)

    if curr_idx < len(odom_trail):
        od_pt = to_mini(odom_trail[curr_idx])
        cv2.circle(img, od_pt, 4, (220, 50, 220), -1, cv2.LINE_AA)

    cv2.putText(img, "GT", (mx + mw - 70, my + 18), font, 0.35, (0, 255, 0), 1, cv2.LINE_AA)
    cv2.putText(img, "ODOM", (mx + mw - 45, my + 18), font, 0.35, (220, 50, 220), 1, cv2.LINE_AA)

def draw_camera_hud(img, frame_idx, total_frames, odom_step, odom_drift_m, odom_dist_m, yaw_deg, pos_err_m):
    """Draws cockpit style telemetry HUD on top-left corner of camera frame"""
    hud_w, hud_h = 460, 160
    overlay = img.copy()
    cv2.rectangle(overlay, (15, 15), (15 + hud_w, 15 + hud_h), (15, 15, 15), -1)
    cv2.addWeighted(overlay, 0.80, img, 0.20, 0, img)
    cv2.rectangle(img, (15, 15), (15 + hud_w, 15 + hud_h), (80, 80, 80), 1)

    font = cv2.FONT_HERSHEY_SIMPLEX
    dx, dy, dz, dyaw, rel = odom_step
    dyaw_deg = math.degrees(dyaw)

    rel_str = "RELIABLE" if rel != 0 else "UNRELIABLE (ZEROED)"
    rel_color = (80, 255, 80) if rel != 0 else (50, 50, 255)

    cv2.putText(img, "UAV ONBOARD CAMERA + VISUAL ODOMETRY", (30, 42), font, 0.60, (255, 255, 80), 2, cv2.LINE_AA)
    cv2.putText(img, f"Frame: [{frame_idx + 1:03d} / {total_frames:03d}]  |  VO Status: {rel_str}",
                (30, 68), font, 0.45, rel_color, 1, cv2.LINE_AA)
    cv2.putText(img, f"Step Motion:  dx: {dx:+6.1f} px | dy: {dy:+6.1f} px | dz: {dz:.3f}",
                (30, 92), font, 0.45, (230, 230, 230), 1, cv2.LINE_AA)
    cv2.putText(img, f"Step Heading: dyaw: {dyaw_deg:+5.2f} deg  |  Heading: {yaw_deg:5.1f} deg",
                (30, 114), font, 0.45, (230, 230, 230), 1, cv2.LINE_AA)
    cv2.putText(img, f"Odom Traveled: {odom_dist_m:6.1f} m  |  Odom Drift vs GT: {odom_drift_m:6.2f} m",
                (30, 140), font, 0.50, (220, 50, 220), 2, cv2.LINE_AA)

def draw_map_hud(img, frame_idx, total_frames, pos_err_m, yaw_err_deg, est_pose, gt_pose, odom_pose, odom_drift_m, odom_step):
    """Draws HUD overlay on top-left corner of the satellite map view"""
    hud_w, hud_h = 470, 180
    overlay = img.copy()
    cv2.rectangle(overlay, (15, 15), (15 + hud_w, 15 + hud_h), (15, 15, 15), -1)
    cv2.addWeighted(overlay, 0.80, img, 0.20, 0, img)
    cv2.rectangle(img, (15, 15), (15 + hud_w, 15 + hud_h), (80, 80, 80), 1)

    font = cv2.FONT_HERSHEY_SIMPLEX
    color_white = (255, 255, 255)
    color_green = (80, 255, 80)
    color_cyan = (255, 255, 80)
    color_yellow = (80, 230, 255)
    color_magenta = (240, 80, 240)

    cv2.putText(img, "lambda-BRIEF Particle Filter + Odometry", (30, 40), font, 0.62, color_cyan, 2, cv2.LINE_AA)
    cv2.putText(img, f"Frame: [{frame_idx + 1:03d} / {total_frames:03d}]", (30, 64), font, 0.48, color_white, 1, cv2.LINE_AA)

    cv2.putText(img, f"Ground Truth:  ({gt_pose[0]:.1f}, {gt_pose[1]:.1f}) px", (30, 86), font, 0.44, color_green, 1, cv2.LINE_AA)
    cv2.putText(img, f"MCL Estimate:  ({est_pose[0]:.1f}, {est_pose[1]:.1f}) px", (30, 106), font, 0.44, color_yellow, 1, cv2.LINE_AA)
    cv2.putText(img, f"Pure Odometry: ({odom_pose[0]:.1f}, {odom_pose[1]:.1f}) px", (30, 126), font, 0.44, color_magenta, 1, cv2.LINE_AA)

    cv2.putText(img, f"MCL Error: {pos_err_m:5.2f} m  |  Odom Drift: {odom_drift_m:6.2f} m",
                (30, 155), font, 0.52, (50, 120, 255), 2, cv2.LINE_AA)

def draw_map_legend(img, x_pos, y_pos, has_odometry=True):
    """Draws color legend for markers on map"""
    leg_w = 270
    leg_h = 125 if has_odometry else 100
    overlay = img.copy()
    cv2.rectangle(overlay, (x_pos, y_pos), (x_pos + leg_w, y_pos + leg_h), (15, 15, 15), -1)
    cv2.addWeighted(overlay, 0.80, img, 0.20, 0, img)
    cv2.rectangle(img, (x_pos, y_pos), (x_pos + leg_w, y_pos + leg_h), (80, 80, 80), 1)

    font = cv2.FONT_HERSHEY_SIMPLEX
    # Ground truth
    cv2.circle(img, (x_pos + 18, y_pos + 22), 6, (0, 220, 0), -1, cv2.LINE_AA)
    cv2.putText(img, "Ground Truth (GPS/IMU)", (x_pos + 36, y_pos + 26), font, 0.42, (255, 255, 255), 1, cv2.LINE_AA)

    # Estimated
    cv2.circle(img, (x_pos + 18, y_pos + 46), 6, (0, 230, 255), -1, cv2.LINE_AA)
    cv2.putText(img, "Estimated Pose (MCL)", (x_pos + 36, y_pos + 50), font, 0.42, (255, 255, 255), 1, cv2.LINE_AA)

    # Particle cloud
    cv2.circle(img, (x_pos + 18, y_pos + 70), 4, (0, 140, 255), -1, cv2.LINE_AA)
    cv2.putText(img, "Particles (Swarm)", (x_pos + 36, y_pos + 74), font, 0.42, (255, 255, 255), 1, cv2.LINE_AA)

    # Odometry
    if has_odometry:
        cv2.circle(img, (x_pos + 18, y_pos + 95), 6, (220, 50, 220), -1, cv2.LINE_AA)
        cv2.putText(img, "Visual Odometry (Dead-Reck.)", (x_pos + 36, y_pos + 99), font, 0.42, (255, 255, 255), 1, cv2.LINE_AA)

def compute_optical_flow(prev_gray, curr_gray):
    """Finds corners and computes optical flow vectors"""
    if prev_gray is None or curr_gray is None:
        return []
    p0 = cv2.goodFeaturesToTrack(prev_gray, maxCorners=80, qualityLevel=0.04, minDistance=25)
    if p0 is None or len(p0) == 0:
        return []
    p1, st, err = cv2.calcOpticalFlowPyrLK(prev_gray, curr_gray, p0, None)
    if p1 is None or st is None:
        return []
    good_prev = p0[st == 1]
    good_curr = p1[st == 1]
    vectors = []
    for (x0, y0), (x1, y1) in zip(good_prev, good_curr):
        dist = math.hypot(x1 - x0, y1 - y0)
        if 2.0 < dist < 90.0:
            vectors.append(((int(x0), int(y0)), (int(x1), int(y1))))
    return vectors

def main():
    args = parse_args()

    errors_path = os.path.join(args.results_dir, "errors.txt")
    particles_path = os.path.join(args.results_dir, "particles_log.csv")

    err_data = load_errors(errors_path)
    if not err_data or len(err_data["frames"]) == 0:
        print(f"Error: Could not load errors from {errors_path}")
        sys.exit(1)

    total_frames = len(err_data["frames"])
    particles_data = load_particles(particles_path)
    print(f"[Video] Loaded {total_frames} error frames and {len(particles_data)} particle frames.")

    odom_file = args.odom_file
    if not odom_file:
        odom_file = auto_find_file(
            ["odom-*.txt", "odom.txt", "traj_odom.txt"],
            [args.results_dir, args.dataset_dir, os.path.join(args.dataset_dir, "traj") if args.dataset_dir else ""]
        )

    odom_records = []
    if odom_file and os.path.exists(odom_file):
        odom_records = load_odometry(odom_file)
        print(f"[Video] Loaded {len(odom_records)} odometry transitions from: {odom_file}")
    else:
        print("[Video] Warning: No odometry file provided or found. Odometry path will be omitted.")

    init_yaw = load_initial_heading(args.dataset_dir, args.results_dir)
    init_pose = (err_data["gt_x"][0], err_data["gt_y"][0], err_data["gt_z"][0], init_yaw)

    odom_poses, odom_steps, odom_dist = integrate_odometry(
        init_pose, odom_records, total_frames, args.pixel_to_meter
    )

    odom_drifts = []
    for f_idx in range(total_frames):
        gx = err_data["gt_x"][f_idx]
        gy = err_data["gt_y"][f_idx]
        ox = odom_poses[f_idx][0]
        oy = odom_poses[f_idx][1]
        drift_m = math.hypot(ox - gx, oy - gy) * args.pixel_to_meter
        odom_drifts.append(drift_m)

    print(f"[Video] Odometry integrated. Max drift: {max(odom_drifts):.2f} m | Final drift: {odom_drifts[-1]:.2f} m")

    map_img = None
    map_w, map_h = 4800, 4800
    base_map = None
    x_offset, y_offset = 0, 0
    scale_x, scale_y = 1.0, 1.0

    if args.layout in ["map", "side_by_side"]:
        map_path = args.map
        if not map_path and args.dataset_dir:
            map_path = auto_find_file(["*.jpg", "*.png"], [os.path.join(args.dataset_dir, "Mapas"), os.path.join(args.dataset_dir, "Mapa")])
        if not map_path or not os.path.exists(map_path):
            print(f"Error: Satellite map required for '{args.layout}' layout but not found.")
            sys.exit(1)

        print(f"[Video] Loading satellite map: {map_path} ...")
        map_img = cv2.imread(map_path)
        if map_img is None:
            print(f"Error: Could not read satellite map from {map_path}")
            sys.exit(1)
        map_h, map_w = map_img.shape[:2]

        all_x = err_data["gt_x"] + err_data["est_x"] + [p[0] for p in odom_poses]
        all_y = err_data["gt_y"] + err_data["est_y"] + [p[1] for p in odom_poses]
        min_x, max_x = min(all_x), max(all_x)
        min_y, max_y = min(all_y), max(all_y)

        if args.zoom_margin > 0:
            roi_x1 = max(0, int(min_x - args.zoom_margin))
            roi_y1 = max(0, int(min_y - args.zoom_margin))
            roi_x2 = min(map_w, int(max_x + args.zoom_margin))
            roi_y2 = min(map_h, int(max_y + args.zoom_margin))

            roi_w = roi_x2 - roi_x1
            roi_h = roi_y2 - roi_y1
            target_aspect = (args.video_width) / args.video_height if args.layout == "map" else (args.video_width // 2) / (args.video_height - 70)
            curr_aspect = roi_w / roi_h

            if curr_aspect > target_aspect:
                new_h = int(roi_w / target_aspect)
                diff = new_h - roi_h
                roi_y1 = max(0, roi_y1 - diff // 2)
                roi_y2 = min(map_h, roi_y2 + diff // 2)
            else:
                new_w = int(roi_h * target_aspect)
                diff = new_w - roi_w
                roi_x1 = max(0, roi_x1 - diff // 2)
                roi_x2 = min(map_w, roi_x2 + diff // 2)

            base_map = map_img[roi_y1:roi_y2, roi_x1:roi_x2]
            x_offset, y_offset = roi_x1, roi_y1
        else:
            base_map = map_img
            x_offset, y_offset = 0, 0

    camera_frames = []
    if args.layout in ["camera", "side_by_side"]:
        camera_frames = load_camera_frames(
            args.dataset_dir, args.images_dir, args.results_dir, total_frames, args.start_frame
        )
        print(f"[Video] Loaded {len(camera_frames)} camera image paths for '{args.layout}' layout.")

    target_w = args.video_width
    target_h = args.video_height
    if args.layout == "side_by_side":
        target_w = max(1920, target_w)
        target_h = max(1080, target_h)

    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out_video = cv2.VideoWriter(args.output, fourcc, args.fps, (target_w, target_h))

    print(f"[Video] Rendering {total_frames} frames to {args.output} ({target_w}x{target_h} @ {args.fps} FPS, layout: {args.layout})...")

    gt_map_trail = []
    est_map_trail = []
    odom_map_trail = []

    gt_mini_trail = [(err_data["gt_x"][i], err_data["gt_y"][i]) for i in range(total_frames)]
    odom_mini_trail = [(odom_poses[i][0], odom_poses[i][1]) for i in range(total_frames)]

    prev_cam_gray = None

    map_name = os.path.splitext(os.path.basename(map_path))[0] if (args.layout in ["map", "side_by_side"] and map_path) else ""

    for f_idx in range(total_frames):
        frame_num = err_data["frames"][f_idx]

        # -------------------------------------------------------------
        # A. Render Map Component
        # -------------------------------------------------------------
        map_canvas = None
        if args.layout in ["map", "side_by_side"]:
            map_target_w = target_w if args.layout == "map" else target_w // 2
            map_target_h = target_h if args.layout == "map" else target_h - 70

            scale_x = map_target_w / base_map.shape[1]
            scale_y = map_target_h / base_map.shape[0]

            map_canvas = cv2.resize(base_map, (map_target_w, map_target_h), interpolation=cv2.INTER_AREA)

            # Particles
            if frame_num in particles_data:
                p_list = particles_data[frame_num]
                max_w = max([p[4] for p in p_list]) if p_list else 1.0
                min_w = min([p[4] for p in p_list]) if p_list else 0.0

                overlay = map_canvas.copy()
                for (px, py, pz, pyaw, pw) in p_list:
                    cx = int((px - x_offset) * scale_x)
                    cy = int((py - y_offset) * scale_y)

                    if 0 <= cx < map_target_w and 0 <= cy < map_target_h:
                        norm_w = (pw - min_w) / (max_w - min_w + 1e-9)
                        r_val = int(255 * (1.0 - norm_w * 0.4))
                        g_val = int(140 + 115 * norm_w)
                        b_val = int(30 * (1.0 - norm_w))
                        color = (b_val, g_val, r_val)
                        radius = 3 if norm_w < 0.7 else 5
                        cv2.circle(overlay, (cx, cy), radius, color, -1)

                        if norm_w > 0.5:
                            rad = math.radians(pyaw)
                            tx = int(cx + 10 * math.cos(rad))
                            ty = int(cy + 10 * math.sin(rad))
                            cv2.line(overlay, (cx, cy), (tx, ty), (255, 255, 255), 1)

                cv2.addWeighted(overlay, 0.65, map_canvas, 0.35, 0, map_canvas)

            # Trails
            curr_gt_x = int((err_data["gt_x"][f_idx] - x_offset) * scale_x)
            curr_gt_y = int((err_data["gt_y"][f_idx] - y_offset) * scale_y)
            curr_est_x = int((err_data["est_x"][f_idx] - x_offset) * scale_x)
            curr_est_y = int((err_data["est_y"][f_idx] - y_offset) * scale_y)
            curr_od_x = int((odom_poses[f_idx][0] - x_offset) * scale_x)
            curr_od_y = int((odom_poses[f_idx][1] - y_offset) * scale_y)

            gt_map_trail.append((curr_gt_x, curr_gt_y))
            est_map_trail.append((curr_est_x, curr_est_y))
            odom_map_trail.append((curr_od_x, curr_od_y))

            for i in range(1, len(gt_map_trail)):
                cv2.line(map_canvas, gt_map_trail[i-1], gt_map_trail[i], (0, 220, 0), 2, cv2.LINE_AA)

            for i in range(1, len(est_map_trail)):
                cv2.line(map_canvas, est_map_trail[i-1], est_map_trail[i], (0, 230, 255), 2, cv2.LINE_AA)

            if args.show_odometry and len(odom_records) > 0:
                for i in range(1, len(odom_map_trail)):
                    cv2.line(map_canvas, odom_map_trail[i-1], odom_map_trail[i], (220, 50, 220), 2, cv2.LINE_AA)

            # Markers
            cv2.circle(map_canvas, (curr_gt_x, curr_gt_y), 9, (0, 255, 0), 2, cv2.LINE_AA)
            cv2.circle(map_canvas, (curr_gt_x, curr_gt_y), 4, (0, 255, 0), -1, cv2.LINE_AA)

            cv2.circle(map_canvas, (curr_est_x, curr_est_y), 9, (0, 255, 255), 2, cv2.LINE_AA)
            cv2.circle(map_canvas, (curr_est_x, curr_est_y), 4, (0, 255, 255), -1, cv2.LINE_AA)
            cv2.drawMarker(map_canvas, (curr_est_x, curr_est_y), (0, 255, 255), cv2.MARKER_CROSS, 16, 2)

            if args.show_odometry and len(odom_records) > 0:
                cv2.circle(map_canvas, (curr_od_x, curr_od_y), 9, (240, 80, 240), 2, cv2.LINE_AA)
                cv2.circle(map_canvas, (curr_od_x, curr_od_y), 4, (240, 80, 240), -1, cv2.LINE_AA)
                cv2.drawMarker(map_canvas, (curr_od_x, curr_od_y), (240, 80, 240), cv2.MARKER_DIAMOND, 14, 2)

            cv2.line(map_canvas, (curr_gt_x, curr_gt_y), (curr_est_x, curr_est_y), (0, 0, 255), 1, cv2.LINE_AA)

            if args.layout == "map":
                draw_map_hud(
                    map_canvas, f_idx, total_frames,
                    err_data["pos_err"][f_idx], err_data["yaw_err"][f_idx],
                    (err_data["est_x"][f_idx], err_data["est_y"][f_idx]),
                    (err_data["gt_x"][f_idx], err_data["gt_y"][f_idx]),
                    (odom_poses[f_idx][0], odom_poses[f_idx][1]),
                    odom_drifts[f_idx],
                    odom_steps[f_idx]
                )
                draw_map_legend(map_canvas, map_target_w - 290, 15, has_odometry=args.show_odometry and len(odom_records) > 0)

        # -------------------------------------------------------------
        # B. Render Camera Component
        # -------------------------------------------------------------
        cam_canvas = None
        if args.layout in ["camera", "side_by_side"]:
            cam_target_w = target_w if args.layout == "camera" else target_w // 2
            cam_target_h = target_h if args.layout == "camera" else target_h - 70

            raw_img = None
            if f_idx < len(camera_frames) and os.path.exists(camera_frames[f_idx]):
                raw_img = cv2.imread(camera_frames[f_idx])

            if raw_img is None:
                raw_img = np.zeros((cam_target_h, cam_target_w, 3), dtype=np.uint8)
                cv2.putText(raw_img, f"Frame {f_idx + 1}: Image not found", (50, 150),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)

            cam_canvas = cv2.resize(raw_img, (cam_target_w, cam_target_h), interpolation=cv2.INTER_AREA)
            curr_cam_gray = cv2.cvtColor(cam_canvas, cv2.COLOR_BGR2GRAY)

            if args.show_flow and prev_cam_gray is not None:
                flow_vectors = compute_optical_flow(prev_cam_gray, curr_cam_gray)
                for (p0, p1) in flow_vectors:
                    cv2.arrowedLine(cam_canvas, p0, p1, (0, 255, 120), 1, tipLength=0.25, line_type=cv2.LINE_AA)
                    cv2.circle(cam_canvas, p0, 2, (0, 255, 255), -1, cv2.LINE_AA)

            prev_cam_gray = curr_cam_gray

            curr_yaw_rad = odom_poses[f_idx][3]
            curr_yaw_deg = math.degrees(curr_yaw_rad) % 360
            draw_camera_hud(
                cam_canvas, f_idx, total_frames,
                odom_steps[f_idx],
                odom_drifts[f_idx],
                odom_dist[f_idx],
                curr_yaw_deg,
                err_data["pos_err"][f_idx]
            )

            draw_compass(cam_canvas, (cam_target_w - 60, 65), 40, curr_yaw_rad)

            if args.layout == "camera":
                draw_minimap(
                    cam_canvas, odom_mini_trail, gt_mini_trail, f_idx,
                    pos=(cam_target_w - 280, cam_target_h - 220), size=(260, 200)
                )

        # -------------------------------------------------------------
        # C. Assemble Final Frame & Write
        # -------------------------------------------------------------
        if args.layout == "map":
            out_video.write(map_canvas)
        elif args.layout == "camera":
            out_video.write(cam_canvas)
        elif args.layout == "side_by_side":
            final_frame = np.zeros((target_h, target_w, 3), dtype=np.uint8)

            header_h = 70
            cv2.rectangle(final_frame, (0, 0), (target_w, header_h), (20, 20, 20), -1)
            cv2.line(final_frame, (0, header_h), (target_w, header_h), (80, 80, 80), 2)

            font = cv2.FONT_HERSHEY_SIMPLEX
            title_text = "lambda-BRIEF: UAV LOCALIZATION — ONBOARD VISION vs SATELLITE MAP CONVERGENCE"
            cv2.putText(final_frame, title_text, (30, 32), font, 0.70, (255, 255, 80), 2, cv2.LINE_AA)

            map_label = f"Map: {map_name}  |  " if map_name else ""
            stats_text = f"{map_label}Frame: [{f_idx + 1:03d} / {total_frames:03d}]  |  MCL Error: {err_data['pos_err'][f_idx]:5.2f} m  |  Odom Drift: {odom_drifts[f_idx]:6.2f} m"
            cv2.putText(final_frame, stats_text, (30, 58), font, 0.50, (220, 220, 220), 1, cv2.LINE_AA)

            cv2.putText(final_frame, "ONBOARD CAMERA VIEW", (40, header_h + 26), font, 0.55, (0, 230, 255), 2, cv2.LINE_AA)
            cv2.putText(final_frame, f"SATELLITE MAP: {map_name}", (target_w // 2 + 30, header_h + 26), font, 0.55, (0, 230, 255), 2, cv2.LINE_AA)

            final_frame[header_h : header_h + cam_canvas.shape[0], 0 : target_w // 2] = cam_canvas
            final_frame[header_h : header_h + map_canvas.shape[0], target_w // 2 : target_w] = map_canvas

            cv2.line(final_frame, (target_w // 2, header_h), (target_w // 2, target_h), (80, 80, 80), 2)

            draw_map_legend(final_frame, target_w - 290, header_h + 15, has_odometry=args.show_odometry and len(odom_records) > 0)

            out_video.write(final_frame)

    out_video.release()
    print(f"\n[Success] Video generated successfully: {args.output}")

if __name__ == "__main__":
    main()
