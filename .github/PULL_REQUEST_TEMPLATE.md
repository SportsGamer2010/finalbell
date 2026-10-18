## Summary

-

## Linked work

- Branch: `feature/`, `fix/`, or `art/` plus a short name. Include a Linear or GitHub issue id when one exists.
- Owner:
- Reviewer:

## Test plan

- [ ] Read the existing ring, camera, and fighter code before changing it
- [ ] Relevant automated check (`python -m unittest discover -s Tools/AI/tests -v` or the golden contract tests)
- [ ] Unreal editor build, if C++ changed, with the result recorded
- [ ] Gameplay playtest, if combat or camera changed, or an explicit note that it was not run

## Assets

- [ ] Confirmed repository visibility before adding binary files
- [ ] `.uasset`, `.umap`, textures, and FBX are stored with Git LFS
- [ ] Generated previews were not copied into `Content/` without review

## Risk

-
