from pathlib import Path
import shutil
import filecmp
from fnmatch import fnmatchcase
import ctypes

# 获取当前脚本所在目录的文件夹名
project_name = Path(__file__).resolve().parent.name

# 自动生成两个目录
A = Path(r"C:\Users\Pudge\source\repos") / project_name
if not A.is_dir():
    A = Path(r"C:\Users\Pudge\source\repos\Misc") / project_name
    if not A.is_dir():
        A=Path(r"D:\Work") / project_name
        if not A.is_dir():
            A=Path(r"D:\Work\Misc") / project_name
            
            
#B = Path(r"D:\Github\Repositories") / project_name
B=Path(__file__).resolve().parent

print("项目名称：", project_name)
print("源目录 A：", A)
print("目标目录 B：", B)

# 相对于 B 的路径：这些文件或目录永远保留
KEEP = {
    ".git",
    "Dependencies",
    "scripts",
    "./.gitignore",
    "./*.q",
    "./*.md",
    "./*.py",
    "./*.jpg",
    "./*.png",
}

# 先设为 True，只预览操作，不实际修改文件
DRY_RUN = False


def is_kept(relative_path: str) -> bool:
    rel = Path(relative_path).as_posix().removeprefix("./")
    name = Path(rel).name

    for item in KEEP:
        pattern = item.replace("\\", "/")
        normalized = pattern.removeprefix("./")

        # 保护 KEEP 中指定的根目录文件夹及其全部内容
        if "/" not in normalized and (
            rel == normalized or rel.startswith(normalized + "/")
        ) and not any(c in normalized for c in "*?["):
            return True

        # 任意目录层级中的文件名，例如 *.png
        if "/" not in pattern and fnmatchcase(name.lower(), pattern.lower()):
            return True

        # 仅项目根目录，例如 ./*.png
        if pattern.startswith("./"):
            root_pattern = pattern[2:]
            if "/" not in root_pattern and "/" not in rel:
                if fnmatchcase(rel.lower(), root_pattern.lower()):
                    return True
            continue

        # 指定相对路径，例如 script/*.jpg
        if "/" in pattern and fnmatchcase(rel.lower(), pattern.lower()):
            return True

    return False


def sync():
    if not A.is_dir():
        raise RuntimeError(f"源目录不存在：{A}")

    if not B.is_dir():
        raise RuntimeError(f"目标目录不存在：{B}")

    # 复制 A 中的文件到 B，覆盖同名文件
    for src in A.rglob("*"):
        rel = src.relative_to(A)
        dst = B / rel

        if src.is_symlink():
            print(f"跳过符号链接：{rel}")
            continue

        if src.is_dir():
            if not dst.exists() and not DRY_RUN:
                dst.mkdir(parents=True, exist_ok=True)
                src_attrs = ctypes.windll.kernel32.GetFileAttributesW(str(src))
                dst_attrs=ctypes.windll.kernel32.SetFileAttributesW(str(dst), src_attrs)
                #print(hex(src_attrs))
                #print(hex(dst_attrs))
            continue

        if src.is_file():
            # 不覆盖 B 独有的保留文件
            if is_kept(rel.as_posix()) and dst.exists():
                continue

            if (
                not dst.exists()
                or not dst.is_file()
                or not filecmp.cmp(src, dst, shallow=False)
            ):
                print(f"复制/更新：{rel}")
                if not DRY_RUN:
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(src, dst)
                    
    # 删除 B 中 A 不存在的文件
    for dst in sorted(B.rglob("*"), reverse=True):
        rel = dst.relative_to(B)

        if dst.is_symlink():
            continue

        if is_kept(rel.as_posix()):
            continue

        src = A / rel

        if dst.is_file() and not src.is_file():
            print(f"删除文件：{rel}")
            if not DRY_RUN:
                dst.unlink()

        elif dst.is_dir() and not src.is_dir():
            # 目录内只要还有保留文件，就不会删除该目录
            if any(dst.iterdir()):
                continue
            print(f"删除空目录：{rel}")
            if not DRY_RUN:
                dst.rmdir()

    print("\n预览完成。" if DRY_RUN else "\n同步完成。")


if __name__ == "__main__":
    sync()