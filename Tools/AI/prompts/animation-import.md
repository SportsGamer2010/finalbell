# Animation import

1. Place DeepMotion or Rokoko FBX/BVH in `Tools/Animation/incoming`.
2. Rename clips to `FB_Shared_Punch_Jab_01` style names before treating them as candidates.
3. Run `python Tools/AI/scripts/fb_orchestrator.py run animation-manifest`.
4. If Blender is installed, run `Tools/Blender/fb_cleanup_animation.py` through Blender and keep the JSON report.
5. Retarget onto `/Game/Mannequins/Meshes/SK_Mannequin` in the editor. The current punches are code poses in `ApplySkinnedPose`, not AnimSequences.
6. Use `Tools/Unreal/create_anim_preview_level.py` only inside the editor. It must not modify `Ring_P1`.
