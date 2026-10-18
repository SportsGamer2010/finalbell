"""Create an empty animation preview map if it does not already exist.

Run inside the Unreal editor Python console or with -ExecutePythonScript.
This script does not open, delete, or modify /Game/Maps/Ring_P1.
"""

from __future__ import annotations


MAP_PATH = "/Game/Maps/AnimPreview_Sandbox"


def main() -> None:
    import unreal

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log("AnimPreview_Sandbox already exists. Leaving the ring and this map untouched.")
        return
    unreal.EditorAssetLibrary.make_directory("/Game/Maps")
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_editor.new_level(MAP_PATH):
        unreal.log_error("Could not create AnimPreview_Sandbox.")
        return
    unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("Created /Game/Maps/AnimPreview_Sandbox. Play it from the editor to scrub clips. This script did not run playback.")


if __name__ == "__main__":
    main()
