# Remaining manual steps

Do these when you want the matching workflow. The installer will not do them on its own.

1. **Sign in to GitHub CLI.** Run `gh auth login` and choose HTTPS. Until that succeeds, Cursor cannot open or review pull requests through `gh`.
2. **Confirm repository visibility before any push.** The remote is `https://github.com/SportsGamer2010/finalbell.git`. The untracked `Content/Mannequins` files stay local until you decide they may be committed. `.uasset` and `.umap` files go through Git LFS.
3. **Open a new terminal** if `gh`, `node`, `ffmpeg`, or `blender` are not found. The install added them to the user PATH. The orchestrator also checks the registry and the WinGet package folder.
4. **Enable MetaHuman only for a face-authoring session.** In the UE 5.8 editor, enable `MetaHumanCharacter`. Leave it off for ordinary ring work. A MetaHuman still needs boxing wardrobe, gloves, and a retarget onto `SK_Mannequin`.
5. **Author UMG from the layout JSON.** `Tools/AI/output/ui/career-mode.layout.json` is a specification. No Widget Blueprint was created.
6. **Drop Adobe exports** in `Tools/Adobe/inbox`, then run `texture-manifest`. PSD files are flagged for PNG or TGA export. There is no Adobe API connection.
7. **Drop DeepMotion or Rokoko FBX** in `Tools/Animation/incoming` using names such as `FB_Shared_Punch_Jab_01`, then run `animation-manifest` and the Blender cleanup script. Foot contact and retarget stay a human review.
8. **Set `LINEAR_API_KEY`** before creating issues. The ten workstream tasks live in `Tools/AI/config/linear_workstreams.json` and were not sent to Linear.
9. **Set `FIGMA_ACCESS_TOKEN`** and a file key before a Figma export. Layouts currently come from `Tools/AI/config/design_system.json`.
10. **Set `HF_TOKEN` only if public rate limits get in the way.** Do not download large models into the repo until the license is accepted.
11. **Ask before paid software, plugin purchases, administrator prompts, or account linking.** Visual Studio Build Tools are already installed, so that installer was not started.

Combat, defense, camera, and multiplayer behavior stay in the existing C++. A request for those systems routes to the files in `Tools/AI/config/combat_extension_points.json`. This setup pass did not edit them and did not run `Build.bat`.
