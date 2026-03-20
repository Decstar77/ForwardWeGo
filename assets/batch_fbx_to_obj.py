"""
Batch FBX → OBJ + MTL Converter with Texture Repathing
--------------------------------------------------------
Usage (headless):
    blender --background --python batch_fbx_to_obj.py -- \
        --input  /path/to/fbx_folder \
        --output /path/to/obj_folder \
        --textures /path/to/new/textures \
        --tex-mode copy

Arguments:
    --input     Folder containing .fbx files (searched recursively)
    --output    Destination folder for .obj / .mtl files
    --textures  (optional) New root folder for texture paths in the .mtl
    --tex-mode  How to handle textures:
                  repath  – rewrite paths in .mtl to point at --textures (default)
                  copy    – copy original textures into --textures and repath
                  strip   – remove all texture references from .mtl
"""

import bpy
import os
import sys
import shutil
import argparse
import glob


# ── helpers ──────────────────────────────────────────────────────────────────

def parse_args():
    """Extract args that come after the '--' separator Blender requires."""
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser(description="Batch FBX → OBJ converter")
    parser.add_argument("--input",    required=True,  help="Folder with .fbx files")
    parser.add_argument("--output",   required=True,  help="Output folder for .obj/.mtl")
    parser.add_argument("--textures", default=None,   help="New texture root folder")
    parser.add_argument("--tex-mode", default="repath",
                        choices=["repath", "copy", "strip"],
                        help="Texture handling mode")
    return parser.parse_args(argv)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    # Purge orphan data
    for block_type in [bpy.data.meshes, bpy.data.materials,
                       bpy.data.images, bpy.data.armatures,
                       bpy.data.actions]:
        for block in list(block_type):
            if block.users == 0:
                block_type.remove(block)


def collect_fbx_files(input_dir):
    pattern = os.path.join(input_dir, "**", "*.fbx")
    return sorted(glob.glob(pattern, recursive=True))


def import_fbx(filepath):
    bpy.ops.import_scene.fbx(
        filepath=filepath,
        use_custom_normals=True,
        use_image_search=True,
        automatic_bone_orientation=True,
    )


def export_obj(filepath):
    """Export with Blender 3.x+ exporter (io_scene_obj built-in)."""
    try:
        # Blender 3.3+ uses the new exporter
        bpy.ops.wm.obj_export(
            filepath=filepath,
            export_materials=True,
            export_normals=True,
            export_uv=True,
            export_triangulated_mesh=False,
            export_object_groups=True,
        )
    except AttributeError:
        # Fallback for Blender < 3.3
        bpy.ops.export_scene.obj(
            filepath=filepath,
            use_materials=True,
            use_normals=True,
            use_uvs=True,
            use_triangles=False,
            group_by_object=True,
        )


# ── texture repathing ─────────────────────────────────────────────────────────

def repath_mtl(mtl_path, new_tex_root, mode, original_fbx_dir):
    """
    Parse the .mtl file and rewrite map_* texture paths.
    Modes:
      repath – point to new_tex_root using the original filename
      copy   – copy texture next to new_tex_root and repath
      strip  – delete all map_* lines
    """
    MAP_KEYS = ("map_Kd", "map_Ks", "map_Ka", "map_Ke",
                "map_Ns", "map_d", "map_bump", "bump", "disp", "norm")

    with open(mtl_path, "r", encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    new_lines = []
    for line in lines:
        stripped = line.strip()
        keyword = stripped.split()[0].lower() if stripped else ""

        is_map = any(stripped.lower().startswith(k.lower()) for k in MAP_KEYS)

        if is_map:
            if mode == "strip":
                continue  # drop the line

            # Extract the texture path (last token on the line)
            parts = stripped.split()
            original_tex_path = parts[-1]

            # Resolve original texture location
            if not os.path.isabs(original_tex_path):
                candidate = os.path.join(original_fbx_dir, original_tex_path)
                if os.path.exists(candidate):
                    original_tex_path = candidate

            tex_filename = os.path.basename(original_tex_path)

            if mode == "copy" and new_tex_root:
                os.makedirs(new_tex_root, exist_ok=True)
                dest = os.path.join(new_tex_root, tex_filename)
                if os.path.exists(original_tex_path) and not os.path.exists(dest):
                    shutil.copy2(original_tex_path, dest)
                new_path = dest
            elif mode == "repath" and new_tex_root:
                new_path = os.path.join(new_tex_root, tex_filename)
            else:
                new_path = original_tex_path  # leave as-is if no root given

            # Use forward slashes (OBJ spec)
            new_path = new_path.replace("\\", "/")
            key_token = parts[0]
            new_lines.append(f"{key_token} {new_path}\n")
        else:
            new_lines.append(line)

    with open(mtl_path, "w", encoding="utf-8") as f:
        f.writelines(new_lines)


# ── main loop ─────────────────────────────────────────────────────────────────

def main():
    args = parse_args()

    input_dir  = os.path.abspath(args.input)
    output_dir = os.path.abspath(args.output)
    tex_root   = os.path.abspath(args.textures) if args.textures else None
    tex_mode   = args.tex_mode

    os.makedirs(output_dir, exist_ok=True)

    fbx_files = collect_fbx_files(input_dir)
    total = len(fbx_files)

    if total == 0:
        print(f"[WARN] No .fbx files found in: {input_dir}")
        return

    print(f"\n{'='*60}")
    print(f"  Batch FBX → OBJ  |  {total} files  |  mode: {tex_mode}")
    print(f"{'='*60}\n")

    ok, failed = [], []

    for i, fbx_path in enumerate(fbx_files, 1):
        fbx_name = os.path.splitext(os.path.basename(fbx_path))[0]
        fbx_dir  = os.path.dirname(fbx_path)

        # Preserve sub-folder structure relative to input_dir
        rel_dir    = os.path.relpath(fbx_dir, input_dir)
        dest_dir   = os.path.join(output_dir, rel_dir)
        os.makedirs(dest_dir, exist_ok=True)

        obj_path = os.path.join(dest_dir, fbx_name + ".obj")
        mtl_path = os.path.join(dest_dir, fbx_name + ".mtl")

        print(f"[{i:>4}/{total}] {fbx_name}")

        try:
            clear_scene()
            import_fbx(fbx_path)
            export_obj(obj_path)

            if os.path.exists(mtl_path) and (tex_root or tex_mode == "strip"):
                repath_mtl(mtl_path, tex_root, tex_mode, fbx_dir)

            print(f"         ✓  →  {obj_path}")
            ok.append(fbx_name)

        except Exception as e:
            print(f"         ✗  FAILED: {e}")
            failed.append((fbx_name, str(e)))

    # ── summary ──
    print(f"\n{'='*60}")
    print(f"  Done.  ✓ {len(ok)} succeeded   ✗ {len(failed)} failed")
    if failed:
        print("\n  Failed files:")
        for name, err in failed:
            print(f"    • {name}: {err}")
    print(f"{'='*60}\n")


if __name__ == "__main__":
    main()
