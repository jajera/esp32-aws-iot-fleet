Import("env")

"""Symlink components/cam_deps only for esp32-cam so esp32-camera is not
pulled into every env via the IDF component manager."""

from pathlib import Path

proj = Path(env["PROJECT_DIR"])
components = proj / "components"
link = components / "cam_deps"
src = proj / "components_cam" / "cam_deps"


def _rm_link():
    if link.is_symlink() or link.is_file():
        link.unlink()
    elif link.is_dir():
        # Refuse to delete a real tree — operator must clean up manually.
        print("*** components/cam_deps is a real directory; leave it alone ***")


components.mkdir(exist_ok=True)

if env["PIOENV"] == "esp32-cam":
    if not src.is_dir():
        print("*** missing components_cam/cam_deps — camera dep unavailable ***")
    elif link.is_symlink() and link.resolve() == src.resolve():
        pass
    else:
        _rm_link()
        if not link.exists():
            link.symlink_to(src.resolve())
            print("*** linked components/cam_deps -> components_cam/cam_deps ***")
else:
    if link.is_symlink():
        link.unlink()
        print("*** removed components/cam_deps symlink (non-cam env) ***")
