import math
import os

# --- 参数 ---
num_vertical = 25  # 垂直方向的粒子圈数 (高度)
num_horizontal = 15  # 水平方向每个圈的粒子数 (周长)
stocking_height = 600.0  # 丝袜的总高度
stocking_radius = 80.0  # 丝袜主要部分的半径
output_filename = "cloth_save.txt"
pin_top_ring = True  # 是否固定顶部圈

# 偏移量，使丝袜不完全在原点
offset_x = 300.0
offset_y = -300.0  # 调整Y偏移使顶部接近 Y=0
offset_z = 0.0

# --- 数据列表 ---
particles = []  # 存储粒子信息 [x, y, z, pinned]
constraints = []  # 存储约束信息 [idx1, idx2, rest_length]


# --- 辅助函数 ---
def get_particle_index(v, h):
    """根据垂直和水平索引获取粒子在一维列表中的索引"""
    return v * num_horizontal + h


def distance(p1_coords, p2_coords):
    """计算两个三维坐标点之间的距离"""
    dx = p1_coords[0] - p2_coords[0]
    dy = p1_coords[1] - p2_coords[1]
    dz = p1_coords[2] - p2_coords[2]
    return math.sqrt(dx * dx + dy * dy + dz * dz)


# --- 生成粒子 ---
print("Generating particles...")
for v in range(num_vertical):
    y = (v / (num_vertical - 1)) * stocking_height + offset_y
    # 底部逐渐变细 (例如最后 1/4 高度开始变细)
    taper_start_ratio = 0.75
    current_radius = stocking_radius
    if v < num_vertical * (1.0 - taper_start_ratio):
        # 从 taper_start_ratio 处开始线性递减到0
        taper_progress = v / (num_vertical * (1.0 - taper_start_ratio))
        current_radius = stocking_radius * (1.0 - taper_progress)

    for h in range(num_horizontal):
        theta = (h / num_horizontal) * 2 * math.pi
        x = current_radius * math.cos(theta) + offset_x
        z = current_radius * math.sin(theta) + offset_z

        # 固定顶部圈
        pinned = 1 if (pin_top_ring and v == num_vertical - 1) else 0

        particles.append([x, y, z, pinned])

print(f"Generated {len(particles)} particles.")

# --- 生成约束 ---
print("Generating constraints...")
# 垂直约束
for v in range(num_vertical - 1):
    for h in range(num_horizontal):
        idx1 = get_particle_index(v, h)
        idx2 = get_particle_index(v + 1, h)
        p1 = particles[idx1]
        p2 = particles[idx2]
        rest_len = distance(p1, p2)
        constraints.append([idx1, idx2, rest_len])

# 水平约束
for v in range(num_vertical):
    # 如果半径变得非常小，水平约束可能意义不大或导致问题，可以跳过
    if v < num_vertical * (1.0 - taper_start_ratio):
        taper_progress = v / (num_vertical * (1.0 - taper_start_ratio))
        current_radius_check = stocking_radius * (1.0 - taper_progress)
        if current_radius_check < 1.0:  # 半径小于1时跳过水平约束
            continue

    for h in range(num_horizontal):
        idx1 = get_particle_index(v, h)
        # 连接到下一个，并绕回第一个
        idx2 = get_particle_index(v, (h + 1) % num_horizontal)
        p1 = particles[idx1]
        p2 = particles[idx2]
        rest_len = distance(p1, p2)
        constraints.append([idx1, idx2, rest_len])

# # 可选：添加斜向约束（增加稳定性）
# for v in range(num_vertical - 1):
#      # 同样，在半径很小时可以跳过
#     if v < num_vertical * (1.0 - taper_start_ratio):
#         taper_progress = v / (num_vertical * (1.0 - taper_start_ratio))
#         current_radius_check = stocking_radius * (1.0 - taper_progress)
#         if current_radius_check < 1.0:
#             continue
#     for h in range(num_horizontal):
#         idx1 = get_particle_index(v, h)
#         idx2 = get_particle_index(v + 1, (h + 1) % num_horizontal)
#         p1 = particles[idx1]
#         p2 = particles[idx2]
#         rest_len = distance(p1, p2)
#         constraints.append([idx1, idx2, rest_len])

print(f"Generated {len(constraints)} constraints.")

# --- 写入文件 ---
print(f"Writing data to {output_filename}...")
try:
    with open(output_filename, "w") as f:
        # 写入粒子
        f.write(f"# particles\n")
        for p in particles:
            f.write(f"{p[0]:.4f} {p[1]:.4f} {p[2]:.4f} {p[3]}\n")

        # 写入约束
        f.write(f"# constraints\n")
        for c in constraints:
            f.write(f"c {c[0]} {c[1]}\n")
    print("File written successfully.")
except IOError as e:
    print(f"Error writing file: {e}")
