# Pulls espressif/esp32-camera only when components/cam_deps is symlinked
# (PlatformIO esp32-cam via scripts/ensure_cam_component.py).
idf_component_register()
