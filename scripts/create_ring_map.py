"""
Create/overwrite Content/Maps/Ring_P1 with P1 arena actors.
Uses LevelEditorSubsystem (UE 5.8). Runtime auto-arena still works without this map.
"""

import unreal


MAP_PATH = "/Game/Maps/Ring_P1"


def spawn(cls_path: str, loc, rot=None):
    if rot is None:
        rot = unreal.Rotator(0.0, 0.0, 0.0)
    cls = unreal.load_class(None, cls_path)
    if not cls:
        unreal.log_error(f"Missing class {cls_path}")
        return None
    return unreal.EditorLevelLibrary.spawn_actor_from_class(cls, loc, rot)


def main() -> None:
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    unreal.EditorAssetLibrary.make_directory("/Game/Maps")

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.EditorAssetLibrary.delete_asset(MAP_PATH)

    if not les.new_level(MAP_PATH):
        # Fallback for older API path
        unreal.EditorLevelLibrary.new_level(MAP_PATH)

    spawn("/Script/FinalBell.FBRingBounds", unreal.Vector(0.0, 0.0, 100.0))
    spawn("/Script/FinalBell.FBBroadcastCamera", unreal.Vector(0.0, -600.0, 280.0))
    spawn("/Script/Engine.PlayerStart", unreal.Vector(-220.0, 0.0, 120.0))
    spawn(
        "/Script/FinalBell.FBBoxerCharacter",
        unreal.Vector(220.0, 0.0, 120.0),
        unreal.Rotator(0.0, 180.0, 0.0),
    )

    floor_cls = unreal.load_class(None, "/Script/Engine.StaticMeshActor")
    floor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        floor_cls, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    if floor:
        mesh = unreal.load_asset("/Engine/BasicShapes/Plane")
        smc = floor.static_mesh_component
        smc.set_editor_property("static_mesh", mesh)
        floor.set_actor_scale3d(unreal.Vector(12.0, 12.0, 1.0))

    light_cls = unreal.load_class(None, "/Script/Engine.DirectionalLight")
    unreal.EditorLevelLibrary.spawn_actor_from_class(
        light_cls, unreal.Vector(0.0, 0.0, 500.0), unreal.Rotator(-40.0, 30.0, 0.0)
    )

    les.save_current_level()
    unreal.log(f"FinalBell: saved {MAP_PATH}")


if __name__ == "__main__":
    main()
