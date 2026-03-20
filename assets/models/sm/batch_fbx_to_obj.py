"""
Batch FBX -> OBJ + MTL Converter
---------------------------------
Usage (PowerShell):
    & "C:\Program Files\Blender Foundation\Blender 4.x\blender.exe" `
        --background --python batch_fbx_to_obj.py -- `
        --input  "C:\models\sm" `
        --output "C:\models\sm-raw" `
        --albedo "C:\textures\sm\albedo.png"

Arguments:
    --input     Folder containing .fbx files (searched recursively)
    --output    Destination folder for .obj / .mtl files
    --albedo    Path to the albedo / base-color texture (sets map_Kd).
                All other texture maps are stripped from the .mtl.
                Omit to strip ALL texture references entirely.
"""

import bpy
import os
import sys
import argparse
import glob


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def parse_args():
    """Extract args that come after the '--' separator Blender requires."""
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []

    parser = argparse.ArgumentParser(description="Batch FBX -> OBJ converter")
    parser.add_argument("--input",  required=True, help="Folder with .fbx files")
    parser.add_argument("--output", required=True, help="Output folder for .obj/.mtl")
    parser.add_argument("--albedo", default=None,
                        help="Albedo/base-color texture path written as map_Kd in every .mtl")
    return parser.parse_args(argv)


# ---------------------------------------------------------------------------
# Blender helpers
# ---------------------------------------------------------------------------

def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for col in [bpy.data.meshes, bpy.data.materials,
                bpy.data.images, bpy.data.armatures, bpy.data.actions]:
        for block in list(col):
            if block.users == 0:
                col.remove(block)


def import_fbx(filepath):
    bpy.ops.import_scene.fbx(
        filepath=filepath,
        use_custom_normals=True,
        use_image_search=True,
        automatic_bone_orientation=True,
    )


def export_obj(filepath):
    try:                          # Blender 3.3+
        bpy.ops.wm.obj_export(
            filepath=filepath,
            export_materials=True,
            export_normals=True,
            export_uv=True,
            export_triangulated_mesh=False,
            export_object_groups=True,
        )
    except AttributeError:        # Blender < 3.3
        bpy.ops.export_scene.obj(
            filepath=filepath,
            use_materials=True,
            use_normals=True,
            use_uvs=True,
            use_triangles=False,
            group_by_object=True,
        )


# ---------------------------------------------------------------------------
# MTL patching  –  strip every map_* line, then inject map_Kd if supplied
# ---------------------------------------------------------------------------

# All OBJ/MTL texture keywords we want to remove
ALL_MAP_KEYS = (
    "map_kd", "map_ks", "map_ka", "map_ke",
    "map_ns", "map_d",  "map_bump", "bump", "disp", "norm",
    "map_pr", "map_pm", "map_rma", "map_orm",   # PBR extensions
)

def patch_mtl(mtl_path, albedo_path):
    """
    Remove all existing texture map lines from the .mtl, then add a single
    map_Kd line (albedo/base-color) to every material block if albedo_path
    is provided.
    """
    with open(mtl_path, "r", encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    # Forward-slashes only (OBJ spec); absolute path is fine here
    albedo_fwd = albedo_path.replace("\\", "/") if albedo_path else None

    new_lines = []
    for line in lines:
        token = line.strip().split()[0].lower() if line.strip() else ""

        # Drop ALL existing texture references
        if token in ALL_MAP_KEYS:
            continue

        new_lines.append(line)

        # After each "newmtl" block starts, we'll inject map_Kd later –
        # actually we inject it once at the very end of each material block.
        # Easier approach: inject right after the last non-map line of each
        # material, which we do by tracking "newmtl" boundaries below.

    # ---- second pass: insert map_Kd after each material's property lines ----
    if albedo_fwd:
        patched = []
        i = 0
        while i < len(new_lines):
            patched.append(new_lines[i])
            current = new_lines[i].strip().lower()
            # When we hit the next "newmtl" or end-of-file, inject before it
            if i + 1 < len(new_lines):
                next_tok = new_lines[i + 1].strip().split()[0].lower() \
                           if new_lines[i + 1].strip() else ""
                if next_tok == "newmtl":
                    patched.append(f"map_Kd {albedo_fwd}\n")
                    patched.append("\n")
            i += 1
        # Inject for the final material block
        patched.append(f"map_Kd {albedo_fwd}\n")
        new_lines = patched

    with open(mtl_path, "w", encoding="utf-8") as f:
        f.writelines(new_lines)


# ---------------------------------------------------------------------------
# Main batch loop
# ---------------------------------------------------------------------------

def main():
    args       = parse_args()
    input_dir  = os.path.abspath(args.input)
    output_dir = os.path.abspath(args.output)
    albedo     = os.path.abspath(args.albedo) if args.albedo else None

    os.makedirs(output_dir, exist_ok=True)

    fbx_files = sorted(glob.glob(os.path.join(input_dir, "**", "*.fbx"), recursive=True))
    total     = len(fbx_files)

    if total == 0:
        print(f"[WARN] No .fbx files found in: {input_dir}")
        return

    print(f"\n{'='*60}")
    print(f"  Batch FBX -> OBJ  |  {total} files")
    print(f"  Albedo : {albedo or '(none – textures will be stripped)'}")
    print(f"{'='*60}\n")

    ok, failed = [], []

    for i, fbx_path in enumerate(fbx_files, 1):
        fbx_name = os.path.splitext(os.path.basename(fbx_path))[0]
        fbx_dir  = os.path.dirname(fbx_path)

        rel_dir  = os.path.relpath(fbx_dir, input_dir)
        dest_dir = os.path.join(output_dir, rel_dir)
        os.makedirs(dest_dir, exist_ok=True)

        obj_path = os.path.join(dest_dir, fbx_name + ".obj")
        mtl_path = os.path.join(dest_dir, fbx_name + ".mtl")

        print(f"[{i:>4}/{total}] {fbx_name}")

        try:
            clear_scene()
            import_fbx(fbx_path)
            export_obj(obj_path)

            if os.path.exists(mtl_path):
                patch_mtl(mtl_path, albedo)

            print(f"         OK  ->  {obj_path}")
            ok.append(fbx_name)

        except Exception as e:
            print(f"         FAILED: {e}")
            failed.append((fbx_name, str(e)))

    print(f"\n{'='*60}")
    print(f"  Done.  OK {len(ok)}   Failed {len(failed)}")
    if failed:
        print("\n  Failed files:")
        for name, err in failed:
            print(f"    - {name}: {err}")
    print(f"{'='*60}\n")


if __name__ == "__main__":
    main()
