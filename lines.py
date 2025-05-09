import os


def count_lines_in_file(filepath):
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        return sum(1 for _ in f)


def count_lines_in_dir(root_dir, exts=(".cpp", ".h", ".hpp", ".c", ".cc")):
    total_lines = 0
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.endswith(exts):
                file_path = os.path.join(dirpath, filename)
                lines = count_lines_in_file(file_path)
                print(f"{file_path}: {lines} 行")
                total_lines += lines
    print(f"\n总行数: {total_lines} 行")


if __name__ == "__main__":
    code_dir = "src"
    count_lines_in_dir(code_dir)
