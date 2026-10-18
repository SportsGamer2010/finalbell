# Collaboration

Two people work in this repo. One person owns a task and the other reviews it. Agree the contract before editing the same binary.

## Branches that already exist

- `master` tracks `origin/master`. This is the current stable branch. It has not been renamed to `main`.
- `cursor/p1-visible-arena-dualsense-input` is the current feature branch.

Do not delete or rename either branch from automation.

## Branch names to use next

- `feature/<issue-or-short-name>` for new behavior
- `fix/<issue-or-short-name>` for corrections
- `art/<short-name>` for fighter models, animation, and textures
- `develop` only when both people want an integration branch

Put a Linear or GitHub issue id in the branch name when an issue exists.

## Pull requests

Use `.github/PULL_REQUEST_TEMPLATE.md`. GitHub Actions runs the toolchain unit tests, the orchestrator invariant check, and the golden contract tests on pull requests and on pushes to `master`, `main`, and `develop`.

`gh` was not authenticated when this workflow was added, so branch protection was not changed on the server.

## Binary files

Git LFS is installed. `.gitattributes` sends `.uasset`, `.umap`, `.fbx`, images, audio, `.psd`, and `.blend` through LFS. `Content/Maps/Ring_P1.umap` is already tracked that way. The copied mannequin assets under `Content/Mannequins/` are still untracked.

Confirm the GitHub repository is private, or confirm which files are allowed to be public, before `git push`. Do not add the untracked mannequin content until that confirmation exists.

## Reviews in Cursor

Review a branch with GitHub once `gh auth login` has succeeded. Until then, review the local diff. Do not mark a review as submitted when the CLI is not authenticated.
