# Run inside the Final Bell editor. Creates MetaHuman Character assets.
# Does not replace the mannequin, the capsule, or the punch spheres.
import unreal

NAMES = [
    ("MH_FB_Hale", 0.28, 0.45, 1.08, 1.15),
    ("MH_FB_Petrov", 0.82, 0.35, 1.05, 1.08),
    ("MH_FB_Voss", 0.58, 0.55, 1.02, 1.0),
    ("MH_FB_Quinn", 0.40, 0.50, 1.0, 1.0),
    ("MH_FB_Marte", 0.66, 0.62, 0.98, 0.95),
    ("MH_FB_Kane", 0.84, 0.30, 0.94, 0.85),
    ("MH_FB_Abebe", 0.18, 0.48, 0.92, 0.82),
    ("MH_FB_Solano", 0.55, 0.58, 0.90, 0.8),
    ("MH_FB_CustomTemplate", 0.5, 0.5, 1.0, 1.0),
]

PACKAGE = "/Game/MetaHumans"
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)

for name, skin_u, skin_v, height_scale, muscle_scale in NAMES:
    path = f"{PACKAGE}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        character = unreal.EditorAssetLibrary.load_asset(path)
        unreal.log(f"Already on disk: {path}")
    else:
        character = asset_tools.create_asset(
            asset_name=name,
            package_path=PACKAGE,
            asset_class=unreal.MetaHumanCharacter,
            factory=unreal.new_object(type=unreal.MetaHumanCharacterFactoryNew),
        )
    if character is None:
        unreal.log_error(f"Failed to create {name}")
        continue

    skin = character.skin_settings
    skin.skin.u = skin_u
    skin.skin.v = skin_v
    character.skin_settings = skin

    if subsystem.try_add_object_to_edit(character=character):
        try:
            constraints = subsystem.get_body_constraints(character=character, scale_measurement_ranges_with_height=False)
            updated = []
            for constraint in constraints:
                if constraint.name in ("Height", "Muscularity"):
                    constraint.is_active = True
                    scale = height_scale if constraint.name == "Height" else muscle_scale
                    constraint.target_measurement = constraint.target_measurement * scale
                updated.append(constraint)
            if len(updated) > 0:
                subsystem.set_body_constraints(character=character, body_constraints=updated)
                subsystem.commit_body_state(character=character)
        except Exception as exc:
            unreal.log_warning(f"{name} body constraints skipped: {exc}")
        subsystem.remove_object_to_edit(character=character)
    else:
        unreal.log_warning(f"{name} could not be opened for sculpt. The asset was still saved.")

    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log(f"Saved {path}")

unreal.log("MetaHuman roster pass finished. Assembly still needs Build in the MetaHuman editor if textures are not downloaded.")
